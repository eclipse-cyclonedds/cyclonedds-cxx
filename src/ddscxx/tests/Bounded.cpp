// Copyright(c) 2006 to 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <gtest/gtest.h>
#include <string>

#include "Util.hpp"
#include "dds/dds.hpp"
#include "Serialization.hpp"

/**
 * Fixture for the tests
 */
class Bounds : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::pub::Publisher publisher;
    dds::sub::Subscriber subscriber;
    dds::topic::Topic<Bounded::Msg> topic;
    dds::pub::DataWriter<Bounded::Msg> writer;
    dds::sub::DataReader<Bounded::Msg> reader;
    std::string partition;

    Bounds() :
        participant(dds::core::null),
        publisher(dds::core::null),
        subscriber(dds::core::null),
        topic(dds::core::null),
        writer(dds::core::null),
        reader(dds::core::null),
        partition("Bounds_test")
    {
    }

    void SetUp()
    {
        participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(participant, dds::core::null);

        dds::pub::qos::PublisherQos pubQos =
                participant.default_publisher_qos() <<
                dds::core::policy::Partition(partition);
        publisher = dds::pub::Publisher(participant, pubQos);
        ASSERT_NE(publisher, dds::core::null);

        dds::sub::qos::SubscriberQos subQos =
                participant.default_subscriber_qos() <<
                dds::core::policy::Partition(partition);
        subscriber = dds::sub::Subscriber(participant, subQos);
        ASSERT_NE(subscriber, dds::core::null);

        char topicname[64];
        create_unique_topic_name("bounds_test_topic",topicname,sizeof(topicname));
        topic = dds::topic::Topic<Bounded::Msg>(participant, topicname);
        ASSERT_NE(topic, dds::core::null);

        dds::pub::qos::DataWriterQos writerQos =
                publisher.default_datawriter_qos() <<
                dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances();
        writer = dds::pub::DataWriter<Bounded::Msg>(publisher, topic, writerQos);
        ASSERT_NE(writer, dds::core::null);

        this->reader = dds::sub::DataReader<Bounded::Msg>(subscriber, topic);
        ASSERT_NE(reader, dds::core::null);
    }

    void TakeAndVerifyBoundedString(size_t N)
    {
        auto msgs = reader.take();
        ASSERT_EQ(msgs.length(),1);

        for (const auto &sample:msgs) {
            const auto& msg  = sample.data();
            const auto& info = sample.info();

            ASSERT_TRUE (info.valid());
            std::string str;
            for (size_t i = 0; i < N; i++)
              str.push_back(static_cast<char>('a'+(i%26)));

            ASSERT_EQ (msg.bounded_string(), str);
        }
    }

    void TakeAndVerifyBoundedSequence(size_t N)
    {
        auto msgs = reader.take();
        ASSERT_EQ(msgs.length(),1);

        for (const auto &sample:msgs) {
            const auto& msg  = sample.data();
            const auto& info = sample.info();

            ASSERT_TRUE (info.valid());
            std::vector<int32_t> vec;
            for (size_t i = 0; i < N; i++)
              vec.push_back(static_cast<int32_t>(i+123456));

            ASSERT_EQ (msg.bounded_sequence(), vec);
        }
    }

    void TakeAndVerifyBooleanSequence(size_t N)
    {
        auto msgs = reader.take();
        ASSERT_EQ(msgs.length(),1);

        for (const auto &sample:msgs) {
            const auto& msg  = sample.data();
            const auto& info = sample.info();

            ASSERT_TRUE (info.valid());
            std::vector<bool> vec;
            for (size_t i = 0; i < N; i++)
              vec.push_back(i%2);

            ASSERT_EQ (msg.boolean_sequence(), vec);
        }
    }

    void TearDown()
    {
        this->reader = dds::core::null;
        this->writer = dds::core::null;
        this->topic = dds::core::null;
        this->publisher = dds::core::null;
        this->subscriber = dds::core::null;
        this->participant = dds::core::null;
    }

    void TryWriteBoundedString(size_t N)
    {
        Bounded::Msg msg;
        for (size_t i = 0; i < N; i++)
          msg.bounded_string().push_back(static_cast<char>('a'+(i%26)));

        writer.write(msg);
    }

    void TryWriteBoundedSequence(size_t N)
    {
        Bounded::Msg msg;
        for (size_t i = 0; i < N; i++)
          msg.bounded_sequence().push_back(static_cast<int32_t>(i+123456));

        writer.write(msg);
    }

    void TryWriteBooleanSequence(size_t N)
    {
        Bounded::Msg msg;
        for (size_t i = 0; i < N; i++)
          msg.boolean_sequence().push_back(i%2);

        writer.write(msg);
    }
};

using namespace org::eclipse::cyclonedds::core::cdr;

/**
 * Test writing below, at and beyond bound (255 chars)
 */
TEST_F(Bounds, strings)
{
    TryWriteBoundedString(254);
    TakeAndVerifyBoundedString(254);

    TryWriteBoundedString(255);
    TakeAndVerifyBoundedString(255);

    ASSERT_THROW({
        TryWriteBoundedString(256);
    }, dds::core::InvalidArgumentError) << "Writing a bounded string with length in excess of its bound did not throw an exception.";

    basic_cdr_stream str;
    char arr[16] = {0};
    str.set_buffer(arr, 16);

    Bounded::Msg msg;
    read(str, msg, false);

    ASSERT_EQ (static_cast<serialization_status>(str.status()), serialization_status::illegal_field_value);
}

/**
 * Test writing below, at and beyond bound (255 chars)
 */
TEST_F(Bounds, sequence)
{
    TryWriteBoundedSequence(254);
    TakeAndVerifyBoundedSequence(254);

    TryWriteBoundedSequence(255);
    TakeAndVerifyBoundedSequence(255);

    ASSERT_THROW({
        TryWriteBoundedSequence(256);
    }, dds::core::InvalidArgumentError) << "Writing a bounded sequence with length in excess of its bound did not throw an exception.";
}

/**
 * Test writing below, at and beyond bound (255 chars)
 */
TEST_F(Bounds, boolean)
{
    TryWriteBooleanSequence(254);
    TakeAndVerifyBooleanSequence(254);

    TryWriteBooleanSequence(255);
    TakeAndVerifyBooleanSequence(255);

    ASSERT_THROW({
        TryWriteBooleanSequence(256);
    }, dds::core::InvalidArgumentError) << "Writing a boolean sequence with length in excess of its bound did not throw an exception.";
}
