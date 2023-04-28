// Copyright(c) 2006 to 2021 ZettaScale Technology and others
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
#include "HelloWorldData.hpp"
#include "Space.hpp"


class TestPublisherListener : public virtual dds::pub::NoOpPublisherListener { };

TestPublisherListener publisherListener;



/**
 * Fixture for the DataWriter tests
 */
class Publisher : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::pub::Publisher publisher;

    std::string partition;

    dds::pub::qos::PublisherQos partition_qos;

    Publisher() :
        participant(dds::core::null),
        publisher(dds::core::null),
        partition("Publisher_test")
    {
    }

    void SetUp()
    {
        this->participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(this->participant, dds::core::null);
        this->partition_qos = this->participant.default_publisher_qos() <<
                              dds::core::policy::Partition(this->partition);
    }

    void
    CreatePublisher()
    {
        if (this->publisher == dds::core::null) {
            this->publisher = dds::pub::Publisher(this->participant);
            ASSERT_NE(this->publisher, dds::core::null);
        }
    }

    void
    TearDown()
    {
        this->publisher = dds::core::null;
        this->participant = dds::core::null;
    }

};



/**
 * Tests
 */

TEST_F(Publisher, null)
{
    dds::pub::Publisher publisher1(dds::core::null);
    dds::pub::Publisher publisher2 = dds::core::null;
    ASSERT_EQ(publisher1, dds::core::null);
    ASSERT_EQ(publisher2, dds::core::null);
}

TEST_F(Publisher, create_multiple)
{
    dds::pub::Publisher publisher1 = dds::core::null;
    dds::pub::Publisher publisher2 = dds::core::null;

    publisher1 = dds::pub::Publisher(this->participant);
    ASSERT_NE(publisher1, dds::core::null);

    publisher2 = dds::pub::Publisher(this->participant);
    ASSERT_NE(publisher2, dds::core::null);
}

TEST_F(Publisher, non_default_constructor)
{
    dds::pub::Publisher publisher = dds::core::null;
    dds::core::status::StatusMask statusMask;
    dds::pub::qos::PublisherQos qos = this->participant.default_publisher_qos();

    publisher = dds::pub::Publisher(
                            this->participant,
                            qos,
                            NULL,
                            statusMask);
    ASSERT_NE(publisher, dds::core::null);
}

TEST_F(Publisher, wait_for_acknowledgments)
{
    this->CreatePublisher();

    /* TODO: Implement. */
    ASSERT_THROW({
        this->publisher.wait_for_acknowledgments(dds::core::Duration::from_secs(2));
    }, dds::core::UnsupportedError);
}

TEST_F(Publisher, participant)
{
    this->CreatePublisher();
    ASSERT_EQ(this->publisher.participant(), this->participant);
}

TEST_F(Publisher, default__qos)
{
    dds::pub::qos::DataWriterQos wQos;
    this->CreatePublisher();
    this->publisher.default_datawriter_qos(wQos);
}

TEST_F(Publisher, use_after_close)
{
    /* Get closed publisher. */
    this->CreatePublisher();
    this->publisher.close();

    ASSERT_THROW({
        this->publisher.close();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::core::status::StatusMask mask;
        this->publisher.listener(&publisherListener, mask);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->publisher.listener();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::pub::qos::PublisherQos qos;
        qos = this->publisher.qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->publisher.qos(this->partition_qos);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->publisher << this->partition_qos;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::pub::qos::PublisherQos qos;
        this->publisher >> qos;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::pub::qos::DataWriterQos qos;
        qos = this->publisher.default_datawriter_qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::pub::qos::DataWriterQos qos;
        this->publisher.default_datawriter_qos(qos);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->publisher.wait_for_acknowledgments(dds::core::Duration::from_secs(2));
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->publisher.participant();
    }, dds::core::AlreadyClosedError);
}

TEST_F(Publisher, use_after_deletion)
{
    /* Get deleted publisher. */
    this->CreatePublisher();
    this->publisher = dds::core::null;

    ASSERT_THROW({
        this->publisher.close();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->publisher.close();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::core::status::StatusMask mask;
        this->publisher.listener(&publisherListener, mask);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->publisher.listener();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::pub::qos::PublisherQos qos;
        qos = this->publisher.qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->publisher.qos(this->partition_qos);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->publisher << this->partition_qos;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::pub::qos::PublisherQos qos;
        this->publisher >> qos;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::pub::qos::DataWriterQos qos;
        qos = this->publisher.default_datawriter_qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::pub::qos::DataWriterQos qos;
        this->publisher.default_datawriter_qos(qos);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->publisher.wait_for_acknowledgments(dds::core::Duration::from_secs(2));
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->publisher.participant();
    }, dds::core::NullReferenceError);
}
