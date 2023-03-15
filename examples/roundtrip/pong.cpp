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

#include "dds/dds.h"
#include "dds/dds.hpp"
#include "roundtrip_common.hpp"

#include <iomanip>
#include <iostream>
#include <thread>

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
        case 1:
          timeOut = std::stoul(argv[a]);
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

  std::cout << "# timeOut: " << timeOut << " | using " << (use_listener ? "listener" : "waitset") << " method\n" << std::flush;

  return true;
}

static void print_usage(void)
{
  std::cout <<
    "Usage (parameters must be supplied in order):\n"
    "./cxxRoundtripPong [-l/-h] [timeOut (seconds, 0 = infinite)]\n"
    "Defaults:\n"
    "./cxxRoundtripPong 0\n" << std::flush;

  exit(EXIT_FAILURE);
}

static bool data_available(dds::sub::DataReader<RoundTripModule::DataType>& rd, dds::pub::DataWriter<RoundTripModule::DataType>& wr)
{
  /* Take sample and check that it is valid */
  try {
    auto samples = rd.take();

    if (samples.length() == 0)
      return false;

    auto info = samples.begin()->info();
    if (info.state().instance_state() == dds::sub::status::InstanceState::not_alive_disposed()) {
      std::cout << "# Quitting Pong.\n" << std::flush;
      done = true;
    } else if (info.valid()) {
      timedOut = false;
      wr.write(samples.begin()->data(),samples.begin()->info().timestamp());
    }
  } catch (const dds::core::TimeoutError &) {
    std::cout << "# Timeout encountered.\n" << std::flush;
    return false;
  } catch (const dds::core::Exception &e) {
    std::cout << "# Error: \"" << e.what() << "\".\n" << std::flush;
    return false;
  } catch (...) {
    std::cout << "# Generic error.\n" << std::flush;
    return false;
  }

  return true;
}

int main (int argc, char *argv[])
{
  signal (SIGINT, sigint);

  if (!parse_args(argc, argv)) {
    print_usage();
    return EXIT_SUCCESS;
  }

  dds::domain::DomainParticipant participant(domain::default_id());

  dds::topic::qos::TopicQos tqos;
  tqos << dds::core::policy::Reliability::Reliable(dds::core::Duration::from_secs(10));

  dds::topic::Topic<RoundTripModule::DataType> topic(participant, "RoundTrip", tqos);

  dds::pub::qos::PublisherQos pqos;
  pqos << dds::core::policy::Partition("pong");

  dds::pub::Publisher publisher(participant, pqos);

  dds::pub::qos::DataWriterQos wqos;
  wqos << dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances();

  dds::pub::DataWriter<RoundTripModule::DataType> writer(publisher, topic, wqos);

  dds::sub::qos::SubscriberQos sqos;
  sqos << dds::core::policy::Partition("ping");

  dds::sub::Subscriber subscriber(participant, sqos);

  RoundTripListener listener(writer, &data_available);

  RoundTripListener *list = use_listener ? &listener : nullptr;
  dds::sub::DataReader<RoundTripModule::DataType>
    reader(
      subscriber,
      topic,
      dds::sub::qos::DataReaderQos(),
      list,
      use_listener ? dds::core::status::StatusMask::data_available() : dds::core::status::StatusMask::none());

  dds::core::Duration waittime = timeOut ?
    dds::core::Duration::from_secs(static_cast<double>(timeOut)) :
    dds::core::Duration::infinite();
  if (!match_readers_and_writers(reader, writer, waittime))
    return EXIT_FAILURE;

  if (use_listener) {
    while (!timedOut && !done) {
      timedOut = (timeOut != 0);
      std::this_thread::sleep_for(std::chrono::seconds(timeOut ? timeOut : 1));
    }
  } else {
    dds::core::cond::WaitSet waitset;

    dds::core::cond::StatusCondition rsc(reader);
    rsc.enabled_statuses(dds::core::status::StatusMask::data_available());
    waitset.attach_condition(rsc);

    while (!timedOut && !done) {
      try {
        waitset.wait(waittime);
      } catch (const dds::core::TimeoutError &) {
        std::cout << "\n# Timeout occurred.\n" << std::flush;
        return EXIT_FAILURE;
      } catch (const dds::core::Exception &e) {
        std::cout << "\n# Pong encountered the following error: \"" << e.what() << "\".\n" << std::flush;
        return EXIT_FAILURE;
      } catch (...) {
        std::cout << "\n# Pong encountered an error.\n" << std::flush;
        return EXIT_FAILURE;
      }
      (void) data_available(reader, writer);
    }
  }

  return EXIT_SUCCESS;
}

