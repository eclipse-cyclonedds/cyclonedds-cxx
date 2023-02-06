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

#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <csignal>

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

using namespace org::eclipse::cyclonedds;

#define MAX_SAMPLES 100
#define pubprefix "=== [Publisher] "

static volatile sig_atomic_t done(false); /*semaphore for keeping track of whether to run the test*/

static uint32_t payloadSize(8192); /*size of the payload of the sent messages*/

static std::chrono::milliseconds burstInterval(100); /*interval between bursts of messages*/

static uint32_t burstSize(10); /*number of messages to send each burst*/

static std::chrono::seconds timeOut(30); /*timeout before the writer will give up*/

static std::string partitionName("Throughput example"); /*name of the domain on which the throughput test is run*/

static int parse_args(
    int argc,
    char **argv)
{
  int result = EXIT_SUCCESS;
  /*
   * Get the program parameters
   * Parameters: publisher [payloadSize] [burstInterval] [burstSize] [timeOut] [partitionName]
   */
  if (argc == 2 && (strcmp (argv[1], "-h") == 0 || strcmp (argv[1], "--help") == 0))
  {
    std::cout << pubprefix << "Usage (parameters must be supplied in order):\n";
    std::cout << pubprefix << "./publisher [payloadSize (bytes)] [burstInterval (ms)] [burstSize (samples)] [timeOut (seconds)] [partitionName]\n";
    std::cout << pubprefix << "Defaults:\n";
    std::cout << pubprefix << "./publisher 8192 0 1 0 \"Throughput example\"\n" << std::flush;
    return EXIT_FAILURE;
  }

  if (argc > 1)
    payloadSize = static_cast<uint32_t>(atoi (argv[1])); /* The size of the payload in bytes */

  if (argc > 2)
    burstInterval = std::chrono::milliseconds(atoi (argv[2])); /* The time interval between each burst in ms */

  if (argc > 3)
    burstSize = static_cast<uint32_t>(atoi (argv[3])); /* The number of samples to send each burst */

  if (argc > 4)
    timeOut = std::chrono::seconds(atoi (argv[4])); /* The number of seconds the publisher should run for (0 = infinite) */

  if (argc > 5)
    partitionName = argv[5]; /* The name of the partition */

  std::cout << pubprefix << "Current parameters:\n\tpayloadSize: " << payloadSize << "\n\tbytes burstInterval: " << burstInterval.count() << " ms\n\tburstSize: " << burstSize << " samples\n\ttimeOut: " << timeOut.count() << " seconds\n\tpartitionName: " << partitionName << "\n" << std::flush;

  return result;
}

static void sigint (int sig)
{
  (void)sig;
  done = true;
}

bool wait_for_reader(dds::pub::DataWriter<ThroughputModule::DataType> &writer)
{
  std::cout << "\n" << pubprefix << "Waiting for a reader...\n" << std::flush;

  dds::core::cond::StatusCondition sc = dds::core::cond::StatusCondition(writer);
  sc.enabled_statuses(dds::core::status::StatusMask::publication_matched());

  dds::core::cond::WaitSet waitset;
  waitset.attach_condition(sc);

  dds::core::cond::WaitSet::ConditionSeq conditions =
    waitset.wait(dds::core::Duration::from_secs(30));

  for (const auto & c:conditions)
  {
    if (c == sc)
      return true;
  }
  
  std::cout << pubprefix << "Did not discover a reader.\n" << std::flush;

  return false;
}

void start_writing(
  dds::pub::DataWriter<ThroughputModule::DataType> &writer,
  ThroughputModule::DataType &sample,
  std::chrono::milliseconds &d_int,
  uint32_t b_size,
  std::chrono::seconds &t_out)
{
  bool timedOut = false;

  dds::pub::AnyDataWriter wr(writer);
  /** wr.set_batch(true);  currently the C++ binding does not support batched writing
    *  issue #366 will address this */

  auto pubStart = std::chrono::steady_clock::now();
  auto reportstart = pubStart;

  if (!done)
  {
    std::cout << pubprefix << "Writing samples" << std::flush;

    while (!done && !timedOut)
    {
      auto burstStart = std::chrono::steady_clock::now();
      for (uint32_t i = 0; i < b_size; i++)
      {
        try {
          writer.write(sample);
        } catch (const dds::core::TimeoutError &) {
          timedOut = true;
        } catch (const dds::core::Exception &e) {
          std::cout << "\n" << pubprefix "Writer encountered the following error: \"" << e.what() << "\".\n" << std::flush;
          done = true;
        } catch (...) {
          std::cout << "\n" << pubprefix "Writer encountered an error.\n" << std::flush;
          done = true;
        }
        sample.count()++;
      }

      /** wr.write_flush();  currently the C++ binding does not support batched writing
       *  issue #366 will address this */
      std::this_thread::sleep_until(burstStart + d_int);

      auto n = std::chrono::steady_clock::now();
      if (t_out > std::chrono::milliseconds(0) &&
          n > pubStart+t_out)
      {
        timedOut = true;
      } else if (reportstart + std::chrono::seconds(1) < n) {
        std::cout << "." << std::flush;
        reportstart = n;
      }

      if (writer.publication_matched_status().current_count() == 0) {
        std::cout << "\n" << pubprefix "Writer has no reader to communicate with.\n" << std::flush;
        done = true;
      }
    }

    std::cout << "\n" << pubprefix << (done ? "Terminated" : "Timed out") << ", " << sample.count() << " samples written.\n" << std::flush;
  }
  /** wr.write_flush();  currently the C++ binding does not support batched writing
    *  issue #366 will address this */
}

int main (int argc, char **argv)
{
  ThroughputModule::DataType sample;

  if (parse_args(argc, argv) == EXIT_FAILURE) {
    return EXIT_FAILURE;
  }

  dds::domain::DomainParticipant participant(domain::default_id());

  dds::topic::qos::TopicQos tqos;
  tqos << dds::core::policy::Reliability::Reliable(dds::core::Duration::from_secs(10))
       << dds::core::policy::History::KeepAll()
       << dds::core::policy::ResourceLimits(MAX_SAMPLES);

  dds::topic::Topic<ThroughputModule::DataType> topic(participant, "Throughput", tqos);

  dds::pub::qos::PublisherQos pqos;
  pqos << dds::core::policy::Partition(partitionName);

  dds::pub::Publisher publisher(participant, pqos);

  dds::pub::DataWriter<ThroughputModule::DataType> writer(publisher, topic);

  if (!wait_for_reader(writer))
    return EXIT_FAILURE;

  /* Fill the sample payload with data */
  sample.payload(std::vector<uint8_t>(payloadSize, 'a'));

  /* Register handler for Ctrl-C */
  signal (SIGINT, sigint);

  /* Register the sample instance and write samples repeatedly or until time out */
  start_writing(writer, sample, burstInterval, burstSize, timeOut);

  /* Cleanup */
  writer.dispose_instance(sample);

  return EXIT_SUCCESS;
}
