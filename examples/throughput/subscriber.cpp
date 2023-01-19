/*
 * Copyright(c) 2022 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#include <map>
#include <iomanip>
#include <iostream>
#include <csignal>
#include <thread>

#include "dds/dds.hpp"
#include "Throughput.hpp"

/*
 * The Throughput example measures data throughput in bytes per second. The publisher
 * allows you to specify a payload size in bytes as well as allowing you to specify
 * whether to send data in bursts. The publisher will continue to send data forever
 * unless a time out is specified. The subscriber will receive data and output the
 * total amount received and the data rate in bytes per second. It will also indicate
 * if any samples were received out of order. A maximum number of cycles can be
 * specified and once this has been reached the subscriber will terminate and output
 * totals and averages.
 */

#define BYTES_PER_SEC_TO_MEGABITS_PER_SEC 125000
#define MAX_SAMPLES 1000
#define subprefix "=== [Subscriber] "

using namespace org::eclipse::cyclonedds;

static unsigned long long outOfOrder(0); /*keeps track of out of order samples*/

static unsigned long long total_bytes(0); /*keeps track of total bytes received*/

static unsigned long long total_samples(0); /*keeps track of total samples received*/

static std::chrono::milliseconds pollingDelay(-1); /*default is a listener*/

static unsigned long long maxCycles(0); /*maximum number of display cycles to show*/

static std::string partitionName("Throughput example"); /*name of the domain on which the throughput test is run*/

static std::map<dds::core::InstanceHandle, unsigned long long> mp; /*collection of expected sequence numbers*/

static volatile sig_atomic_t done(false); /*semaphore for keeping track of whether to run the test*/

static int parse_args(int argc, char **argv)
{
  /*
   * Get the program parameters
   * Parameters: subscriber [maxCycles] [pollingDelay] [partitionName]
   */
  if (argc == 2 && (strcmp (argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0))
  {
    std::cout << subprefix << "Usage (parameters must be supplied in order):\n" <<
                 subprefix << "./subscriber [maxCycles (0 = infinite)] [pollingDelay (ms, 0 = no polling, use waitset, -1 = no polling, use listener)] [partitionName]\n" <<
                 subprefix << "Defaults:\n" <<
                 subprefix << "./subscriber 0 0 \"Throughput example\"\n" << std::flush;
    return EXIT_FAILURE;
  }

  if (argc > 1)
  {
    maxCycles = static_cast<unsigned long long>(atoi (argv[1])); /* The number of times to output statistics before terminating */
  }
  if (argc > 2)
  {
    pollingDelay = std::chrono::milliseconds(atoi(argv[2])); /* The number of ms to wait between reads (0 = waitset, -1 = listener) */
  }
  if (argc > 3)
  {
    partitionName = argv[3]; /* The name of the partition */
  }
  return EXIT_SUCCESS;
}

unsigned long long do_take(dds::sub::DataReader<ThroughputModule::DataType>& rd)
{
  auto samples = rd.take();
  unsigned long long valid_samples = 0;
  for (const auto & s:samples)
  {
    if (!s.info().valid())
      continue;

    auto pub_handle = s.info().publication_handle();
    auto ct = s.data().count();
    auto it = mp.insert({pub_handle,ct}).first;

    /*check whether the received sequence number matches that which we expect*/
    if (it->second != ct)
      outOfOrder++;

    valid_samples++;
    total_bytes += s.data().payload().size();
    it->second = ct+1;
  }

  total_samples += valid_samples;
  return valid_samples;
}

void process_samples(dds::sub::DataReader<ThroughputModule::DataType> &reader, size_t max_c)
{
  unsigned long long prev_bytes = 0;
  unsigned long long prev_samples = 0;
  unsigned long cycles = 0;
  auto startTime = std::chrono::steady_clock::now();
  auto prev_time = startTime;

  dds::core::cond::WaitSet waitset;

  dds::core::cond::StatusCondition sc = dds::core::cond::StatusCondition(reader);
  sc.enabled_statuses(dds::core::status::StatusMask::data_available());
  waitset.attach_condition(sc);

  dds::core::Duration waittime =
    dds::core::Duration::from_millisecs(pollingDelay.count() > 0 ? pollingDelay.count() : 100);

  while (!done && (max_c == 0 || cycles < max_c))
  {
    dds::core::cond::WaitSet::ConditionSeq conditions;
    bool wait_again = true;
    try {
      if (pollingDelay.count() == 0) {
        conditions = waitset.wait(waittime);
      } else {
        wait_again = false;
        std::this_thread::sleep_for(pollingDelay.count() > 0 ? pollingDelay : std::chrono::milliseconds(100));
      }
    } catch (const dds::core::TimeoutError &) {
      done = true;
    } catch (const dds::core::Exception &e) {
      std::cout << subprefix << "Waitset encountered the following error: \"" << e.what() << "\".\n" << std::flush;
      break;
    } catch (...) {
      std::cout << subprefix << "Waitset encountered an unknown error.\n" << std::flush;
      break;
    }

    if (reader.subscription_matched_status().current_count() == 0)
      done = true;

    for (const auto &c:conditions)
    {
      if (c.trigger_value())
      {
        wait_again = false;
        break;
      }
    }

    if (wait_again)
      continue;

    /*we only do take in the case of a waitset/polling approach,
      as the listener will have already processed the incoming messages*/
    if (pollingDelay.count() >= 0)
    {
      while (do_take (reader))
        ;
    }

    auto time_now = std::chrono::steady_clock::now();

    if (prev_time == startTime)
    {
      prev_time = time_now;
    } else
    {
      if (time_now > prev_time + std::chrono::seconds(1) && total_samples != prev_samples)
      {
        /* Output intermediate statistics */
        auto deltaTime = static_cast<double>((time_now-prev_time).count())/1e9; //always 1e9?
        std::cout << subprefix << "Received " << total_samples-prev_samples << " samples totalling " << total_bytes-prev_bytes << " bytes in "
                  << std::setprecision(4) << deltaTime << " seconds " << "| Rates: " << static_cast<double>(total_samples - prev_samples) / deltaTime
                  << " samples/s, " << static_cast<double>(total_bytes - prev_bytes) / (deltaTime*BYTES_PER_SEC_TO_MEGABITS_PER_SEC) << " Mbit/s, with "
                  << outOfOrder << " samples received out of order.\n" << std::flush;
        cycles++;
        prev_time = time_now;
        prev_bytes = total_bytes;
        prev_samples = total_samples;
      }
    }
  }

  /* Output totals and averages */
  auto deltaTime = static_cast<double>((std::chrono::steady_clock::now()-startTime).count())/1e9; //always 1e9?
  std::cout << "\n" << subprefix << "Total received: " << total_samples << " samples, " << total_bytes << " bytes.\n";
  std::cout << subprefix << "Out of order: " << outOfOrder << " samples.\n";
  std::cout << subprefix << std::setprecision(4) << "Average transfer rate: " << static_cast<double>(total_samples) / deltaTime << " samples/s, "
            << static_cast<double>(total_bytes) / (deltaTime*BYTES_PER_SEC_TO_MEGABITS_PER_SEC)  << " Mbit/s.\n" << std::flush;
}

static void sigint (int sig)
{
  (void) sig;
  done = true;
}

bool wait_for_writer(dds::sub::DataReader<ThroughputModule::DataType> &reader)
{
  std::cout << "\n" << subprefix << "Waiting for a writer ...\n" << std::flush;

  dds::core::cond::StatusCondition sc = dds::core::cond::StatusCondition(reader);
  sc.enabled_statuses(dds::core::status::StatusMask::subscription_matched());

  dds::core::cond::WaitSet waitset;
  waitset.attach_condition(sc);

  dds::core::cond::WaitSet::ConditionSeq conditions =
    waitset.wait(dds::core::Duration::from_secs(30));

  for (const auto & c:conditions)
  {
    if (c == sc)
      return true;
  }

  std::cout << subprefix << "Did not discover a writer.\n" << std::flush;

  return false;
}

class ThroughputListener: public dds::sub::NoOpDataReaderListener<ThroughputModule::DataType>
{
  public:
  /*implementation of function overrides*/

  /*only on_data_available does anything*/
  void on_data_available(dds::sub::DataReader<ThroughputModule::DataType>& rd) {
    (void)do_take(rd);
  }
};

int main (int argc, char **argv)
{

  if (parse_args(argc, argv) == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  std::cout << subprefix << "Cycles: " << maxCycles << " | PollingDelay: " << pollingDelay.count() << " ms | Partition: " << partitionName << "\n"
            << subprefix << "Using a " << (pollingDelay.count() > 0 ? "polling" : pollingDelay.count() < 0 ? "listener" : "waitset") << " approach.\n" << std::flush;

  dds::domain::DomainParticipant participant(domain::default_id());

  dds::topic::qos::TopicQos tqos;
  tqos << dds::core::policy::Reliability::Reliable(dds::core::Duration::from_secs(10))
       << dds::core::policy::History::KeepAll()
       << dds::core::policy::ResourceLimits(MAX_SAMPLES);

  dds::topic::Topic<ThroughputModule::DataType> topic(participant, "Throughput", tqos);

  dds::sub::qos::SubscriberQos sqos;
  sqos << dds::core::policy::Partition(partitionName);

  dds::sub::Subscriber subscriber(participant, sqos);

  ThroughputListener listener;

  dds::sub::DataReader<ThroughputModule::DataType>
    reader(
      subscriber,
      topic,
      dds::sub::qos::DataReaderQos(),
      pollingDelay.count() < 0 ? &listener : NULL,
      pollingDelay.count() < 0 ? dds::core::status::StatusMask::data_available() : dds::core::status::StatusMask::none());

  /* Process samples until Ctrl-C is pressed or until maxCycles */
  /* has been reached (0 = infinite) */
  signal (SIGINT, sigint);

  if (wait_for_writer (reader))
  {
    std::cout << subprefix << "Waiting for samples...\n" << std::flush;

    process_samples(reader, maxCycles);
  }

  return EXIT_SUCCESS;
}
