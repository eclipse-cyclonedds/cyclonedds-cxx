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
#include "Serialization.hpp"

#define EXPECT_THROW_EXCEPTION(statement, error_msg) \
  ASSERT_THROW(statement, dds::core::Exception); \
  try { \
    statement; \
  } catch (const dds::core::Exception & err) { \
    EXPECT_NE(std::string(err.what()).find(error_msg), std::string::npos); \
  }

namespace
{
template<class T>
void make_sample_(T & sample, const int32_t cnt)
{
  (void) sample;
  (void) cnt;
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

template<>
void make_sample_(Bounded::Msg & sample, const int32_t cnt)
{
  sample.bounded_string(std::to_string(cnt));
  sample.boolean_sequence().reserve(255);
  std::fill(sample.boolean_sequence().begin(), sample.boolean_sequence().begin() + 255, true);
  sample.bounded_sequence().reserve(255);
  std::fill(sample.bounded_sequence().begin(), sample.bounded_sequence().begin() + 255, cnt);
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

  std::vector<T> WriteData(const int32_t instances_cnt)
  {
    std::vector<T> samples;
    // if writer supports loaning
    if (writer.delegate()->is_loan_supported()) {
      for (int32_t i = 0; i < instances_cnt; i++) {
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
    } else {
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

  void run_communication_test(
    const dds::sub::qos::DataReaderQos & r_qos,
    const dds::pub::qos::DataWriterQos & w_qos,
    const int32_t num_samples)
  {
    dds::sub::LoanedSamples<T> samples;
    std::vector<T> test_samples;

    /* setup communication */
    this->SetupCommunication(r_qos, w_qos);
    /* write data. */
    test_samples = this->WriteData(num_samples);
    /* wait for data */
    this->WaitForData();
    /* Check result by taking. */
    samples = this->reader.take();
    this->CheckData(samples, test_samples);
  }

  void run_loan_support_api_test(const bool valid_r_shm_qos, const bool valid_w_shm_qos)
  {
    EXPECT_EQ(this->reader.delegate()->is_loan_supported(),
      (org::eclipse::cyclonedds::topic::TopicTraits<T>::isSelfContained() && valid_r_shm_qos));
    EXPECT_EQ(this->writer.delegate()->is_loan_supported(),
      (org::eclipse::cyclonedds::topic::TopicTraits<T>::isSelfContained() && valid_w_shm_qos));
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

using TestTypes = testing::Types<Space::Type1, HelloWorldData::Msg, Bounded::Msg>;
TYPED_TEST_CASE(SharedMemoryTest, TestTypes);

TYPED_TEST(SharedMemoryTest, writer_reader_valid_shm_qos)
{
  // valid SHM QoS
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::BestEffort();
  r_qos << dds::core::policy::Durability::Volatile();
  r_qos << dds::core::policy::History::KeepLast(10U);
  constexpr bool valid_r_shm_qos = true;

  // valid SHM QoS
  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::Volatile();
  w_qos << dds::core::policy::History::KeepLast(10U);
  constexpr bool valid_w_shm_qos = true;

  // tests
  this->run_communication_test(r_qos, w_qos, 10);
  this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos);
}

TYPED_TEST(SharedMemoryTest, writer_reader_default_qos)
{
  // default Qos (is valid SHM qos)
  dds::sub::qos::DataReaderQos r_qos{};
  dds::pub::qos::DataWriterQos w_qos{};
  constexpr bool valid_shm_qos = true;

  // test communication
  this->run_communication_test(r_qos, w_qos, 1);
  this->run_loan_support_api_test(valid_shm_qos, valid_shm_qos);
}

TYPED_TEST(SharedMemoryTest, writer_valid_shm_qos)
{
  // valid reader shm qos
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  // to not enable SHM for the reader, we should use transient local, but volatile writer and
  // transient local reader is not a valid QoS
  r_qos << dds::core::policy::Durability::Volatile();
  r_qos << dds::core::policy::History::KeepLast(10U);
  constexpr bool valid_r_shm_qos = true;

  // valid writer shm qos
  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::Volatile();
  w_qos << dds::core::policy::History::KeepLast(10U);
  constexpr bool valid_w_shm_qos = true;

  // tests
  this->run_communication_test(r_qos, w_qos, 10);
  this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos);
}

TYPED_TEST(SharedMemoryTest, reader_valid_shm_qos)
{
  // valid reader shm qos
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::Volatile();
  r_qos << dds::core::policy::History::KeepLast(10U);
  constexpr bool valid_r_shm_qos = true;

  // invalid writer shm qos
  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::TransientLocal();
  w_qos << dds::core::policy::History::KeepLast(10U);
  constexpr bool valid_w_shm_qos = false;

  // tests
  this->run_communication_test(r_qos, w_qos, 10);
  this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos);
}

TYPED_TEST(SharedMemoryTest, invalid_shm_qos)
{
  // invalid reader shm QoS
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::TransientLocal();
  r_qos << dds::core::policy::History::KeepLast(10U);
  constexpr bool valid_r_shm_qos = false;

  // invalid writer SHM QoS
  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::TransientLocal();
  w_qos << dds::core::policy::History::KeepLast(10U);
  constexpr bool valid_w_shm_qos = false;

  this->run_communication_test(r_qos, w_qos, 10);
  this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos);
}

TYPED_TEST(SharedMemoryTest, loan_sample)
{
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::Volatile();
  r_qos << dds::core::policy::History::KeepLast(10U);

  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::Volatile();
  w_qos << dds::core::policy::History::KeepLast(10U);

  this->SetupCommunication(r_qos, w_qos);
  using DDSType = typename TestFixture::TopicType;
  // request loan
  if (org::eclipse::cyclonedds::topic::TopicTraits<DDSType>::isSelfContained()) {
    try {
      auto & loaned_sample = this->writer.delegate()->loan_sample();

      // return the loan back without publishing
      this->writer.delegate()->return_loan(loaned_sample);
    } catch (...) {
      // should never execute this
      EXPECT_TRUE(false);
    }
  } else {  // when loaning is not supported
    EXPECT_THROW_EXCEPTION(this->writer.delegate()->loan_sample(),
      "Error Unsupported - sample loan failed.");

    // illegal loan
    DDSType sample;
    EXPECT_THROW_EXCEPTION(
      this->writer.delegate()->return_loan(sample),
      "Error Unsupported - return of sample loan failed.");
  }
}
