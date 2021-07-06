/*
 * Copyright(c) 2006 to 2021 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include <gtest/gtest.h>

#include "dds/dds.hpp"
#include "dds/ddsrt/environ.h"
#include "iceoryx_posh/testing/roudi_gtest.hpp"
#include "HelloWorldData.hpp"
#include "Space.hpp"

namespace
{
template<class T>
void make_sample_(T & sample, const int32_t cnt)
{
  throw std::runtime_error("make_sample_ called on unsupported type");
}

template<>
void make_sample_(Space::Type1 & sample, const int32_t cnt)
{
  sample.long_1(cnt);
  sample.long_2(cnt + 1);
  sample.long_3(cnt + 2);
}

template<>
void make_sample_(HelloWorldData::Msg & sample, const int32_t cnt)
{
  sample.userID(cnt);
  sample.message(std::to_string(cnt));
}

template<class T>
std::vector<T> write_loaned_data_(dds::pub::DataWriter<T> & writer, const int32_t cnt)
{
  std::vector<T> samples;
  for (int32_t i = 0; i < cnt; i++) {
    // loan memory from the middleware
    auto & loaned_sample = writer.delegate()->loan_sample();
    // make sample
    make_sample_(loaned_sample, i);
    // store sample for comparison later
    samples.push_back(loaned_sample);
    // write sample (which will also release the loan to the middleware)
    writer.write(loaned_sample);
  }
  return samples;
}

template<>
std::vector<HelloWorldData::Msg> write_loaned_data_(
  dds::pub::DataWriter<HelloWorldData::Msg> & writer, const int32_t cnt)
{
  // loaning on a non-fixed type throws an error
  try {
    auto & loaned_sample = writer.delegate()->loan_sample();
    (void)loaned_sample;
    EXPECT_FALSE(true);
  } catch (const std::exception & e) {
    EXPECT_STREQ(e.what(), "sample loan failed");
  }

  std::vector<HelloWorldData::Msg> samples;
  for (int32_t i = 0; i < cnt; i++) {
    HelloWorldData::Msg sample;
    // make sample
    make_sample_(sample, i);
    // store sample for comparison later
    samples.push_back(sample);
    // write sample (which will also release the loan to the middleware)
    writer.write(sample);
  }
  return samples;
}
}
/**
 * Fixture for the shared memory tests with RouDi
 */
template<class T>
class SharedMemoryTest : public RouDi_GTest
{
public:
  using TopicType = T;

  dds::domain::DomainParticipant participant;
  dds::sub::Subscriber subscriber;
  dds::pub::Publisher publisher;
  dds::topic::Topic<T> topic;
  dds::sub::DataReader<T> reader;
  dds::pub::DataWriter<T> writer;
  dds::sub::cond::ReadCondition rc;
  dds::core::cond::WaitSet waitset;

  SharedMemoryTest()
  : participant(dds::core::null),
    subscriber(dds::core::null),
    publisher(dds::core::null),
    topic(dds::core::null),
    reader(dds::core::null),
    writer(dds::core::null),
    rc(dds::core::null)
  {
  }

  void SetUp()
  {
  }

  void CreateParticipant()
  {
    if (this->participant == dds::core::null) {
      // configure cyclone to enable shared memory communication
      ddsrt_setenv(
        "CYCLONEDDS_URI",
        "<CycloneDDS><Domain><SharedMemory><Enable>true</Enable><SubQueueCapacity>256</SubQueueCapacity><SubHistoryRequest>16</SubHistoryRequest><PubHistoryCapacity>16</PubHistoryCapacity><LogLevel>verbose</LogLevel></SharedMemory></Domain></CycloneDDS>");
      this->participant = dds::domain::DomainParticipant(
        org::eclipse::cyclonedds::domain::default_id());
      ASSERT_NE(this->participant, dds::core::null);
    }
  }

  void CreateTopic()
  {
    if (this->topic == dds::core::null) {
      this->CreateParticipant();
      this->topic = dds::topic::Topic<T>(this->participant, "datareader_test_topic");
      ASSERT_NE(this->topic, dds::core::null);
    }
  }

  void CreateWriter(dds::pub::qos::DataWriterQos w_qos)
  {
    this->SetupWriter();
    if (this->writer == dds::core::null) {
      this->writer = dds::pub::DataWriter<T>(this->publisher, this->topic, w_qos);
      ASSERT_NE(this->writer, dds::core::null);
    }
  }

  void CreateReader(dds::sub::qos::DataReaderQos r_qos)
  {
    this->SetupReader();
    if (this->reader == dds::core::null) {
      this->reader = dds::sub::DataReader<T>(this->subscriber, this->topic, r_qos);
      ASSERT_NE(this->reader, dds::core::null);
    }
  }

  void SetupWriter()
  {
    this->CreateTopic();
    if (this->publisher == dds::core::null) {
      this->publisher = dds::pub::Publisher(this->participant);
    }
  }

  void SetupReader()
  {
    this->CreateTopic();
    if (this->subscriber == dds::core::null) {
      this->subscriber = dds::sub::Subscriber(this->participant);
    }
  }

  void SetupCommunication(
    dds::sub::qos::DataReaderQos r_qos = dds::sub::qos::DataReaderQos{},
    dds::pub::qos::DataWriterQos w_qos = dds::pub::qos::DataWriterQos{})
  {
    this->CreateWriter(w_qos);
    this->CreateReader(r_qos);
    rc = dds::sub::cond::ReadCondition(reader, dds::sub::status::DataState::any());
    waitset.attach_condition(rc);
  }

  void WaitForData()
  {
    EXPECT_NO_THROW(waitset.wait(dds::core::Duration(10, 0)));
  }

  std::vector<T> WriteData(int32_t instances_cnt)
  {
    std::vector<T> samples;
    for (int32_t i = 0; i < instances_cnt; i++) {
      // make sample
      T sample;
      make_sample_(sample, i);
      // write sample
      this->writer.write(sample);
      // store sample
      samples.push_back(sample);
    }
    return samples;
  }

  std::vector<T> WriteLoanedData(int32_t instances_cnt)
  {
    return write_loaned_data_<T>(this->writer, instances_cnt);
  }

  void
  CheckData(
    const dds::sub::LoanedSamples<T> & samples,
    const std::vector<T> & test_data,
    const dds::sub::status::DataState & test_state =
    dds::sub::status::DataState(dds::sub::status::SampleState::not_read(),
    dds::sub::status::ViewState::new_view(),
    dds::sub::status::InstanceState::alive()))
  {
    unsigned long count = 0UL;
    ASSERT_EQ(samples.length(), test_data.size());
    typename dds::sub::LoanedSamples<T>::const_iterator it;
    for (it = samples.begin(); it != samples.end(); ++it, ++count) {
      const T & data = it->data();
      const dds::sub::SampleInfo & info = it->info();
      const dds::sub::status::DataState & state = info.state();
      ASSERT_EQ(data, test_data[count]);
      ASSERT_EQ(state.view_state(), test_state.view_state());
      ASSERT_EQ(state.sample_state(), test_state.sample_state());
      ASSERT_EQ(state.instance_state(), test_state.instance_state());
    }
  }

  void run_test(
    const dds::sub::qos::DataReaderQos & r_qos,
    const dds::pub::qos::DataWriterQos & w_qos,
    const int32_t num_samples,
    const bool use_loaning = false)
  {
    dds::sub::LoanedSamples<T> samples;
    std::vector<T> test_samples;

    /* setup communication */
    this->SetupCommunication(r_qos, w_qos);
    /* write data. */
    if (use_loaning) {
      test_samples = this->WriteLoanedData(num_samples);
    } else {
      test_samples = this->WriteData(num_samples);
    }
    /* wait for data */
    this->WaitForData();
    /* Check result by taking. */
    samples = this->reader.take();
    this->CheckData(samples, test_samples);
  }

  void TearDown()
  {
    this->writer = dds::core::null;
    this->reader = dds::core::null;
    this->topic = dds::core::null;
    this->publisher = dds::core::null;
    this->subscriber = dds::core::null;
    this->participant = dds::core::null;
  }
};

/**
 * Tests
 */

using TestTypes = testing::Types<Space::Type1, HelloWorldData::Msg>;
TYPED_TEST_CASE(SharedMemoryTest, TestTypes);

TYPED_TEST(SharedMemoryTest, writer_reader_valid_shm_qos)
{
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::BestEffort();
  r_qos << dds::core::policy::Durability::Volatile();
  r_qos << dds::core::policy::History::KeepLast(10U);

  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::Volatile();
  w_qos << dds::core::policy::History::KeepLast(10U);

  this->run_test(r_qos, w_qos, 10);
}

TYPED_TEST(SharedMemoryTest, writer_reader_default_qos)
{
  dds::sub::qos::DataReaderQos r_qos{};
  dds::pub::qos::DataWriterQos w_qos{};

  this->run_test(r_qos, w_qos, 1);
}

TYPED_TEST(SharedMemoryTest, writer_valid_shm_qos)
{
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::BestEffort();
  r_qos << dds::core::policy::Durability::Volatile();
  r_qos << dds::core::policy::History::KeepLast(10U);

  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::Volatile();
  w_qos << dds::core::policy::History::KeepLast(10U);

  this->run_test(r_qos, w_qos, 10);
}

TYPED_TEST(SharedMemoryTest, reader_valid_shm_qos)
{
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::BestEffort();
  r_qos << dds::core::policy::Durability::Volatile();
  r_qos << dds::core::policy::History::KeepLast(10U);

  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::TransientLocal();
  w_qos << dds::core::policy::History::KeepLast(10U);

  this->run_test(r_qos, w_qos, 10);
}

TYPED_TEST(SharedMemoryTest, invalid_shm_qos)
{
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::BestEffort();
  r_qos << dds::core::policy::Durability::TransientLocal();
  r_qos << dds::core::policy::History::KeepLast(10U);

  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::TransientLocal();
  w_qos << dds::core::policy::History::KeepLast(10U);

  this->run_test(r_qos, w_qos, 10);
}

TYPED_TEST(SharedMemoryTest, loan_sample)
{
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::BestEffort();
  r_qos << dds::core::policy::Durability::Volatile();
  r_qos << dds::core::policy::History::KeepLast(10U);

  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::Volatile();
  w_qos << dds::core::policy::History::KeepLast(10U);

  this->run_test(r_qos, w_qos, 10, true);
}
