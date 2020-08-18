/*
 * Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include "dds/dds.hpp"
#include "dds/ddscxx/test.h"
#include "Space_DCPS.hpp"



/**
 * Dummy listener for the Topic tests
 */
namespace ddscxx { namespace tests { namespace Topic {
    class TestTopic1Listener : public virtual dds::topic::NoOpTopicListener<Space::Type1>{ };
} } }
ddscxx::tests::Topic::TestTopic1Listener topic1Listener;



/**
 * Fixture for the Topic tests
 */
class ddscxx_Topic : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::topic::Topic<Space::Type1> topic;
    dds::topic::Topic<Space::Type1> topic1A;
    dds::topic::Topic<Space::Type1> topic1B;
    dds::topic::Topic<Space::Type2> topic2A;
    dds::topic::Topic<Space::Type2> topic2B;
    dds::topic::qos::TopicQos reliable_qos;
    dds::topic::qos::TopicQos lifespan_qos;

    ddscxx_Topic() :
        participant(dds::core::null),
        topic(dds::core::null),
        topic1A(dds::core::null),
        topic1B(dds::core::null),
        topic2A(dds::core::null),
        topic2B(dds::core::null),
        reliable_qos(),
        lifespan_qos()
    {
    }

    void SetUp() {
        // Create participant
        participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(participant, dds::core::null);

        reliable_qos = participant.default_topic_qos();
        reliable_qos.policy(dds::core::policy::Reliability::Reliable());

        lifespan_qos = participant.default_topic_qos();
        lifespan_qos.policy(dds::core::policy::Lifespan(dds::core::Duration(10, 0)));
    }

    void CreateTopic() {
        topic = dds::topic::Topic<Space::Type1>(this->participant, "topic");
        ASSERT_NE(topic, dds::core::null);
    }

    void CreateTopics() {
        topic1A = dds::topic::Topic<Space::Type1>(this->participant, "topic1A");
        ASSERT_NE(topic1A, dds::core::null);
        topic1B = dds::topic::Topic<Space::Type1>(this->participant, "topic1B");
        ASSERT_NE(topic1B, dds::core::null);
        topic2A = dds::topic::Topic<Space::Type2>(this->participant, "topic2A");
        ASSERT_NE(topic2A, dds::core::null);
        topic2B = dds::topic::Topic<Space::Type2>(this->participant, "topic2B");
        ASSERT_NE(topic2B, dds::core::null);
    }

    void TearDown() {
        topic = dds::core::null;
        topic1A = dds::core::null;
        topic1B = dds::core::null;
        topic2A = dds::core::null;
        topic2B = dds::core::null;
        participant = dds::core::null;
    }
};



/**
 * Tests
 */

DDSCXX_TEST_F(ddscxx_Topic, null)
{
    dds::topic::Topic<Space::Type1> topic1(dds::core::null);
    dds::topic::Topic<Space::Type2> topic2 = dds::core::null;
    ASSERT_EQ(topic1, dds::core::null);
    ASSERT_EQ(topic2, dds::core::null);
}


DDSCXX_TEST_F(ddscxx_Topic, create_null)
{
    dds::domain::DomainParticipant participant = dds::core::null;;
    dds::topic::Topic<Space::Type1> ttopic = dds::core::null;

    ASSERT_THROW({
        ttopic = dds::topic::Topic<Space::Type1>(participant, "topic_null");
    }, dds::core::NullReferenceError);
}


DDSCXX_TEST_F(ddscxx_Topic, create)
{
    dds::topic::Topic<Space::Type1> ttopic = dds::core::null;
    ttopic = dds::topic::Topic<Space::Type1>(this->participant, "ttopic");
    ASSERT_NE(ttopic, dds::core::null);
}


DDSCXX_TEST_F(ddscxx_Topic, create_same)
{
    dds::topic::Topic<Space::Type1> ttopic1A = dds::core::null;
    dds::topic::Topic<Space::Type1> ttopic1B = dds::core::null;
    ttopic1A = dds::topic::Topic<Space::Type1>(this->participant, "ttopic");
    ASSERT_NE(ttopic1A, dds::core::null);
    ttopic1B = dds::topic::Topic<Space::Type1>(this->participant, "ttopic");
    ASSERT_NE(ttopic1B, dds::core::null);

    ASSERT_STREQ(ttopic1A.type_name().c_str(), ttopic1B.type_name().c_str());
}


DDSCXX_TEST_F(ddscxx_Topic, create_conflict)
{
    dds::topic::Topic<Space::Type1> ttopic1 = dds::core::null;
    dds::topic::Topic<Space::Type2> ttopic2 = dds::core::null;

    ttopic1 = dds::topic::Topic<Space::Type1>(this->participant, "ttopic");
    ASSERT_NE(ttopic1, dds::core::null);

    ASSERT_THROW({
        ttopic2 = dds::topic::Topic<Space::Type2>(this->participant, "ttopic");
    }, dds::core::PreconditionNotMetError);
}


DDSCXX_TEST_F(ddscxx_Topic, name)
{
    this->CreateTopics();
    ASSERT_STREQ(this->topic1A.name().c_str(), "topic1A");
    ASSERT_STREQ(this->topic1B.name().c_str(), "topic1B");
    ASSERT_STREQ(this->topic2A.name().c_str(), "topic2A");
    ASSERT_STREQ(this->topic2B.name().c_str(), "topic2B");
}


DDSCXX_TEST_F(ddscxx_Topic, type_name_default)
{
    this->CreateTopics();
    ASSERT_STREQ(this->topic1A.type_name().c_str(), "Space::Type1");
    ASSERT_STREQ(this->topic1B.type_name().c_str(), "Space::Type1");
    ASSERT_STREQ(this->topic2A.type_name().c_str(), "Space::Type2");
    ASSERT_STREQ(this->topic2B.type_name().c_str(), "Space::Type2");
}


DDSCXX_TEST_F(ddscxx_Topic, type_name_nondefault)
{
    /* TODO: Implement. For now it throws unsupported. */

    ASSERT_THROW({
        dds::topic::Topic<Space::Type1> ttopic = dds::core::null;
        ttopic = dds::topic::Topic<Space::Type1>(
                                    this->participant,
                                    "mytopic",
                                    "mytype");
    }, dds::core::UnsupportedError);

    ASSERT_THROW({
        dds::topic::Topic<Space::Type1> ttopic = dds::core::null;
        dds::core::status::StatusMask mask;
        ttopic = dds::topic::Topic<Space::Type1>(
                                    this->participant,
                                    "mytopic",
                                    "mytype",
                                    this->reliable_qos,
                                    &topic1Listener,
                                    mask);
    }, dds::core::UnsupportedError);
}


DDSCXX_TEST_F(ddscxx_Topic, domain_participant)
{
    this->CreateTopic();
    ASSERT_EQ(this->topic.domain_participant(), this->participant);
}


DDSCXX_TEST_F(ddscxx_Topic, qos_default)
{
    dds::topic::qos::TopicQos shift_qos;
    dds::topic::qos::TopicQos dflt_qos;
    dds::topic::qos::TopicQos get_qos;
    this->CreateTopic();
    get_qos = this->topic.qos();
    this->topic >> shift_qos;
    ASSERT_EQ(get_qos, dflt_qos);
    ASSERT_EQ(get_qos, shift_qos);
    ASSERT_EQ(get_qos, this->participant.default_topic_qos());
    ASSERT_NE(get_qos, this->reliable_qos);
}


DDSCXX_TEST_F(ddscxx_Topic, qos_nondefault_constructor)
{
    dds::topic::qos::TopicQos shift_qos;
    dds::topic::qos::TopicQos get_qos;
    dds::topic::Topic<Space::Type1> ttopic = dds::core::null;
    dds::core::status::StatusMask mask;
    ttopic = dds::topic::Topic<Space::Type1>(
                                this->participant,
                                "mytopic",
                                this->lifespan_qos,
                                &topic1Listener,
                                mask);
    ASSERT_NE(ttopic, dds::core::null);
    get_qos = ttopic.qos();
    ttopic >> shift_qos;
    ASSERT_EQ(get_qos, shift_qos);
    ASSERT_EQ(get_qos, this->lifespan_qos);
    ASSERT_NE(get_qos, this->participant.default_topic_qos());
}


DDSCXX_TEST_F(ddscxx_Topic, qos_immutable_constructor)
{
    dds::topic::Topic<Space::Type1> ttopic = dds::core::null;
    dds::core::status::StatusMask mask;
    ttopic = dds::topic::Topic<Space::Type1>(
                                this->participant,
                                "mytopic",
                                this->reliable_qos,
                                &topic1Listener,
                                mask);
    ASSERT_NE(ttopic, dds::core::null);
    ASSERT_EQ(ttopic.qos(), this->reliable_qos);
    ASSERT_NE(ttopic.qos(), this->participant.default_topic_qos());
}


DDSCXX_TEST_F(ddscxx_Topic, qos_nondefault_set)
{
    this->CreateTopic();
    this->topic.qos(this->lifespan_qos);
    ASSERT_EQ(this->topic.qos(), this->lifespan_qos);
    ASSERT_NE(this->topic.qos(), this->participant.default_topic_qos());
}


DDSCXX_TEST_F(ddscxx_Topic, qos_immutable_set)
{
    this->CreateTopic();
    ASSERT_THROW({
        this->topic.qos(this->reliable_qos);
    }, dds::core::ImmutablePolicyError);
}


DDSCXX_TEST_F(ddscxx_Topic, qos_nondefault_shift)
{
    this->CreateTopic();
    this->topic << this->lifespan_qos;
    ASSERT_EQ(this->topic.qos(), this->lifespan_qos);
    ASSERT_NE(this->topic.qos(), this->participant.default_topic_qos());
}


DDSCXX_TEST_F(ddscxx_Topic, qos_immutable_shift)
{
    this->CreateTopic();
    ASSERT_THROW({
        this->topic << this->reliable_qos;
    }, dds::core::ImmutablePolicyError);
}


DDSCXX_TEST_F(ddscxx_Topic, inconsistent_topic_status)
{
    this->CreateTopic();
    dds::core::status::InconsistentTopicStatus itStatus =
            this->topic.inconsistent_topic_status();
    ASSERT_EQ(itStatus.total_count(), 0);
    ASSERT_EQ(itStatus.total_count_change(), 0);
}


DDSCXX_TEST_F(ddscxx_Topic, close_busy)
{
    dds::sub::Subscriber sub = dds::core::null;
    dds::sub::DataReader<Space::Type1> reader = dds::core::null;

    this->CreateTopic();

    sub = dds::sub::Subscriber(this->participant);
    ASSERT_NE(sub, dds::core::null);
    reader = dds::sub::DataReader<Space::Type1>(sub, this->topic);
    ASSERT_NE(reader, dds::core::null);

    /* With a reader attached, the topic should not close. */
    ASSERT_THROW({
        this->topic.close();
    }, dds::core::PreconditionNotMetError);

    /* When deleted, the topic should be able to close. */
    reader = dds::core::null;
    this->topic.close();
}


DDSCXX_TEST_F(ddscxx_Topic, traits)
{
    ASSERT_NE(dds::topic::is_topic_type<Space::Type1>::value, 0);
    ASSERT_STREQ(dds::topic::topic_type_name<Space::Type1>::value().c_str(), "Space::Type1");
    ASSERT_NE(dds::topic::is_topic_type<Space::Type2>::value, 0);
    ASSERT_STREQ(dds::topic::topic_type_name<Space::Type2>::value().c_str(), "Space::Type2");
}


DDSCXX_TEST_F(ddscxx_Topic, use_after_close)
{
    /* Get a closed topic. */
    this->CreateTopic();
    this->topic.close();

    ASSERT_THROW({
        this->topic.close();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->topic.name();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->topic.type_name();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->topic.domain_participant();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->topic.qos(this->lifespan_qos);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->topic << this->lifespan_qos;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::topic::qos::TopicQos qos;
        qos = this->topic.qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::topic::qos::TopicQos qos;
        this->topic >> qos;
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        this->topic.inconsistent_topic_status();
    }, dds::core::AlreadyClosedError);
}


DDSCXX_TEST_F(ddscxx_Topic, use_after_deletion)
{
    /* Get a deleted topic. */
    this->CreateTopic();
    this->topic = dds::core::null;

    ASSERT_THROW({
        this->topic.close();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->topic.name();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->topic.type_name();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->topic.domain_participant();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->topic.qos(this->lifespan_qos);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->topic << this->lifespan_qos;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::topic::qos::TopicQos qos;
        qos = this->topic.qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::topic::qos::TopicQos qos;
        this->topic >> qos;
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        this->topic.inconsistent_topic_status();
    }, dds::core::NullReferenceError);
}
