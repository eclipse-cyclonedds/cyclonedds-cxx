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
static bool sleep_between_write = false;
static uint32_t keyval = static_cast<uint32_t>(getpid());
static uint32_t ntopics = 10;
static uint32_t stagger = 0; // ms
static std::optional<uint32_t> pubidx;
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

static void make_start_entities (dds::domain::DomainParticipant dp, dds::sub::DataReader<MopSync>& rd, dds::pub::DataWriter<MopSync>& wr)
{
  auto tpqos = dp.default_topic_qos()
    << dds::core::policy::Reliability::Reliable(dds::core::Duration::infinite())
    << dds::core::policy::Durability::TransientLocal()
    << dds::core::policy::History::KeepLast(1);
  auto tp = dds::topic::Topic<MopSync>(dp, "MopSync", tpqos);
  rd = make_reader(tp);
  wr = make_writer(tp);
}

static time_point<CLK> get_start_time(dds::sub::DataReader<MopSync> rd, dds::pub::DataWriter<MopSync> wr)
{
  auto tstart = CLK::now() + 1s;
  int64_t tstart_int64 = duration_cast<nanoseconds>(tstart.time_since_epoch()).count();
  std::cout << keyval << " proposing " << tstart.time_since_epoch().count() << std::endl;
  wr << MopSync{keyval, tstart_int64};
  while (CLK::now() < tstart - 2ms)
  {
    auto ms = rd.take();
    for (const auto& m : ms)
    {
      if (!m.info().valid())
        continue;
      auto prop = time_point<CLK>(nanoseconds(m.data().tstart()));
      if (prop > tstart)
      {
        tstart = prop;
        std::cout << keyval << " updating to " << tstart.time_since_epoch().count() << " from " << m.data().k() << std::endl;
      }
    }
    std::this_thread::sleep_for(1ms);
  }
  tstart += milliseconds(stagger);
  std::cout << keyval << " starting at " << tstart.time_since_epoch().count() << std::endl;
  return tstart;
}

template<typename T>
static void source(std::vector<dds::topic::Topic<T>>& tps)
{
  // make entities for synchronised start first, make sure they stay around while
  // we measure to avoid disturbing the measurement with the entity deletion and
  // associated discovery work
  dds::sub::DataReader<MopSync> start_rd = dds::core::null;
  dds::pub::DataWriter<MopSync> start_wr = dds::core::null;
  make_start_entities(tps[0].domain_participant(), start_rd, start_wr);
  std::vector<dds::pub::DataWriter<T>> wrs;
  for (auto tp : tps)
    wrs.push_back(make_writer(tp));
  signal(SIGINT, sigh);
  signal(SIGTERM, sigh);
  T sample{};
  sample.k(keyval);
  auto now = get_start_time (start_rd, start_wr);
  std::this_thread::sleep_until(now);
  while (!interrupted)
  {
    if (pubidx.has_value())
    {
      wrs[pubidx.value()].write(sample, mkDDSTime(CLK::now()));
    }
    else
    {
      auto nowx = now;
      for (auto wr : wrs)
      {
        wr.write(sample, mkDDSTime(CLK::now()));
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
  std::cout << keyval << "wrote " << ntopics << " * " << sample.seq() << " samples" << std::endl;
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
static void sink(std::vector<dds::topic::Topic<T>>& tps)
{
  // latencies in microseconds
  std::vector<LIK> lats;
  // read until source disappears
  {
    std::vector<dds::sub::DataReader<T>> rds;
    std::vector<Sink<T>> ls;
    for (size_t i = 0; i < tps.size(); i++)
      ls.push_back(Sink<T>{});
    for (size_t i = 0; i < tps.size(); i++)
      rds.push_back(make_reader(tps[i]));
    for (size_t i = 0; i < tps.size(); i++)
      rds[i].listener(&ls[i], dds::core::status::StatusMask::data_available());
    while (!interrupted)
      std::this_thread::sleep_for(103ms);
    for (auto rd : rds)
      rd.close();
    // collect latencies for all topics and sort by reception time
    std::vector<TLIK> tlats;
    for (size_t i = 0; i < ls.size(); i++)
      for (const auto& x : ls[i].lats())
        tlats.push_back(TLIK{x.t, x.l, i, x.k});
    std::sort(tlats.begin(), tlats.end(), [](const TLIK& a, const TLIK& b) -> bool { return a.t < b.t; });
    // then reduce to just latency, topic and key
    for (const auto& x : tlats)
      lats.push_back(LIK{x.l, x.i, x.k});
  }
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
  << "usage: mop {source|sink} [OPTIONS]" << std::endl
  << std::endl
  << "COMMON OPTIONS:" << std::endl
  << "-tTYPE  type to use one of 8, 128 (def), 1k, 8k, 128k" << std::endl
  << "-nNTPS  use N (def = 10) topics in parallel" << std::endl 
  << std::endl
  << "SOURCE OPTIONS:" << std::endl
  << "-kKVAL  use KVAL as key value instead of process id" << std::endl
  << "-pIDX   publish only on topic IDX" << std::endl
  << "-sDELAY stagger: offset by DELAY ms (def = 0)" << std::endl
  << "-x      sleep 100us between successive writes" << std::endl
  << std::endl
  << "SINK OPTIONS:" << std::endl
  << "-oFILE  write latencies to FILE" << std::endl;
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

static bool handle_common_opt (int opt)
{
  switch (opt)
  {
    case 'n':
      ntopics = static_cast<uint32_t>(std::atoi(optarg));
      return true;
    case 't':
      type = convert_typestr(std::string(optarg));
      return true;
    default:
      // not a common option
      return false;
  }
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

  const std::string common_opt = "n:t:";
  optind = 2;
  int opt;
  switch (mode)
  {
    case Mode::Source:
      while ((opt = getopt (argc, argv, (common_opt + "k:p:s:x").c_str())) != EOF)
      {
        if (handle_common_opt (opt))
          continue;
        switch (opt)
        {
          case 'k':
            keyval = static_cast<uint32_t>(std::atoi(optarg));
            break;
          case 'p':
            pubidx = static_cast<uint32_t>(std::atoi(optarg));
            break;
          case 's':
            stagger = static_cast<uint32_t>(std::atoi(optarg));
            break;
          case 'x':
            sleep_between_write = true;
            break;
          default:
            usage();
        }
      }
      break;
    case Mode::Sink:
      while ((opt = getopt (argc, argv, (common_opt + "o:").c_str())) != EOF)
      {
        if (handle_common_opt (opt))
          continue;
        switch (opt)
        {
          case 'o':
            datafile = std::string(optarg);
            break;
          default:
            usage();
        }
      }
      break;
  }
  if (pubidx.has_value() && pubidx.value() >= ntopics)
  {
    std::cout << "topic index for publishing out of range" << std::endl;
    return 1;
  }
  if (argc - optind != 0)
  {
    usage();
  }
  switch (type)
  {
    case Type::T8: run<Mop8>(mode); break;
    case Type::T128: run<Mop128>(mode); break;
    case Type::T1k: run<Mop1k>(mode); break;
    case Type::T8k: run<Mop8k>(mode); break;
    case Type::T128k: run<Mop128k>(mode); break;
  }
  return 0;
}
