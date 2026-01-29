/*
 * Copyright(c) 2024 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <string>
#include <cstdlib>
#include <random>

#include <unistd.h>

#include "dds/dds.hpp"
#include "hop_type.hpp"

using namespace org::eclipse::cyclonedds;
using namespace std::chrono_literals;

static bool use_listener = true;
static double junkrate = 0.0;

template<typename CLK>
static dds::core::Time mkDDSTime (const std::chrono::time_point<CLK> x)
{
  int64_t t = std::chrono::duration_cast<std::chrono::nanoseconds>(x.time_since_epoch()).count();
  return dds::core::Time(t / 1000000000, static_cast<uint32_t>(t % 1000000000));
}

static volatile std::atomic<bool> interrupted = false;
static void sigh(int sig)
{
  static_cast<void>(sig);
  interrupted = true;
}

static const dds::sub::status::DataState not_read()
{
  return dds::sub::status::DataState(dds::sub::status::SampleState::not_read(),
                                     dds::sub::status::ViewState::any(),
                                     dds::sub::status::InstanceState::any());
}

template<typename T>
static dds::sub::DataReader<T> make_reader(dds::topic::Topic<T> tp, int stage)
{
  dds::domain::DomainParticipant dp = tp.domain_participant();
  std::vector<std::string> spart{"P" + std::to_string(stage)};
  dds::sub::qos::SubscriberQos sqos = dp.default_subscriber_qos() << dds::core::policy::Partition(spart);
  dds::sub::Subscriber sub{dp, sqos};
  return dds::sub::DataReader<T>{sub, tp, tp.qos()};
}

template<typename T>
static dds::pub::DataWriter<T> make_writer(dds::topic::Topic<T> tp, int stage)
{
  dds::domain::DomainParticipant dp = tp.domain_participant();
  std::vector<std::string> ppart{"P" + std::to_string(stage)};
  dds::pub::qos::PublisherQos pqos = dp.default_publisher_qos() << dds::core::policy::Partition(ppart);
  dds::pub::Publisher pub{dp, pqos};
  return dds::pub::DataWriter<T>{pub, tp, tp.qos()};
}

// to be run on a separate thread
template<typename T>
static void junksource(dds::topic::Topic<T> tp)
{
  std::random_device ran_dev;
  std::mt19937 prng(ran_dev());
  //std::uniform_int_distribution<> wrdist(0, tps.size() - 1);
  std::exponential_distribution<double> intvdist(junkrate);
  std::vector<dds::pub::DataWriter<T>> wrs;
  wrs.push_back(make_writer(tp, -1));
  T sample{};
  auto now = std::chrono::high_resolution_clock::now();
  while (!interrupted)
  {
    //wrs[wrdist(prng)] << sample;
    wrs[0] << sample;
    ++sample.seq();
    auto delay = std::chrono::duration<double>(intvdist(prng));
    if (delay > 1s)
      delay = 1s;
    now += std::chrono::duration_cast<std::chrono::nanoseconds>(delay);
    std::this_thread::sleep_until(now);
  }
  std::cout << "wrote " << sample.seq() << " junk samples" << std::endl;
}

template<typename T>
static dds::sub::DataReader<T> make_junkreader(dds::topic::Topic<T> tp)
{
  return make_reader(tp, -1);
}

template<typename T>
static void source(dds::topic::Topic<T> tp, int stage, const std::optional<std::string>)
{
  auto wr = make_writer(tp, stage);
  signal(SIGINT, sigh);
  T sample{};
  auto now = std::chrono::high_resolution_clock::now();
  // give forwarders and sink time to start & discovery to run
  std::cout << "starting in 1s" << std::endl;
  now += 1s;
  std::this_thread::sleep_until(now);
  while (!interrupted)
  {
    wr.write(sample, mkDDSTime(now));
    ++sample.seq();
    now += 10ms;
    std::this_thread::sleep_until(now);
  }
  std::cout << "wrote " << sample.seq() << " samples" << std::endl;
}

template<typename T>
static void run_reader(dds::sub::DataReaderListener<T> *list, dds::sub::DataReader<T> rd, std::function<void()> action)
{
  if (use_listener)
  {
    rd.listener(list, dds::core::status::StatusMask::data_available());
    while (!interrupted)
      std::this_thread::sleep_for(103ms);
  }
  else
  {
    dds::core::cond::WaitSet ws;
    dds::sub::cond::ReadCondition rc{rd, not_read()};
    ws += rc;
    while (!interrupted)
    {
      ws.wait();
      action();
    }
  }
}

template<typename T>
class Forward : public dds::sub::NoOpDataReaderListener<T> {
public:
  Forward() = delete;
  Forward(dds::sub::DataReader<T> rd, dds::pub::DataWriter<T> wr) : rd_{rd}, wr_{wr} { }

  void run()
  {
    run_reader(this, rd_, [this](){action();});
  }

private:
  void action()
  {
    auto xs = rd_.take();
    for (const auto& x : xs) {
      if (x.info().valid()) {
        wr_.write (x.data(), x.info().timestamp());
      } else {
        interrupted = true;
      }
    };
  }

  void on_data_available(dds::sub::DataReader<T>&)
  {
    action();
  }

  dds::sub::DataReader<T> rd_;
  dds::pub::DataWriter<T> wr_;
};

template<typename T>
static void forward(dds::topic::Topic<T> tp, int stage, const std::optional<std::string>)
{
  auto rd = make_reader(tp, stage);
  auto wr = make_writer(tp, stage + 1);
  Forward<T> x{rd, wr};
  x.run();
}

template<typename T>
class Sink : public dds::sub::NoOpDataReaderListener<T> {
public:
  Sink() = delete;
  Sink(dds::sub::DataReader<T> rd, std::vector<double>& lats) : rd_{rd}, lats_{lats} { }
  
  void run()
  {
    run_reader(this, rd_, [this](){action();});
  }

private:
  void action()
  {
    const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    auto xs = rd_.take();
    for (const auto& x : xs) {
      if (x.info().valid()) {
        const auto lat = now - (x.info().timestamp().sec() * 1000000000 + x.info().timestamp().nanosec());
        lats_.push_back(lat / 1e3);
      } else {
        interrupted = true;
      }
    };
  }

  void on_data_available(dds::sub::DataReader<T>&)
  {
    action();
  }

  dds::sub::DataReader<T> rd_;
  std::vector<double>& lats_;
};

template<typename T>
static void sink(dds::topic::Topic<T> tp, int stage, const std::optional<std::string> datafile)
{
  // latencies in microseconds
  std::vector<double> lats;
  // read until source disappears
  // always create the "junk reader": it costs us nothing if no junk data is being published
  {
    auto rd = make_reader(tp, stage);
    Sink<T> x{rd, lats};
    x.run();
  }
  // destructors will have run, latencies are ours now
  if (datafile.has_value())
  {
    std::ofstream f;
    f.open(datafile.value());
    for (const auto l : lats)
      f << l << std::endl;
    f.close();
  }
  const size_t n = lats.size();
  if (n < 2) {
    std::cout << "insufficient data" << std::endl;
  } else {
    std::sort(lats.begin(), lats.end());
    std::cout << "received " << n << " samples; min " << lats[0] << " max-1 " << lats[n-2] << " max " << lats[n-1] << std::endl;
  }
}

enum class Mode { Source, Forward, Sink };

template<typename T>
static void run(const Mode mode, int stage, const std::optional<std::string> datafile)
{
  dds::domain::DomainParticipant dp{0};
  auto tpqos = dp.default_topic_qos()
    << dds::core::policy::Reliability::Reliable(dds::core::Duration::infinite())
    << dds::core::policy::History::KeepLast(1);
  dds::topic::Topic<T> tp(dp, "Hop", tpqos);
  std::thread junkthr;
  if (junkrate > 0)
    junkthr = std::thread(junksource<T>, tp);
  auto junkrd = make_junkreader(tp);
  switch (mode)
  {
    case Mode::Source: source(tp, stage, datafile); break;
    case Mode::Forward: forward(tp, stage, datafile); break;
    case Mode::Sink: sink(tp, stage, datafile); break;
  }
  if (junkthr.joinable())
    junkthr.join();
}

// type=128 n=1 bash -c 'bin/hop sink -ohop-result.$n.txt $n $type & i=0;while [[ i -lt n ]]; do bin/hop forward $i $type & i=$((i+1)) ; done ; bin/hop source 0 $type'
// for n in {8..10} ; do n=$n type=128 bash -c 'bin/hop sink -ohop-result.$n.txt $n $type & i=0;while [[ i -lt n ]]; do bin/hop forward $i $type & i=$((i+1)) ; done ; bin/hop source 0 $type & p=$! ; sleep 10 ; kill -INT $p ; wait' ; done

[[noreturn]]
static void usage()
{
  std::cout 
  << "usage: hop {source|forward|sink} [OPTIONS] STAGE TYPE" << std::endl
  << "OPTIONS:" << std::endl
  << "-jRATE  write junk at RATE Hz" << std::endl
  << "-w:     use waitset instead of listener (forward, sink)" << std::endl
  << "-oFILE  write latencies to FILE (sink)" << std::endl
  << "TYPE:   one of 8, 128, 1k, 8k, 128k" << std::endl;
  std::exit(1);
}

int main (int argc, char **argv)
{
  if (argc < 2)
    usage();
  const std::string modestr = std::string(argv[1]);
  Mode mode;
  if (modestr == "source") {
    mode = Mode::Source;
  } else if (modestr == "forward") {
    mode = Mode::Forward;
  } else if (modestr == "sink") {
    mode = Mode::Sink;
  } else {
    std::cout << "invalid mode, should be source, forward or sink" << std::endl;
    return 1;
  }

  std::optional<std::string> datafile;
  optind = 2;
  int opt;
  while ((opt = getopt (argc, argv, "j:o:w")) != EOF)
  {
    switch (opt)
    {
      case 'j':
        junkrate = std::atof(optarg);
        break;
      case 'o':
        datafile = std::string(optarg);
        break;
      case 'w':
        use_listener = false;
        break;
      default:
        usage();
    }
  }

  if (argc - optind != 2)
    usage();
  const int stage = std::atoi(argv[optind]);
  const std::string typestr = std::string(argv[optind + 1]);
  if (typestr == "8") {
    run<Hop8>(mode, stage, datafile);
  } else if (typestr == "128") {
    run<Hop128>(mode, stage, datafile);
  } else if (typestr == "1k") {
    run<Hop1k>(mode, stage, datafile);
  } else if (typestr == "8k") {
    run<Hop8k>(mode, stage, datafile);
  } else if (typestr == "128k") {
    run<Hop128k>(mode, stage, datafile);
  } else {
    std::cout << "invalid type, should be 8, 128, 1k, 8k, 128k" << std::endl;
    return 1;
  }
  return 0;
}
