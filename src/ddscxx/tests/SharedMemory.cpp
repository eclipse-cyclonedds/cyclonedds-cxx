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
#include "dds/ddsi/ddsi_shm_transport.h"
#include "iceoryx_posh/testing/roudi_gtest.hpp"
#include "HelloWorldData.hpp"
#include "Space.hpp"
#include "Serialization.hpp"
#include "iceoryx_posh/popo/subscriber.hpp"
#include "iceoryx_posh/popo/sample.hpp"
#include "iceoryx_utils/cxx/optional.hpp"

#include <random>

#define EXPECT_THROW_EXCEPTION(statement, error_msg) \
  ASSERT_THROW(statement, dds::core::Exception); \
  try { \
    statement; \
  } catch (const dds::core::Exception & err) { \
    EXPECT_NE(std::string(err.what()).find(error_msg), std::string::npos); \
  }

namespace
{

using IceoryxHeader = iceoryx_header;

template<class T>
void make_sample_(T & sample, const int32_t cnt);

template<>
void make_sample_(Space::Type1 & sample, const int32_t cnt)
{
  sample.long_1(cnt);
  sample.long_2(cnt + 1);
  sample.long_3(cnt + 2);
}

template<>
void make_sample_(Space::Type2 & sample, const int32_t cnt)
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

char get_random_char()
{
  static std::mt19937 gen(std::random_device{}());
  static std::uniform_int_distribution<> dist('a', 'z');
  return static_cast<char>(dist(gen));
}

template<>
void make_sample_(Bounded::Msg & sample, const int32_t cnt)
{
  // the sequence types are bounded to 255, so limit the capacity to 255
  int32_t capacity = cnt * 10;
  capacity = (capacity > 255) ? 255 : capacity;

  sample.bounded_string().resize(static_cast<uint32_t>(capacity));
  std::fill(sample.bounded_string().begin(),
    sample.bounded_string().begin() + capacity, get_random_char());

  sample.boolean_sequence().resize(static_cast<uint32_t>(capacity));
  std::fill(sample.boolean_sequence().begin(), sample.boolean_sequence().begin() + capacity, true);

  sample.bounded_sequence().resize(static_cast<uint32_t>(capacity));
  std::fill(sample.bounded_sequence().begin(), sample.bounded_sequence().begin() + capacity, cnt);
}

template<>
void make_sample_(UnBounded::Msg & sample, const int32_t cnt)
{
  // the sequence types are unbounded, reserve the capacity to 100x of the count
  int32_t capacity = cnt * 100;

  sample.unbounded_string().resize(static_cast<uint32_t>(capacity));
  std::fill(sample.unbounded_string().begin(),
    sample.unbounded_string().begin() + capacity, get_random_char());

  sample.unbounded_sequence_bool().resize(static_cast<uint32_t>(capacity));
  std::fill(sample.unbounded_sequence_bool().begin(),
    sample.unbounded_sequence_bool().begin() + capacity, true);

  sample.unbounded_sequence_long().resize(static_cast<uint32_t>(capacity));
  std::fill(sample.unbounded_sequence_long().begin(),
    sample.unbounded_sequence_long().begin() + capacity, cnt);
}

constexpr bool MUST_USE_ICEORYX = true;
constexpr bool DO_NOT_USE_ICEORYX = false;
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
  iox::cxx::optional<iox::popo::Subscriber<T, iceoryx_header_t>> iceoryx_subscriber;

  static constexpr char TOPIC_NAME[] = "datareader_test_topic";

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
      // configuration to enable shared memory communication
      static const std::string shm_config {
        "<CycloneDDS><Domain><SharedMemory><Enable>true</Enable><SubQueueCapacity>256</SubQueueCapacity><SubHistoryRequest>16</SubHistoryRequest><PubHistoryCapacity>16</PubHistoryCapacity><LogLevel>verbose</LogLevel></SharedMemory></Domain></CycloneDDS>"};

      this->participant = dds::domain::DomainParticipant(
        0,
        dds::domain::DomainParticipant::default_participant_qos(),
        nullptr,
        dds::core::status::StatusMask::none(),
        shm_config);
      ASSERT_NE(this->participant, dds::core::null);
    }
  }

  void CreateTopic()
  {
    if (this->topic == dds::core::null) {
      this->CreateParticipant();
      this->topic = dds::topic::Topic<T>(this->participant, TOPIC_NAME);
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

      this->iceoryx_subscriber.emplace(
          iox::capro::ServiceDescription{"DDS_CYCLONE", 
               iox::capro::IdString_t(iox::cxx::TruncateToCapacity, org::eclipse::cyclonedds::topic::TopicTraits<TopicType>::getTypeName()), 
               TOPIC_NAME});
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
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
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
    const bool must_use_iceoryx,
    const dds::sub::LoanedSamples<T> & sample,
    const T & test_data)
  {
    // we only have one sample
    ASSERT_EQ(sample.length(), 1);
    const T & data = sample.begin()->data();
    const dds::sub::SampleInfo & info = sample.begin()->info();
    const dds::sub::status::DataState & state = info.state();
    ASSERT_EQ(data, test_data);
    // the sample view state can either be new or not new (which has not been taken)
    ASSERT_TRUE((state.view_state() == dds::sub::status::ViewState::new_view()) ||
      (state.view_state() == dds::sub::status::ViewState::not_new_view()));
    ASSERT_EQ(state.sample_state(), dds::sub::status::SampleState::not_read());
    ASSERT_EQ(state.instance_state(), dds::sub::status::InstanceState::alive());

    if (must_use_iceoryx) {
      ASSERT_TRUE(iceoryx_subscriber->hasData());
      auto result = iceoryx_subscriber->take();
      ASSERT_FALSE(result.has_error());
      // get the sample with the user header
      auto & iox_sample = result.value();
      // get the user header
      auto user_header = iox_sample.getUserHeader();
      // get the actual data
      auto iceoryx_data = iox_sample.get();

      // if the data in the chunk is of the serialized type
      if (user_header.shm_data_state == IOX_CHUNK_CONTAINS_SERIALIZED_DATA) {
        // Since the data in iceoryx chunk is in the serialized form, take the
        // sample and deserialize the data and then compare with the sent data
        auto buf_ptr = reinterpret_cast<unsigned char *>(const_cast<T *>(iceoryx_data));
        T msg;
        deserialize_sample_from_buffer(buf_ptr, msg);
        ASSERT_EQ(msg, test_data);
      } else if (user_header.shm_data_state == IOX_CHUNK_CONTAINS_RAW_DATA) {
        // the chunk already has deserialized data and can be compared directly
        ASSERT_EQ(*iceoryx_data, test_data);
      } else {
        throw std::runtime_error(
                "the data state is not expected " + std::to_string(user_header.shm_data_state));
      }
    } else {
      ASSERT_FALSE(iceoryx_subscriber->hasData());
    }
  }

  void run_communication_test(
    const bool must_use_iceoryx,
    const dds::sub::qos::DataReaderQos & r_qos,
    const dds::pub::qos::DataWriterQos & w_qos,
    const int32_t num_samples)
  {
    // setup communication
    this->SetupCommunication(r_qos, w_qos);
    // write data
    std::vector<T> test_samples = this->WriteData(num_samples);

    // take the data and verify
    for (int32_t i = 0; i < num_samples; i++) {
      // wait for data
      this->WaitForData();
      // Take one sample at a time (to not introduce undesired flakiness in the test)
      auto sample = this->reader.select().max_samples(1).take();
      // verify the received data
      this->CheckData(must_use_iceoryx, sample, test_samples[static_cast<size_t>(i)]);
    }
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

using TestTypes = ::testing::Types<Space::Type1, Space::Type2, HelloWorldData::Msg,
    Bounded::Msg, UnBounded::Msg>;
TYPED_TEST_SUITE(SharedMemoryTest, TestTypes, );

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
  EXPECT_NO_THROW(this->run_communication_test(MUST_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
}

TYPED_TEST(SharedMemoryTest, writer_reader_default_qos)
{
  // default Qos (is valid SHM qos)
  dds::sub::qos::DataReaderQos r_qos{};
  dds::pub::qos::DataWriterQos w_qos{};
  constexpr bool valid_shm_qos = true;

  // test communication
  EXPECT_NO_THROW(this->run_communication_test(MUST_USE_ICEORYX, r_qos, w_qos, 1));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_shm_qos, valid_shm_qos));
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
  EXPECT_NO_THROW(this->run_communication_test(MUST_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
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
  EXPECT_NO_THROW(this->run_communication_test(DO_NOT_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
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

  EXPECT_NO_THROW(this->run_communication_test(DO_NOT_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
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
  // request loan (supported only for fixed-size self-contained types)
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
