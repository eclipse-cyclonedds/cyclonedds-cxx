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
#include <string>

#include "dds/dds.hpp"
#include "Bounded.hpp"

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

        topic = dds::topic::Topic<Bounded::Msg>(participant, "bounds_test_topic");
        ASSERT_NE(topic, dds::core::null);

        dds::pub::qos::DataWriterQos writerQos =
                publisher.default_datawriter_qos() <<
                dds::core::policy::WriterDataLifecycle::ManuallyDisposeUnregisteredInstances();
        writer = dds::pub::DataWriter<Bounded::Msg>(publisher, topic, writerQos);
        ASSERT_NE(writer, dds::core::null);

        this->reader = dds::sub::DataReader<Bounded::Msg>(subscriber, topic);
        ASSERT_NE(reader, dds::core::null);
    }

    void TakeAndVerify(size_t N)
    {
        auto msgs = reader.take();
        ASSERT_EQ(msgs.length(),1);

        for (const auto &sample:msgs) {
            const auto& msg  = sample.data();
            const auto& info = sample.info();

            ASSERT_TRUE (info.valid());
            ASSERT_EQ (msg.b_str_(), std::string(N,'a'));
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

    void TryWrite(size_t N)
    {
        Bounded::Msg msg;
        msg.b_str_() = std::string(N,'a');

        writer.write(msg);
    }
};

using namespace org::eclipse::cyclonedds::core::cdr;

/**
 * Test writing below, at and beyond bound (255 chars)
 */
TEST_F(Bounds, strings)
{
    TryWrite(254);
    TakeAndVerify(254);

    TryWrite(255);
    TakeAndVerify(255);

    ASSERT_THROW({
        TryWrite(256);
    }, dds::core::InvalidArgumentError) << "Writing a bounded string with length in excess of its bound did not throw an exception.";

    basic_cdr_stream str;
    char arr[16];
    str.set_buffer(arr);
    write(str,uint32_t(0));
    str.position(0);

    Bounded::Msg msg;
    read(str, msg);

    ASSERT_EQ (static_cast<serialization_status>(str.status()), serialization_status::illegal_field_value);
}
