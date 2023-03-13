/*
 * Copyright(c) 2006 to 2022 ZettaScale Technology and others
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
#include "iceoryx_hoofs/cxx/optional.hpp"

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
  dds::sub::Subscriber flt_subscriber;
  dds::pub::Publisher publisher;
  dds::topic::qos::TopicQos t_qos;
  dds::topic::Topic<T> topic;
  dds::topic::ContentFilteredTopic<T> flt_topic;
  dds::sub::DataReader<T> reader;
  dds::sub::DataReader<T> flt_reader;
  dds::pub::DataWriter<T> writer;
  // read condition for the reader
  dds::sub::cond::ReadCondition rc;
  // read condition for the reader of content filtered topic
  dds::sub::cond::ReadCondition flt_rc;
  dds::core::cond::WaitSet waitset;
  iox::cxx::optional<iox::popo::Subscriber<T, iceoryx_header_t>> iceoryx_subscriber;

  static constexpr char TOPIC_NAME[] = "datareader_test_topic";

  SharedMemoryTest()
  : participant(dds::core::null),
    subscriber(dds::core::null),
    flt_subscriber(dds::core::null),
    publisher(dds::core::null),
    topic(dds::core::null),
    flt_topic(dds::core::null),
    reader(dds::core::null),
    flt_reader(dds::core::null),
    writer(dds::core::null),
    rc(dds::core::null),
    flt_rc(dds::core::null)
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
        "<CycloneDDS><Domain><SharedMemory><Enable>true</Enable><LogLevel>verbose</LogLevel></SharedMemory></Domain></CycloneDDS>"};

      this->participant = dds::domain::DomainParticipant(
        0,
        dds::domain::DomainParticipant::default_participant_qos(),
        nullptr,
        dds::core::status::StatusMask::none(),
        shm_config);
      ASSERT_NE(this->participant, dds::core::null);
    }
  }

  void SetDurabilityServiceHistory(int32_t depth) {
    dds::core::policy::DurabilityService durabilityService;
    durabilityService.history_depth(depth);
    this->t_qos << durabilityService;
  }

  void CreateTopic()
  {
    if (this->topic == dds::core::null) {
      this->CreateParticipant();
      this->topic =
          dds::topic::Topic<T>(this->participant, TOPIC_NAME, this->t_qos);
      ASSERT_NE(this->topic, dds::core::null);
    }

    if (this->flt_topic == dds::core::null) {
      this->CreateParticipant();
      this->flt_topic =
          dds::topic::ContentFilteredTopic<T>(this->topic, TOPIC_NAME, dds::topic::Filter(""));
      ASSERT_NE(this->flt_topic, dds::core::null);
      // add a noop filter function
      const auto sample_filter = [](const T &, const dds::sub::SampleInfo &) -> bool {
        return true;
      };
      flt_topic->filter_function(std::move(sample_filter));
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
               iox::capro::IdString_t(iox::cxx::TruncateToCapacity, std::string(".") + std::string(TOPIC_NAME)) /* default partition . topic name */ });
    }

    if (this->flt_reader == dds::core::null) {
      this->flt_reader = dds::sub::DataReader<T>(this->flt_subscriber, this->flt_topic, r_qos);
      ASSERT_NE(this->flt_reader, dds::core::null);
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
    if (this->flt_subscriber == dds::core::null) {
      this->flt_subscriber = dds::sub::Subscriber(this->participant);
    }
  }

  void SetupCommunication(
    dds::sub::qos::DataReaderQos r_qos = dds::sub::qos::DataReaderQos{},
    dds::pub::qos::DataWriterQos w_qos = dds::pub::qos::DataWriterQos{})
  {
    this->CreateWriter(w_qos);
    this->CreateReader(r_qos);
    rc = dds::sub::cond::ReadCondition(reader, dds::sub::status::DataState::any());
    flt_rc = dds::sub::cond::ReadCondition(flt_reader, dds::sub::status::DataState::any());
    waitset.attach_condition(rc);
    waitset.attach_condition(flt_rc);
  }

  dds::core::cond::WaitSet::ConditionSeq WaitForData()
  {
    return waitset.wait(dds::core::Duration(10, 0));
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
      const dds::sub::LoanedSamples<T> & samples,
      const T & test_data) {
    // we only have one sample
    ASSERT_EQ(samples.length(), 1);
    const T & data = samples.begin()->data();
    const dds::sub::SampleInfo & info = samples.begin()->info();
    const dds::sub::status::DataState & state = info.state();
    ASSERT_EQ(data, test_data);
    // the sample view state can either be new or not new (which has not been taken)
    ASSERT_TRUE((state.view_state() == dds::sub::status::ViewState::new_view()) ||
      (state.view_state() == dds::sub::status::ViewState::not_new_view()));
    ASSERT_EQ(state.sample_state(), dds::sub::status::SampleState::not_read());
    ASSERT_EQ(state.instance_state(), dds::sub::status::InstanceState::alive());
  }

  void
  CheckData(
    const bool must_use_iceoryx,
    const dds::sub::LoanedSamples<T> & samples,
    const T & test_data)
  {
    CheckData(samples, test_data);

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
        deserialize_sample_from_buffer(buf_ptr, SIZE_MAX, msg);
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

    int32_t i = 0, flt_i = 0;
    // check until we received expected number of samples on both filtered topic and normal topic
    while(i < num_samples || flt_i < num_samples) {
      // wait for data
      dds::core::cond::WaitSet::ConditionSeq activated_conditions;
      try {
        activated_conditions = this->WaitForData();
      } catch(dds::core::TimeoutError&) {
        // This should never be executed
        ASSERT_TRUE(false) << "Waitset wait timedout";
      }
      EXPECT_LE(activated_conditions.size(), this->waitset.conditions().size());

      for(const auto& cond :activated_conditions ) {
        if (cond == this->rc) {
          // Take one sample at a time (to not introduce undesired flakiness in the test)
          auto sample = this->reader.select().max_samples(1).take();
          // verify the received data, and verify it its received on iceoryx when expected
          this->CheckData(must_use_iceoryx, sample, test_samples[static_cast<size_t>(i)]);
          i++;
        } else if (cond == this->flt_rc) {
          auto flt_sample = this->flt_reader.select().max_samples(1).take();
          // verify the received data
          this->CheckData(flt_sample, test_samples[static_cast<size_t>(flt_i)]);
          flt_i++;
        } else {
          ASSERT_TRUE(false) << "Unexpected condition in waitset";
        }
      }
    }
  }

  void expect_no_communication (
    const dds::sub::qos::DataReaderQos & r_qos,
    const dds::pub::qos::DataWriterQos & w_qos)
  {
    // setup communication
    this->SetupCommunication(r_qos, w_qos);

    // we need to trust this check, otherwise we would have to wait
    // for data until some arbitrary timeout (which may still be to small)
    auto status = this->reader.subscription_matched_status();
    EXPECT_EQ(status.current_count(), 0);
  }

  void run_loan_support_api_test(const bool valid_r_shm_qos, const bool valid_w_shm_qos)
  {
    EXPECT_EQ(this->reader.delegate()->is_loan_supported(),
      (org::eclipse::cyclonedds::topic::TopicTraits<T>::isSelfContained() && valid_r_shm_qos));
    EXPECT_EQ(this->flt_reader.delegate()->is_loan_supported(),
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

  EXPECT_NO_THROW(this->run_communication_test(MUST_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
}

TYPED_TEST(SharedMemoryTest, writer_reader_default_qos)
{
  // default Qos (is valid SHM qos)
  dds::sub::qos::DataReaderQos r_qos{};
  dds::pub::qos::DataWriterQos w_qos{};
  constexpr bool valid_shm_qos = true;

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

  EXPECT_NO_THROW(this->run_communication_test(MUST_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
}

TYPED_TEST(SharedMemoryTest, reader_valid_shm_qos)
{
  // valid reader shm qos
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::Volatile();
  r_qos << dds::core::policy::History::KeepLast(10);
  constexpr bool valid_r_shm_qos = true;

  // invalid writer shm qos
  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::TransientLocal();
  w_qos << dds::core::policy::History::KeepLast(10);
  constexpr bool valid_w_shm_qos = true;

  EXPECT_NO_THROW(this->run_communication_test(MUST_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
}

TYPED_TEST(SharedMemoryTest, invalid_shm_qos)
{
  // invalid reader shm QoS
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::Transient();
  r_qos << dds::core::policy::History::KeepLast(10);
  constexpr bool valid_r_shm_qos = false;

  // invalid writer SHM QoS
  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::Transient();
  w_qos << dds::core::policy::History::KeepLast(10);
  constexpr bool valid_w_shm_qos = false;

  EXPECT_NO_THROW(this->run_communication_test(DO_NOT_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
}

TYPED_TEST(SharedMemoryTest, writer_invalid_shm_qos)
{
  // valid reader shm QoS
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::TransientLocal();
  r_qos << dds::core::policy::History::KeepLast(10);
  constexpr bool valid_r_shm_qos = true;

  // invalid writer SHM QoS
  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::Transient();
  w_qos << dds::core::policy::History::KeepLast(10);
  constexpr bool valid_w_shm_qos = false;  

  EXPECT_NO_THROW(this->run_communication_test(DO_NOT_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
}

TYPED_TEST(SharedMemoryTest, reader_invalid_shm_qos)
{
  // invalid reader shm QoS
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::BestEffort();
  r_qos << dds::core::policy::Durability::Transient();
  r_qos << dds::core::policy::History::KeepLast(10);

  // valid writer SHM QoS
  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::BestEffort();
  w_qos << dds::core::policy::Durability::TransientLocal();
  w_qos << dds::core::policy::History::KeepLast(10);

  EXPECT_NO_THROW(this->expect_no_communication(r_qos, w_qos));
}

TYPED_TEST(SharedMemoryTest, reliable_reader_does_not_match_best_effort_writer)
{
  // valid reader shm QoS
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::TransientLocal();
  r_qos << dds::core::policy::History::KeepLast(10);

  // valid writer SHM QoS
  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::BestEffort();
  w_qos << dds::core::policy::Durability::TransientLocal();
  w_qos << dds::core::policy::History::KeepLast(10);

  this->expect_no_communication(r_qos, w_qos);
}

TYPED_TEST(SharedMemoryTest, best_effort_reader_receives_data_from_reliable_writer_via_shm)
{
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::BestEffort();
  r_qos << dds::core::policy::Durability::TransientLocal();
  r_qos << dds::core::policy::History::KeepLast(10);
  constexpr bool valid_r_shm_qos = true;

  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::TransientLocal();
  w_qos << dds::core::policy::History::KeepLast(10);
  constexpr bool valid_w_shm_qos = true;

  this->SetDurabilityServiceHistory(10);

  EXPECT_NO_THROW(this->run_communication_test(MUST_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
}

TYPED_TEST(SharedMemoryTest, tl_reader_with_lower_history_depth_receives_data_via_shm)
{
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::TransientLocal();
  r_qos << dds::core::policy::History::KeepLast(10U);
  constexpr bool valid_r_shm_qos = true;

  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::TransientLocal();
  w_qos << dds::core::policy::History::KeepLast(11);
  constexpr bool valid_w_shm_qos = true;

  this->SetDurabilityServiceHistory(11);

  EXPECT_NO_THROW(this->run_communication_test(MUST_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
}

TYPED_TEST(SharedMemoryTest, tl_reader_with_larger_history_depth_receives_data_via_shm)
{
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::TransientLocal();
  r_qos << dds::core::policy::History::KeepLast(10);
  constexpr bool valid_r_shm_qos = true;

  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::TransientLocal();
  w_qos << dds::core::policy::History::KeepLast(9);
  constexpr bool valid_w_shm_qos = true;

  this->SetDurabilityServiceHistory(9);

  EXPECT_NO_THROW(this->run_communication_test(MUST_USE_ICEORYX, r_qos, w_qos, 10));
  EXPECT_NO_THROW(this->run_loan_support_api_test(valid_r_shm_qos, valid_w_shm_qos));
}

TYPED_TEST(SharedMemoryTest, tl_reader_does_not_match_volatile_writer)
{
  // valid reader shm QoS
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::TransientLocal();
  r_qos << dds::core::policy::History::KeepLast(10);

  // valid writer SHM QoS
  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::Volatile();
  w_qos << dds::core::policy::History::KeepLast(10);

  this->expect_no_communication(r_qos, w_qos);
}

TYPED_TEST(SharedMemoryTest, loan_sample)
{
  dds::sub::qos::DataReaderQos r_qos;
  r_qos << dds::core::policy::Reliability::Reliable();
  r_qos << dds::core::policy::Durability::Volatile();
  r_qos << dds::core::policy::History::KeepLast(10);

  dds::pub::qos::DataWriterQos w_qos;
  w_qos << dds::core::policy::Reliability::Reliable();
  w_qos << dds::core::policy::Durability::Volatile();
  w_qos << dds::core::policy::History::KeepLast(10);

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
