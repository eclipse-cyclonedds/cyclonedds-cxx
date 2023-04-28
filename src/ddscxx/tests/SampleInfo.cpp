// Copyright(c) 2006 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "dds/dds.hpp"
#include <gtest/gtest.h>
#include "Space.hpp"
#include <vector>
#include <map>


/**
 * Fixture for the Sample_Info tests
 */
class Sample_Info : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::sub::Subscriber subscriber;
    dds::pub::Publisher publisher;
    dds::topic::Topic<Space::Type1> topic;
    dds::sub::DataReader<Space::Type1> reader;
    dds::pub::DataWriter<Space::Type1> writer;
    std::vector<Space::Type1> data;

    dds::sub::qos::DataReaderQos reader_qos;
    dds::pub::qos::DataWriterQos writer_qos;

    static const int32_t history_depth, number_of_keys;

    Sample_Info() :
        participant(org::eclipse::cyclonedds::domain::default_id()),
        subscriber(participant),
        publisher(participant),
        topic(participant, "Unittest_Sample_Info_Space_Type1"),
        reader(dds::core::null),
        writer(dds::core::null),
        reader_qos(),
        writer_qos()
    {
      reader_qos << dds::core::policy::History(dds::core::policy::HistoryKind::KEEP_LAST, history_depth);
      writer_qos << dds::core::policy::History(dds::core::policy::HistoryKind::KEEP_LAST, history_depth);

      for (int i = 0; i < number_of_keys; i++) {
        for (int j = 0; j < history_depth; j++) {
          Space::Type1 s;
          s.long_1(i);
          s.long_2(j);
          data.push_back(s);
        }
      }

    }

    void SetupReaderWriter(bool autodispose) {
      auto wqos = writer_qos;
      wqos << dds::core::policy::WriterDataLifecycle(autodispose);
      reader = dds::sub::DataReader<Space::Type1>(subscriber, topic, reader_qos);
      writer = dds::pub::DataWriter<Space::Type1>(publisher, topic, wqos);

    }

    void Test(bool unregister, bool dispose, int disposed_count, int no_writer_count) {

      writer.write(data.begin(), data.end());

      auto samples = reader.take();
      std::map<int32_t, int> occurances;
      for (const auto & s:samples) {
        ASSERT_TRUE(s.info().valid());

        occurances[s.data().long_1()]++;

        ASSERT_EQ(s.info().generation_count().disposed(), 0);
        ASSERT_EQ(s.info().generation_count().no_writers(), 0);
        ASSERT_EQ(s.data().long_2()+s.info().rank().sample(), history_depth-1);
      }

      ASSERT_EQ(occurances.size(), number_of_keys);
      for (const auto & p:occurances) {
        ASSERT_EQ(p.second, history_depth);
      }
      
      for (const auto & s:samples) {
        if (dispose)
          writer.dispose_instance(s.data());
        if (unregister)
          writer.unregister_instance(s.data());
      }
      occurances.clear();

      writer.write(data.begin(), data.end());

      samples = reader.take();
      for (const auto & s:samples) {
        ASSERT_TRUE(s.info().valid());

        occurances[s.data().long_1()]++;

        ASSERT_EQ(s.info().generation_count().disposed(), disposed_count);
        ASSERT_EQ(s.info().generation_count().no_writers(), no_writer_count);
        ASSERT_EQ(s.data().long_2()+s.info().rank().sample(), history_depth-1);
      }

      ASSERT_EQ(occurances.size(), number_of_keys);
      for (const auto & p:occurances) {
        ASSERT_EQ(p.second, history_depth);
      }
    }

};

const int32_t Sample_Info::history_depth(2);
const int32_t Sample_Info::number_of_keys(2);

TEST_F(Sample_Info, rank_and_generation_count_autodispose_no_dispose_unregister)
{
  this->SetupReaderWriter(true);

  this->Test(true, false, 1, history_depth);
}

TEST_F(Sample_Info, rank_and_generation_count_autodispose_dispose_no_unregister)
{
  this->SetupReaderWriter(true);

  this->Test(false, true, 1, 0);
}

TEST_F(Sample_Info, rank_and_generation_count_autodispose_dispose_unregister)
{
  this->SetupReaderWriter(true);

  this->Test(true, true, 1, history_depth);
}

TEST_F(Sample_Info, rank_and_generation_count_no_autodispose_no_dispose_unregister)
{
  this->SetupReaderWriter(false);

  this->Test(true, false, 0, 1);
}

TEST_F(Sample_Info, rank_and_generation_count_no_autodispose_dispose_no_unregister)
{
  this->SetupReaderWriter(false);

  this->Test(false, true, 1, 0);
}

TEST_F(Sample_Info, rank_and_generation_count_no_autodispose_dispose_unregister)
{
  this->SetupReaderWriter(true);

  this->Test(true, true, 1, history_depth);
}
