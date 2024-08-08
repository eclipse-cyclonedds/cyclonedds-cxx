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
#include "mop_type.hpp"

using namespace org::eclipse::cyclonedds;
using namespace std::chrono_literals;

static bool sleep_between_write = false;
static uint32_t ntopics = 1;
static std::optional<uint32_t> pubidx;
static std::optional<std::string> datafile;

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

template<typename T>
static dds::sub::DataReader<T> make_reader(dds::topic::Topic<T> tp)
{
  dds::domain::DomainParticipant dp = tp.domain_participant();
  std::vector<std::string> spart{"P"};
  dds::sub::qos::SubscriberQos sqos = dp.default_subscriber_qos() << dds::core::policy::Partition(spart);
  dds::sub::Subscriber sub{dp, sqos};
  return dds::sub::DataReader<T>{sub, tp, tp.qos()};
}

template<typename T>
static dds::pub::DataWriter<T> make_writer(dds::topic::Topic<T> tp)
{
  dds::domain::DomainParticipant dp = tp.domain_participant();
  std::vector<std::string> ppart{"P"};
  dds::pub::qos::PublisherQos pqos = dp.default_publisher_qos() << dds::core::policy::Partition(ppart);
  dds::pub::Publisher pub{dp, pqos};
  return dds::pub::DataWriter<T>{pub, tp, tp.qos()};
}

template<typename T>
static void source(std::vector<dds::topic::Topic<T>>& tps)
{
  std::vector<dds::pub::DataWriter<T>> wrs;
  for (auto tp : tps)
    wrs.push_back(make_writer(tp));
  signal(SIGINT, sigh);
  T sample{};
  sample.k(static_cast<uint32_t>(getpid()));
  auto now = std::chrono::high_resolution_clock::now();
  // give forwarders and sink time to start & discovery to run
  std::cout << "starting in 1s" << std::endl;
  now += 1s;
  std::this_thread::sleep_until(now);
  while (!interrupted)
  {
    if (pubidx.has_value())
    {
      wrs[pubidx.value()].write(sample, mkDDSTime(std::chrono::high_resolution_clock::now()));
    }
    else
    {
      auto nowx = now;
      for (auto wr : wrs)
      {
        wr.write(sample, mkDDSTime(std::chrono::high_resolution_clock::now()));
        if (sleep_between_write)
        {
          nowx += 100us;
          std::this_thread::sleep_until(nowx);
        }
      }
    }
    ++sample.seq();
    now += 10ms;
    std::this_thread::sleep_until(now);
  }
  std::cout << "wrote " << ntopics << " * " << sample.seq() << " samples" << std::endl;
}

template<typename T>
class Sink : public dds::sub::NoOpDataReaderListener<T> {
public:
  Sink() = delete;
  Sink(size_t idx, std::vector<std::pair<double, size_t>>& lats) : idx_{idx}, lats_{lats} { }
  
private:
  void on_data_available(dds::sub::DataReader<T>& rd)
  {
    const auto now = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    auto xs = rd.take();
    for (const auto& x : xs) {
      if (x.info().valid()) {
        const auto lat = now - (x.info().timestamp().sec() * 1000000000 + x.info().timestamp().nanosec());
        lats_.push_back(std::make_pair(lat / 1e3, idx_));
      } else {
        interrupted = true;
      }
    };
  }

  size_t idx_;
  std::vector<std::pair<double, size_t>>& lats_;
};

template<typename T>
static void sink(std::vector<dds::topic::Topic<T>>& tps)
{
  // latencies in microseconds
  std::vector<std::pair<double, size_t>> lats;
  // read until source disappears
  // always create the "junk reader": it costs us nothing if no junk data is being published
  {
    std::vector<dds::sub::DataReader<T>> rds;
    std::vector<Sink<T>> ls;
    for (size_t i = 0; i < tps.size(); i++)
      ls.push_back(Sink<T>{i, lats});
    for (size_t i = 0; i < tps.size(); i++)
    {
      rds.push_back(make_reader(tps[i]));
      rds[i].listener(&ls[i], dds::core::status::StatusMask::data_available());
    }
    while (!interrupted)
    {
      std::this_thread::sleep_for(103ms);
    }
    for (auto rd : rds)
    {
      rd.listener();
      rd.close();
    }
  }
  // destructors will have run, latencies are ours now
  if (datafile.has_value())
  {
    std::ofstream f;
    f.open(datafile.value());
    for (const auto& l : lats)
      f << l.first << " " << l.second << std::endl;
    f.close();
  }
  const size_t n = lats.size();
  if (n < 2) {
    std::cout << "insufficient data" << std::endl;
  } else {
    std::sort(lats.begin(), lats.end());
    std::cout << "received " << n << " samples; min " << lats[0].first << " max-1 " << lats[n-2].first << " max " << lats[n-1].first << std::endl;
  }
}

enum class Mode { Source, Sink };

template<typename T>
static void run(const Mode mode)
{
  dds::domain::DomainParticipant dp{0};
  auto tpqos = dp.default_topic_qos()
    << dds::core::policy::Reliability::Reliable(dds::core::Duration::infinite())
    << dds::core::policy::History::KeepLast(1);
  std::vector<dds::topic::Topic<T>> tps;
  for (uint32_t i = 0; i < ntopics; i++)
    tps.push_back(dds::topic::Topic<T>{dp, "Mop" + std::to_string(i), tpqos});
  switch (mode)
  {
    case Mode::Source: source(tps); break;
    case Mode::Sink: sink(tps); break;
  }
}

[[noreturn]]
static void usage()
{
  std::cout 
  << "usage: mop {source|sink} [OPTIONS] TYPE" << std::endl
  << "OPTIONS:" << std::endl
  << "-nNTPS  use N topics in parallel (def = 1)" << std::endl
  << "-pIDX   publish only on topic IDX" << std::endl
  << "-oFILE  write latencies to FILE (sink)" << std::endl
  << "-x      sleep 100us between successive writes" << std::endl
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
  } else if (modestr == "sink") {
    mode = Mode::Sink;
  } else {
    std::cout << "invalid mode, should be source or sink" << std::endl;
    return 1;
  }

  optind = 2;
  int opt;
  while ((opt = getopt (argc, argv, "n:o:p:x")) != EOF)
  {
    switch (opt)
    {
      case 'n':
        ntopics = static_cast<uint32_t>(std::atoi(optarg));
        break;
      case 'o':
        datafile = std::string(optarg);
        break;
      case 'p':
        pubidx = static_cast<uint32_t>(std::atoi(optarg));
        break;
      case 'x':
        sleep_between_write = true;
        break;
      default:
        usage();
    }
  }
  if (pubidx.has_value() && pubidx.value() >= ntopics)
  {
    std::cout << "topic index for publishing out of range" << std::endl;
    return 1;
  }
  if (argc - optind != 1)
  {
    usage();
  }
  const std::string typestr = std::string(argv[optind]);
  if (typestr == "8") {
    run<Mop8>(mode);
  } else if (typestr == "128") {
    run<Mop128>(mode);
  } else if (typestr == "1k") {
    run<Mop1k>(mode);
  } else if (typestr == "8k") {
    run<Mop8k>(mode);
  } else if (typestr == "128k") {
    run<Mop128k>(mode);
  } else {
    std::cout << "invalid type, should be 8, 128, 1k, 8k, 128k" << std::endl;
    return 1;
  }
  return 0;
}
