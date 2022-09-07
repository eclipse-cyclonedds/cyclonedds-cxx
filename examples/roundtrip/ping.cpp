/**
*  Copyright(c) 2022 ZettaScale Technology and others
*
*   This program and the accompanying materials are made available under the
*   terms of the Eclipse Public License v. 2.0 which is available at
*   http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
*   v. 1.0 which is available at
*   http://www.eclipse.org/org/documents/edl-v10.php.
*
*   SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
*/

#include <vector>
#include <chrono>
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <thread>
#include <string>

#include "dds/dds.hpp"
#include "dds/dds.h"
#include "RoundTrip.hpp"
#include "roundtrip_common.hpp"

#define US_IN_ONE_SEC 1000000LL

using times = std::vector<dds_time_t>;

static times
  roundTrip,
  writeAccess,
  readAccess,
  roundTripOverall,
  writeAccessOverall,
  readAccessOverall;

static dds_time_t
  startTime,
  preWriteTime,
  postWriteTime,
  preTakeTime,
  postTakeTime;

static bool warmUp = true;

static RoundTripModule::DataType roundtrip_msg;

static unsigned long payloadSize = 0,
                     elapsed = 0,
                     numSamples = 0;
static const size_t column_width = 8;

static void print_stats(times &to_print)
{
  std::sort(to_print.begin(), to_print.end());

  std::cout << std::setw(column_width) << to_print[to_print.size()/2] << std::setw(column_width) << to_print.front() << std::setw(column_width) << to_print[static_cast<size_t>(static_cast<double>(to_print.size())*0.99)] << std::setw(column_width) << to_print.back() << std::flush;

  to_print.clear();
}

static void print_all(times &rt, times &wa, times &ra)
{
    std::cout << std::setw(column_width) << rt.size() << " | ";
    print_stats(rt);
    std::cout << " | ";
    print_stats(wa);
    std::cout << " | ";
    print_stats(ra);
    std::cout << std::endl << std::flush;
}

static void print_header()
{
  std::cout <<
    "# Warm up complete.\n"
    "\n"
    "#                 |            Latency [us]          |        Write-access time [us]    |     Read-access time [us]\n"
    "#    N |    Count |   median   min     99%     max   |   median   min     99%     max   |   median   min     99%     max\n" << std::flush;
}

static void print_usage(void)
{
  std::cout <<
    "Usage (parameters must be supplied in order):\n"
    "./cxxRoundtripPong [-l/-h] [payloadSize (bytes, 0 - 100M)] [numSamples (0 = infinite)] [timeOut (seconds, 0 = infinite)]\n"
    /*"./cxxRoundtripPong quit - ping sends a quit signal to pong.\n"*/
    "Defaults:\n"
    "./cxxRoundtripPong 0 0 0\n" << std::flush;
  exit(EXIT_FAILURE);
}

template<typename T>
bool data_available(dds::sub::DataReader<T>& rd, dds::pub::DataWriter<T>& wr)
{
  if (done)
    return true;

  dds_time_t difference = 0;

  /* Take sample and check that it is valid */
  try {
    preTakeTime = dds_time ();
    auto samples = rd.take();
    postTakeTime = dds_time ();

    if (samples.length() == 0)
      return false;

    auto info = samples.begin()->info();
    if (info.valid() == false)
      return false;
    else
      timedOut = false;

    if (warmUp) {
      if ((postTakeTime - startTime) > DDS_SECS(5)) {
        warmUp = false;
        startTime = dds_time ();
        print_header();
      }
    } else {
      difference = (postWriteTime - preWriteTime)/DDS_NSECS_IN_USEC;
      writeAccess.push_back(difference);
      writeAccessOverall.push_back(difference);

      difference = (postTakeTime - preTakeTime)/DDS_NSECS_IN_USEC;
      readAccess.push_back(difference);
      readAccessOverall.push_back(difference);

      difference = (postTakeTime - info.timestamp().nanosec() - info.timestamp().sec()*DDS_NSECS_IN_SEC)/(2*DDS_NSECS_IN_USEC);
      roundTrip.push_back(difference);
      roundTripOverall.push_back(difference);

      /* Print stats each second */
      if ((postTakeTime - startTime)/DDS_NSECS_IN_SEC > 1 && roundTrip.size() > 0)
      {

        std::cout << "# " << std::setw(4) << ++elapsed << " | "  << std::flush;
        print_all(roundTrip, writeAccess, readAccess);

        startTime = dds_time ();
      }
    }

    preWriteTime = dds_time();
    wr.write(roundtrip_msg, dds::core::Time(preWriteTime/DDS_NSECS_IN_SEC, static_cast<uint32_t>(preWriteTime%DDS_NSECS_IN_SEC)));
    postWriteTime = dds_time();
  } catch (const dds::core::TimeoutError &) {
    std::cout << "Timeout encountered.\n" << std::flush;
    return false;
  } catch (const dds::core::Exception &e) {
    std::cout << "Error: \"" << e.what() << "\".\n" << std::flush;
    return false;
  } catch (...) {
    std::cout << "Generic error.\n" << std::flush;
    return false;
  }

  return true;
}

static bool parse_args(int argc, char *argv[])
{
  if (argc >= 2)
  {
    if (0 == strcmp(argv[1], "-l"))
      use_listener = true;
    else if (0 == strcmp(argv[1], "-h"))
      return false;
  }

  try {
    for (int a = 1 + (use_listener ? 1 : 0); a < argc; a++) {
      switch (a - (use_listener ? 1 : 0)) {
        case 3:
          timeOut = std::stoul(argv[a]);
          break;
        case 2:
          numSamples = std::stoul(argv[a]);
          break;
        case 1:
          payloadSize = std::stoul(argv[a]);
          break;
        default:
          std::cout << "invalid number of command line parameters supplied\n" << std::flush;
          return false;
      }
    }
  } catch (...) {
    std::cout << "invalid command line parameter supplied\n" << std::flush;
    return false;
  }

  std::cout << "# payloadSize: " << payloadSize << " | numSamples: " << numSamples << " | timeOut: " << timeOut  << " | using " << (use_listener ? "listener" : "waitset") << " method\n" << std::flush;

  return true;
}

int main (int argc, char *argv[])
{
  /* Register handler for Ctrl-C */
  signal (SIGINT, sigint);

  if (!parse_args(argc, argv)) {
    print_usage();
    return EXIT_FAILURE;
  }

  dds::domain::DomainParticipant participant(domain::default_id());

  dds::topic::Topic<RoundTripModule::DataType> topic(participant, "RoundTrip");

  dds::pub::qos::PublisherQos pqos;
  pqos.policy(dds::core::policy::Partition("ping"));

  dds::pub::Publisher publisher(participant, pqos);

  dds::pub::qos::DataWriterQos wqos;
  wqos.policy(
    dds::core::policy::Reliability(
      dds::core::policy::ReliabilityKind::Type::RELIABLE,
      dds::core::Duration::from_secs(10)));
  wqos.policy(
    dds::core::policy::WriterDataLifecycle(true));

  dds::pub::DataWriter<RoundTripModule::DataType> writer(publisher, topic, wqos);

  dds::sub::qos::SubscriberQos sqos;
  sqos.policy(dds::core::policy::Partition("pong"));

  dds::sub::Subscriber subscriber(participant, sqos);

  dds::sub::qos::DataReaderQos rqos;
  rqos.policy(
    dds::core::policy::Reliability(
      dds::core::policy::ReliabilityKind::Type::RELIABLE,
      dds::core::Duration::from_secs(10)));

  RoundTripListener<RoundTripModule::DataType> list(writer, &data_available<RoundTripModule::DataType>);

  dds::sub::DataReader<RoundTripModule::DataType>
    reader(
      subscriber,
      topic,
      rqos,
      use_listener ? &list : NULL,
      use_listener ? dds::core::status::StatusMask::data_available() : dds::core::status::StatusMask::none());

  dds::core::Duration waittime = dds::core::Duration::from_secs(static_cast<double>(timeOut ? timeOut : 5));
  if (!match_readers_and_writers(reader, writer, waittime))
    return EXIT_FAILURE;

  roundtrip_msg.payload(std::vector<uint8_t>(payloadSize, 'a'));

  startTime = dds_time ();
  preWriteTime = dds_time ();
  printf ("# Waiting for startup jitter to stabilise\n");

  //starting write
  if (use_listener) {
    writer.write(roundtrip_msg);
    while (!timedOut && !done && (!numSamples || elapsed < numSamples)) { 
      timedOut = true;
      std::this_thread::sleep_for(std::chrono::seconds(timeOut ? timeOut : 1));
    }
  } else {
    dds::core::cond::WaitSet waitset;

    dds::core::cond::StatusCondition rsc(reader);
    rsc.enabled_statuses(dds::core::status::StatusMask::data_available());
    waitset.attach_condition(rsc);

    writer.write(roundtrip_msg);
    while (!timedOut && !done && (!numSamples || elapsed < numSamples)) {
      try {
        waitset.wait(waittime);
      } catch (const dds::core::TimeoutError &) {
        timedOut = true;
        break;
      }
      (void) data_available(reader, writer);
    }
  }

  if (!warmUp)
  {
    std::cout << "# Overall" << std::flush;
    print_all(roundTripOverall, writeAccessOverall, readAccessOverall);
  }

  //send quit?

  return EXIT_SUCCESS;
}
