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

class TestSubscriberListener : public virtual dds::sub::NoOpSubscriberListener { };

TestSubscriberListener subscriberListener;



/**
 * Fixture for the DataReader tests
 */
class Subscriber : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::sub::Subscriber subscriber;

    std::string partition;

    dds::sub::qos::SubscriberQos partition_qos;

    Subscriber() :
        participant(dds::core::null),
        subscriber(dds::core::null),
        partition("Subscriber_test")
    {
    }

    void SetUp()
    {
        this->participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(this->participant, dds::core::null);
        this->partition_qos = this->participant.default_subscriber_qos() <<
                              dds::core::policy::Partition(this->partition);
    }

    void
    CreateSubscriber()
    {
        if (this->subscriber == dds::core::null) {
            this->subscriber = dds::sub::Subscriber(this->participant);
            ASSERT_NE(this->subscriber, dds::core::null);
        }
    }

    void
    TearDown()
    {
        this->subscriber = dds::core::null;
        this->participant = dds::core::null;
    }

};



/**
 * Tests
 */

TEST_F(Subscriber, null)
{
    dds::sub::Subscriber subscriber1(dds::core::null);
    dds::sub::Subscriber subscriber2 = dds::core::null;
    ASSERT_EQ(subscriber1, dds::core::null);
    ASSERT_EQ(subscriber2, dds::core::null);
}

TEST_F(Subscriber, create_multiple)
{
    dds::sub::Subscriber subscriber1 = dds::core::null;
    dds::sub::Subscriber subscriber2 = dds::core::null;

    subscriber1 = dds::sub::Subscriber(this->participant);
    ASSERT_NE(subscriber1, dds::core::null);

    subscriber2 = dds::sub::Subscriber(this->participant);
    ASSERT_NE(subscriber2, dds::core::null);
}

TEST_F(Subscriber, non_default_constructor)
{
    dds::sub::Subscriber subscriber = dds::core::null;
    dds::core::status::StatusMask statusMask;
    dds::sub::qos::SubscriberQos qos = this->participant.default_subscriber_qos();

    subscriber = dds::sub::Subscriber(
                            this->participant,
                            qos,
                            NULL,
                            statusMask);
    ASSERT_NE(subscriber, dds::core::null);
}

TEST_F(Subscriber, participant)
{
    this->CreateSubscriber();
    ASSERT_EQ(this->subscriber.participant(), this->participant);
}

TEST_F(Subscriber, default_qos)
{
    dds::sub::qos::DataReaderQos wQos;
    this->CreateSubscriber();
    this->subscriber.default_datareader_qos(wQos);
}

TEST_F(Subscriber, use_after_close)
{
    /* Get closed subscriber. */
    this->CreateSubscriber();
    this->subscriber.close();

    ASSERT_THROW({
        this->subscriber.close();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::core::status::StatusMask mask;
        this->subscriber.listener(&subscriberListener, mask);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->subscriber.listener();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::sub::qos::SubscriberQos qos;
        qos = this->subscriber.qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->subscriber.qos(this->partition_qos);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->subscriber << this->partition_qos;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::sub::qos::SubscriberQos qos;
        this->subscriber >> qos;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::sub::qos::DataReaderQos qos;
        qos = this->subscriber.default_datareader_qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::sub::qos::DataReaderQos qos;
        this->subscriber.default_datareader_qos(qos);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->subscriber.participant();
    }, dds::core::AlreadyClosedError);
}

TEST_F(Subscriber, use_after_deletion)
{
    /* Get deleted subscriber. */
    this->CreateSubscriber();
    this->subscriber = dds::core::null;

    ASSERT_THROW({
        this->subscriber.close();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->subscriber.close();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::core::status::StatusMask mask;
        this->subscriber.listener(&subscriberListener, mask);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->subscriber.listener();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::sub::qos::SubscriberQos qos;
        qos = this->subscriber.qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->subscriber.qos(this->partition_qos);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->subscriber << this->partition_qos;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::sub::qos::SubscriberQos qos;
        this->subscriber >> qos;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::sub::qos::DataReaderQos qos;
        qos = this->subscriber.default_datareader_qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::sub::qos::DataReaderQos qos;
        this->subscriber.default_datareader_qos(qos);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->subscriber.participant();
    }, dds::core::NullReferenceError);
}

TEST_F(Subscriber, builtin)
{
    this->CreateSubscriber();
    ASSERT_THROW({
        dds::sub::builtin_subscriber(this->participant);
    }, dds::core::UnsupportedError);
}
