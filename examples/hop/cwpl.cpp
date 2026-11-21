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
using namespace std::chrono;

using CLK = high_resolution_clock;

enum class Type { T8, T128, T1k, T8k, T128k };

static Type type = Type::T128;
static uint32_t pairid = 0;
static uint32_t ntopics = 10;
static bool random_timing = false;
static std::optional<std::string> datafile;

static dds::core::Time mkDDSTime (const time_point<CLK> x)
{
  int64_t t = duration_cast<nanoseconds>(x.time_since_epoch()).count();
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
  std::vector<std::string> spart{"P" + std::to_string(pairid + 1 - 2 * (pairid % 2))};
  dds::sub::qos::SubscriberQos sqos = dp.default_subscriber_qos() << dds::core::policy::Partition(spart);
  dds::sub::Subscriber sub{dp, sqos};
  return dds::sub::DataReader<T>{sub, tp, tp.qos()};
}

template<typename T>
static dds::pub::DataWriter<T> make_writer(dds::topic::Topic<T> tp)
{
  dds::domain::DomainParticipant dp = tp.domain_participant();
  std::vector<std::string> ppart{"P" + std::to_string(pairid)};
  dds::pub::qos::PublisherQos pqos = dp.default_publisher_qos() << dds::core::policy::Partition(ppart);
  dds::pub::Publisher pub{dp, pqos};
  return dds::pub::DataWriter<T>{pub, tp, tp.qos()};
}

template<typename T>
static void source(dds::pub::DataWriter<T> wr)
{
  T sample{};
  sample.k(0);
  auto now = CLK::now();
  while (!interrupted)
  {
    wr.write(sample, mkDDSTime(CLK::now()));
    ++sample.seq();
    now += 10ms;
    std::this_thread::sleep_until(now);
  }
}

template<typename T>
static void randomsource(dds::pub::DataWriter<T> wr)
{
  std::random_device ran_dev;
  std::mt19937 prng(ran_dev());
  std::exponential_distribution<double> intvdist(100);

  T sample{};
  sample.k(0);
  auto now = CLK::now();
  while (!interrupted)
  {
    wr.write(sample, mkDDSTime(CLK::now()));
    ++sample.seq();
    auto delay = std::chrono::duration<double>(intvdist(prng));
    if (delay > 1s)
      delay = 1s;
    now += std::chrono::duration_cast<std::chrono::nanoseconds>(delay);
    std::this_thread::sleep_until(now);
  }
}

// t = reception time, l = latency, i = topic index, k = source key
struct TLK { int64_t t; double l; uint32_t k; };
struct TLIK { int64_t t; double l; size_t i; uint32_t k; };
struct LIK { double l; size_t i; uint32_t k; };

template<typename T>
class Sink : public dds::sub::NoOpDataReaderListener<T> {
public:
  Sink() = default;
  
  const std::vector<TLK>& lats() const {
    return lats_;
  };
  
private:
  void on_data_available(dds::sub::DataReader<T>& rd)
  {
    const auto now_clk = CLK::now();
    const int64_t now = duration_cast<nanoseconds>(now_clk.time_since_epoch()).count();
    auto xs = rd.take();
    for (const auto& x : xs) {
      if (x.info().valid()) {
        const auto lat = now - (x.info().timestamp().sec() * 1000000000 + x.info().timestamp().nanosec());
        lats_.push_back(TLK{now, lat / 1e3, x.data().k()});
      } else {
        interrupted = true;
      }
    };
  }

  std::vector<TLK> lats_;
};

template<typename T>
static void run()
{
  dds::domain::DomainParticipant dp{0};
  auto tpqos = dp.default_topic_qos()
    << dds::core::policy::Reliability::Reliable(dds::core::Duration::infinite())
    << dds::core::policy::History::KeepLast(1);
  std::vector<dds::topic::Topic<T>> tps;
  std::vector<dds::pub::DataWriter<T>> wrs;
  for (uint32_t i = 0; i < ntopics; i++)
    tps.push_back(dds::topic::Topic<T>{dp, "Mop" + std::to_string(i), tpqos});
  for (auto& tp : tps)
    wrs.push_back(make_writer(tp));
  std::vector<dds::sub::DataReader<T>> rds;
  std::vector<Sink<T>> ls;
  for (size_t i = 0; i < tps.size(); i++)
    ls.push_back(Sink<T>{});
  for (size_t i = 0; i < tps.size(); i++)
    rds.push_back(make_reader(tps[i]));
  for (size_t i = 0; i < tps.size(); i++)
    rds[i].listener(&ls[i], dds::core::status::StatusMask::data_available());

  signal(SIGINT, sigh);
  signal(SIGTERM, sigh);
  std::vector<std::thread> threads;
  for (auto wr : wrs)
    threads.push_back(std::thread(random_timing ? randomsource<T> : source<T>, wr));

  // latencies in microseconds
  std::vector<LIK> lats;
  while (!interrupted)
    std::this_thread::sleep_for(103ms);
  for (auto& t : threads)
    t.join();
  for (auto rd : rds)
    rd.close();
  for (auto wr : wrs)
    wr.close();
  // collect latencies for all topics and sort by reception time
  std::vector<TLIK> tlats;
  for (size_t i = 0; i < ls.size(); i++)
    for (const auto& x : ls[i].lats())
      tlats.push_back(TLIK{x.t, x.l, i, x.k});
  std::sort(tlats.begin(), tlats.end(), [](const TLIK& a, const TLIK& b) -> bool { return a.t < b.t; });
  // then reduce to just latency, topic and key
  for (const auto& x : tlats)
    lats.push_back(LIK{x.l, x.i, x.k});

  if (datafile.has_value())
  {
    std::ofstream f;
    f.open(datafile.value());
    for (const auto& l : lats)
      f << l.l << " " << l.i << " " << l.k << std::endl;
    f.close();
  }
  const size_t n = lats.size();
  if (n < 2) {
    std::cout << "insufficient data" << std::endl;
  } else {
    std::sort(lats.begin(), lats.end(), [](const LIK& a, const LIK& b) -> bool { return a.l < b.l; });
    std::cout
      << "received " << n
      << " samples; min " << lats[0].l
      << " max-1 " << lats[n-2].l
      << " max " << lats[n-1].l << std::endl;
  }
}

[[noreturn]]
static void usage()
{
  std::cout 
  << "usage: cwpl [OPTIONS] id" << std::endl
  << std::endl
  << "OPTIONS:" << std::endl
  << "-tTYPE  type to use one of 8, 128 (def), 1k, 8k, 128k" << std::endl
  << "-nNTPS  use N (def = 10) topics in parallel" << std::endl
  << "-r      use randomized write intervals with average 10ms" << std::endl
  << "-oFILE  write latencies to FILE" << std::endl
  << std::endl
  << "id = 0 writes in partition P0, reads from P1" << std::endl
  << "id = 1 writes in partition P1, reads from P0" << std::endl
  << "id = 2 writes in partition P2, reads from P3" << std::endl
  << "etc." << std::endl;
  std::exit(1);
}

static Type convert_typestr (const std::string& typestr)
{
  if (typestr == "8") {
    return Type::T8;
  } else if (typestr == "128") {
    return Type::T128;
  } else if (typestr == "1k") {
    return Type::T1k;
  } else if (typestr == "8k") {
    return Type::T8k;
  } else if (typestr == "128k") {
    return Type::T128k;
  } else {
    std::cout << "invalid type, should be 8, 128, 1k, 8k, 128k" << std::endl;
    std::exit(1);
    return Type::T128;
  }
}

int main (int argc, char **argv)
{
  if (argc < 1)
    usage();

  int opt;
  while ((opt = getopt (argc, argv, "n:o:rt:")) != EOF)
  {
    switch (opt)
    {
      case 'n':
        ntopics = static_cast<uint32_t>(std::atoi(optarg));
        break;
      case 'o':
        datafile = std::string(optarg);
        break;
      case 'r':
        random_timing = true;
        break;
      case 't':
        type = convert_typestr(std::string(optarg));
        break;
      default:
        usage();
    }
  }
  if (argc - optind != 1)
  {
    usage();
  }
  pairid = static_cast<uint32_t>(std::atoi(argv[optind]));
  switch (type)
  {
    case Type::T8: run<Mop8>(); break;
    case Type::T128: run<Mop128>(); break;
    case Type::T1k: run<Mop1k>(); break;
    case Type::T8k: run<Mop8k>(); break;
    case Type::T128k: run<Mop128k>(); break;
  }
  return 0;
}
