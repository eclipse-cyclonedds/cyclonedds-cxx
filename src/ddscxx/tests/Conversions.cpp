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

#include "dds/dds.hpp"
#include "Space.hpp"

/*
 * For some reason, implicit casting can cause invalid castings on some
 * compilers (not all). For instance, dds::domain::DomainParticipant can
 * be implicitly cast to a dds::pub::Publisher. Obviously, this can cause
 * all kinds of problems. This should be investigated.
 * But for now, just know that it can happen on a few compilers and don't
 * test for those invalid castings by disabling the related tests.
 */

/**
 * Fixture for the Topic tests
 */
class Conversions : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::pub::Publisher publisher;
    dds::sub::Subscriber subscriber;
    dds::topic::Topic<Space::Type1> topic;
    dds::pub::DataWriter<Space::Type1> writer;
    dds::sub::DataReader<Space::Type1> reader;

    Conversions() :
        participant(dds::core::null),
        publisher(dds::core::null),
        subscriber(dds::core::null),
        topic(dds::core::null),
        writer(dds::core::null),
        reader(dds::core::null)
    {
    }

    void CreateParticipant() {
        if (this->participant == dds::core::null) {
            this->participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
            ASSERT_NE(this->participant, dds::core::null);
        }
    }

    void CreatePublisher() {
        if (this->publisher == dds::core::null) {
            this->CreateParticipant();
            this->publisher = dds::pub::Publisher(this->participant);
            ASSERT_NE(this->publisher, dds::core::null);
        }
    }

    void CreateSubscriber() {
        if (this->subscriber == dds::core::null) {
            this->CreateParticipant();
            this->subscriber = dds::sub::Subscriber(this->participant);
            ASSERT_NE(this->subscriber, dds::core::null);
        }
    }

    void CreateTopic() {
        if (this->topic == dds::core::null) {
            this->CreateParticipant();
            this->topic = dds::topic::Topic<Space::Type1>(this->participant, "converions_topic");
            ASSERT_NE(this->topic, dds::core::null);
        }
    }

    void CreateWriter() {
        if (this->writer == dds::core::null) {
            this->CreateTopic();
            this->CreatePublisher();
            this->writer = dds::pub::DataWriter<Space::Type1>(this->publisher, this->topic);
            ASSERT_NE(this->writer, dds::core::null);
        }
    }

    void CreateReader() {
        if (this->reader == dds::core::null) {
            this->CreateTopic();
            this->CreateSubscriber();
            this->reader = dds::sub::DataReader<Space::Type1>(this->subscriber, this->topic);
            ASSERT_NE(this->reader, dds::core::null);
        }
    }

    void SetUp() {
    }

    void TearDown() {
        topic = dds::core::null;
        participant = dds::core::null;
    }
};



/**
 * Tests
 */

TEST_F(Conversions, participant_entity_to)
{
    this->CreateParticipant();

    /* Constructor */
    {
        dds::core::Entity entity = dds::core::null;
        entity = dds::core::Entity(this->participant);
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->participant);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        entity = this->participant;
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->participant);
    }
}

TEST_F(Conversions, participant_entity_from)
{
    this->CreateParticipant();
    dds::core::Entity entity(this->participant);

    /* Constructor */
    {
        dds::domain::DomainParticipant par_from = dds::core::null;
        par_from = dds::domain::DomainParticipant(entity);
        ASSERT_NE(par_from, dds::core::null);
        ASSERT_EQ(par_from, this->participant);
    }

    /* Assignment */
    {
        dds::domain::DomainParticipant par_from = dds::core::null;
        par_from = entity;
        ASSERT_NE(par_from, dds::core::null);
        ASSERT_EQ(par_from, this->participant);
    }
}

TEST_F(Conversions, participant_entity_null)
{
    /* Constructor */
    {
        dds::domain::DomainParticipant par1(dds::core::null);
        ASSERT_EQ(par1, dds::core::null);

        dds::core::Entity entity(par1);
        ASSERT_EQ(entity, dds::core::null);

        dds::domain::DomainParticipant par2(entity);
        ASSERT_EQ(par2, dds::core::null);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        dds::domain::DomainParticipant par2 = dds::core::null;

        dds::domain::DomainParticipant par1 = dds::core::null;
        ASSERT_EQ(par1, dds::core::null);

        entity = par1;
        ASSERT_EQ(entity, dds::core::null);

        par2 = entity;
        ASSERT_EQ(par2, dds::core::null);
    }
}

TEST_F(Conversions, participant_entity_invalid)
{
    this->CreateParticipant();
    dds::core::Entity entity(this->participant);

    /* Constructor */
    ASSERT_THROW({
        dds::sub::Subscriber invalid_from(entity);
    }, dds::core::IllegalOperationError);

    /* Assignment */
    ASSERT_THROW({
        dds::pub::Publisher invalid_from = dds::core::null;
        invalid_from = entity;
    }, dds::core::IllegalOperationError);
}

/* Please see comment at top of the file why this test is disabled. */
TEST_F(Conversions, participant_entity_implicit)
{
    this->CreateParticipant();

    /* Test implicit conversions to entity and back. */
    const dds::core::Entity& e(this->participant);

    dds::domain::DomainParticipant par1(e);
    dds::domain::DomainParticipant par2 = e;
    dds::core::Entity ent1(e);
    dds::core::Entity ent2 = e;

    ASSERT_EQ(this->participant, par1);
    ASSERT_EQ(this->participant, par2);
    ASSERT_EQ(this->participant, ent1);
    ASSERT_EQ(this->participant, ent2);

    ASSERT_THROW({
        dds::pub::Publisher par3(e);
    }, dds::core::IllegalOperationError);
}

TEST_F(Conversions, publisher_entity_to)
{
    this->CreatePublisher();

    /* Constructor */
    {
        dds::core::Entity entity = dds::core::null;
        entity = dds::core::Entity(this->publisher);
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->publisher);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        entity = this->publisher;
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->publisher);
    }
}

TEST_F(Conversions, publisher_entity_from)
{
    this->CreatePublisher();
    dds::core::Entity entity(this->publisher);

    /* Constructor */
    {
        dds::pub::Publisher pub_from = dds::core::null;
        pub_from = dds::pub::Publisher(entity);
        ASSERT_NE(pub_from, dds::core::null);
        ASSERT_EQ(pub_from, this->publisher);
    }

    /* Assignment */
    {
        dds::pub::Publisher pub_from = dds::core::null;
        pub_from = entity;
        ASSERT_NE(pub_from, dds::core::null);
        ASSERT_EQ(pub_from, this->publisher);
    }
}

TEST_F(Conversions, publisher_entity_null)
{
    /* Constructor */
    {
        dds::pub::Publisher pub1(dds::core::null);
        ASSERT_EQ(pub1, dds::core::null);

        dds::core::Entity entity(pub1);
        ASSERT_EQ(entity, dds::core::null);

        dds::pub::Publisher pub2(entity);
        ASSERT_EQ(pub2, dds::core::null);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        dds::pub::Publisher pub2 = dds::core::null;

        dds::pub::Publisher pub1 = dds::core::null;
        ASSERT_EQ(pub1, dds::core::null);

        entity = pub1;
        ASSERT_EQ(entity, dds::core::null);

        pub2 = entity;
        ASSERT_EQ(pub2, dds::core::null);
    }
}

TEST_F(Conversions, publisher_entity_invalid)
{
    this->CreatePublisher();
    dds::core::Entity entity(this->publisher);

    /* Constructor */
    ASSERT_THROW({
        dds::sub::Subscriber invalid_from(entity);
    }, dds::core::IllegalOperationError);

    /* Assignment */
    ASSERT_THROW({
        dds::sub::Subscriber invalid_from = dds::core::null;
        invalid_from = entity;
    }, dds::core::IllegalOperationError);
}

/* Please see comment at top of the file why this test is disabled. */
TEST_F(Conversions, publisher_entity_implicit)
{
    this->CreatePublisher();

    /* Test implicit conversions to entity and back. */
    const dds::core::Entity& e(this->publisher);

    dds::pub::Publisher pub1(e);
    dds::pub::Publisher pub2 = e;
    dds::core::Entity ent1(e);
    dds::core::Entity ent2 = e;

    ASSERT_EQ(this->publisher, pub1);
    ASSERT_EQ(this->publisher, pub2);
    ASSERT_EQ(this->publisher, ent1);
    ASSERT_EQ(this->publisher, ent2);

    ASSERT_THROW({
        dds::sub::Subscriber pub3(e);
    }, dds::core::IllegalOperationError);
}

TEST_F(Conversions, subscriber_entity_to)
{
    this->CreateSubscriber();

    /* Constructor */
    {
        dds::core::Entity entity = dds::core::null;
        entity = dds::core::Entity(this->subscriber);
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->subscriber);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        entity = this->subscriber;
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->subscriber);
    }
}

TEST_F(Conversions, subscriber_entity_from)
{
    this->CreateSubscriber();
    dds::core::Entity entity(this->subscriber);

    /* Constructor */
    {
        dds::sub::Subscriber sub_from = dds::core::null;
        sub_from = dds::sub::Subscriber(entity);
        ASSERT_NE(sub_from, dds::core::null);
        ASSERT_EQ(sub_from, this->subscriber);
    }

    /* Assignment */
    {
        dds::sub::Subscriber sub_from = dds::core::null;
        sub_from = entity;
        ASSERT_NE(sub_from, dds::core::null);
        ASSERT_EQ(sub_from, this->subscriber);
    }
}

TEST_F(Conversions, subscriber_entity_null)
{
    /* Constructor */
    {
        dds::sub::Subscriber sub1(dds::core::null);
        ASSERT_EQ(sub1, dds::core::null);

        dds::core::Entity entity(sub1);
        ASSERT_EQ(entity, dds::core::null);

        dds::sub::Subscriber sub2(entity);
        ASSERT_EQ(sub2, dds::core::null);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        dds::sub::Subscriber sub2 = dds::core::null;

        dds::sub::Subscriber sub1 = dds::core::null;
        ASSERT_EQ(sub1, dds::core::null);

        entity = sub1;
        ASSERT_EQ(entity, dds::core::null);

        sub2 = entity;
        ASSERT_EQ(sub2, dds::core::null);
    }
}

TEST_F(Conversions, subscriber_entity_invalid)
{
    this->CreateSubscriber();
    dds::core::Entity entity(this->subscriber);

    /* Constructor */
    ASSERT_THROW({
        dds::pub::Publisher invalid_from(entity);
    }, dds::core::IllegalOperationError);

    /* Assignment */
    ASSERT_THROW({
        dds::pub::Publisher invalid_from = dds::core::null;
        invalid_from = entity;
    }, dds::core::IllegalOperationError);
}

/* Please see comment at top of the file why this test is disabled. */
TEST_F(Conversions, subscriber_entity_implicit)
{
    this->CreateSubscriber();

    /* Test implicit conversions to entity and back. */
    const dds::core::Entity& e(this->subscriber);

    dds::sub::Subscriber sub1(e);
    dds::sub::Subscriber sub2 = e;
    dds::core::Entity ent1(e);
    dds::core::Entity ent2 = e;

    ASSERT_EQ(this->subscriber, sub1);
    ASSERT_EQ(this->subscriber, sub2);
    ASSERT_EQ(this->subscriber, ent1);
    ASSERT_EQ(this->subscriber, ent2);

    ASSERT_THROW({
        dds::pub::Publisher sub3(e);
    }, dds::core::IllegalOperationError);
}

TEST_F(Conversions, topic_any_to)
{
    this->CreateTopic();

    /* Constructor */
    {
        dds::topic::AnyTopic any = dds::core::null;
        any = dds::topic::AnyTopic(this->topic);
        ASSERT_NE(any, dds::core::null);
        ASSERT_EQ(any, this->topic);
    }

    /* Assignment */
    {
        dds::topic::AnyTopic any = dds::core::null;
        any = this->topic;
        ASSERT_NE(any, dds::core::null);
        ASSERT_EQ(any, this->topic);
    }
}

TEST_F(Conversions, topic_any_from)
{
    this->CreateTopic();
    dds::topic::AnyTopic any(this->topic);

    /* Constructor */
    {
        dds::topic::Topic<Space::Type1> topic_from = dds::core::null;
        topic_from = dds::topic::Topic<Space::Type1>(any);
        ASSERT_NE(topic_from, dds::core::null);
        ASSERT_EQ(topic_from, this->topic);
    }

    /* Assignment */
    {
        dds::topic::Topic<Space::Type1> topic_from = dds::core::null;
        topic_from = any;
        ASSERT_NE(topic_from, dds::core::null);
        ASSERT_EQ(topic_from, this->topic);
    }
}

TEST_F(Conversions, topic_any_null)
{
    /* Constructor */
    {
        dds::topic::Topic<Space::Type1> tpc1(dds::core::null);
        ASSERT_EQ(tpc1, dds::core::null);

        dds::topic::AnyTopic any(tpc1);
        ASSERT_EQ(any, dds::core::null);

        dds::topic::Topic<Space::Type1> tpc2(any);
        ASSERT_EQ(tpc2, dds::core::null);
    }

    /* Assignment */
    {
        dds::topic::AnyTopic any = dds::core::null;
        dds::topic::Topic<Space::Type1> tpc2 = dds::core::null;

        dds::topic::Topic<Space::Type1> tpc1 = dds::core::null;
        ASSERT_EQ(tpc1, dds::core::null);

        any = tpc1;
        ASSERT_EQ(any, dds::core::null);

        tpc2 = any;
        ASSERT_EQ(tpc2, dds::core::null);
    }
}

TEST_F(Conversions, topic_any_invalid)
{
    this->CreateTopic();
    dds::topic::AnyTopic any(this->topic);

    /* Constructor */
    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> topic_from(any);
    }, dds::core::IllegalOperationError);

    /* Assignment */
    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> topic_from = dds::core::null;
        topic_from = any;
    }, dds::core::IllegalOperationError);
}

/* Please see comment at top of the file why this test is disabled. */
TEST_F(Conversions, topic_any_implicit)
{
    this->CreateTopic();

    /* Test implicit conversions to anytopic and back. */
    const dds::topic::AnyTopic& tp(this->topic);

    dds::topic::Topic<Space::Type1> tpc1(tp);
    dds::topic::Topic<Space::Type1> tpc2 = tp;
    dds::topic::AnyTopic any1(tp);
    dds::topic::AnyTopic any2 = tp;

    ASSERT_EQ(this->topic, tpc1);
    ASSERT_EQ(this->topic, tpc2);
    ASSERT_EQ(this->topic, any1);
    ASSERT_EQ(this->topic, any2);

    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> tpc3 = tp;
    }, dds::core::IllegalOperationError);
}

TEST_F(Conversions, topic_description_to)
{
    this->CreateTopic();
    //dds::topic::TopicDescription
    /* Constructor */
    {
        dds::topic::TopicDescription desc = dds::core::null;
        desc = dds::topic::AnyTopic(this->topic);
        ASSERT_NE(desc, dds::core::null);
        ASSERT_EQ(desc, this->topic);
    }

    /* Assignment */
    {
        dds::topic::TopicDescription desc = dds::core::null;
        desc = this->topic;
        ASSERT_NE(desc, dds::core::null);
        ASSERT_EQ(desc, this->topic);
    }
}

TEST_F(Conversions, topic_description_from)
{
    this->CreateTopic();
    dds::topic::TopicDescription desc(this->topic);

    /* Constructor */
    {
        dds::topic::Topic<Space::Type1> topic_from = dds::core::null;
        topic_from = dds::topic::Topic<Space::Type1>(desc);
        ASSERT_NE(topic_from, dds::core::null);
        ASSERT_EQ(topic_from, this->topic);
    }

    /* Assignment */
    {
        dds::topic::Topic<Space::Type1> topic_from = dds::core::null;
        topic_from = desc;
        ASSERT_NE(topic_from, dds::core::null);
        ASSERT_EQ(topic_from, this->topic);
    }
}

TEST_F(Conversions, topic_description_null)
{
    /* Constructor */
    {
        dds::topic::Topic<Space::Type1> tpc1(dds::core::null);
        ASSERT_EQ(tpc1, dds::core::null);

        dds::topic::TopicDescription desc(tpc1);
        ASSERT_EQ(desc, dds::core::null);

        dds::topic::Topic<Space::Type1> tpc2(desc);
        ASSERT_EQ(tpc2, dds::core::null);
    }

    /* Assignment */
    {
        dds::topic::TopicDescription desc = dds::core::null;
        dds::topic::Topic<Space::Type1> tpc2 = dds::core::null;

        dds::topic::Topic<Space::Type1> tpc1 = dds::core::null;
        ASSERT_EQ(tpc1, dds::core::null);

        desc = tpc1;
        ASSERT_EQ(desc, dds::core::null);

        tpc2 = desc;
        ASSERT_EQ(tpc2, dds::core::null);
    }
}

TEST_F(Conversions, topic_description_invalid)
{
    this->CreateTopic();
    dds::topic::TopicDescription desc(this->topic);

    /* Constructor */
    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> topic_from(desc);
    }, dds::core::IllegalOperationError);

    /* Assignment */
    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> topic_from = dds::core::null;
        topic_from = desc;
    }, dds::core::IllegalOperationError);
}

/* Please see comment at top of the file why this test is disabled. */
TEST_F(Conversions, topic_description_implicit)
{
    this->CreateTopic();

    /* Test implicit conversions to topicdescription and back. */
    const dds::topic::TopicDescription& td(this->topic);

    dds::topic::Topic<Space::Type1> tpc1(td);
    dds::topic::Topic<Space::Type1> tpc2 = td;
    dds::topic::TopicDescription desc1(td);
    dds::topic::TopicDescription desc2 = td;

    ASSERT_EQ(this->topic, tpc1);
    ASSERT_EQ(this->topic, tpc2);
    ASSERT_EQ(this->topic, desc1);
    ASSERT_EQ(this->topic, desc2);

    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> tpc3 = td;
    }, dds::core::IllegalOperationError);
}

TEST_F(Conversions, topic_entity_to)
{
    this->CreateTopic();

    /* Constructor */
    {
        dds::core::Entity entity = dds::core::null;
        entity = dds::core::Entity(this->topic);
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->topic);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        entity = this->topic;
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->topic);
    }
}

TEST_F(Conversions, topic_entity_from)
{
    this->CreateTopic();
    dds::core::Entity entity(this->topic);

    /* Constructor */
    {
        dds::topic::Topic<Space::Type1> topic_from = dds::core::null;
        topic_from = dds::topic::Topic<Space::Type1>(entity);
        ASSERT_NE(topic_from, dds::core::null);
        ASSERT_EQ(topic_from, this->topic);
    }

    /* Assignment */
    {
        dds::topic::Topic<Space::Type1> topic_from = dds::core::null;
        topic_from = entity;
        ASSERT_NE(topic_from, dds::core::null);
        ASSERT_EQ(topic_from, this->topic);
    }
}

TEST_F(Conversions, topic_entity_null)
{
    /* Constructor */
    {
        dds::topic::Topic<Space::Type1> tpc1(dds::core::null);
        ASSERT_EQ(tpc1, dds::core::null);

        dds::core::Entity entity(tpc1);
        ASSERT_EQ(entity, dds::core::null);

        dds::topic::Topic<Space::Type1> tpc2(entity);
        ASSERT_EQ(tpc2, dds::core::null);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        dds::topic::Topic<Space::Type1> tpc2 = dds::core::null;

        dds::topic::Topic<Space::Type1> tpc1 = dds::core::null;
        ASSERT_EQ(tpc1, dds::core::null);

        entity = tpc1;
        ASSERT_EQ(entity, dds::core::null);

        tpc2 = entity;
        ASSERT_EQ(tpc2, dds::core::null);
    }
}

TEST_F(Conversions, topic_entity_invalid)
{
    this->CreateTopic();
    dds::core::Entity entity(this->topic);

    /* Constructor */
    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> topic_from(entity);
    }, dds::core::IllegalOperationError);

    /* Assignment */
    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> topic_from = dds::core::null;
        topic_from = entity;
    }, dds::core::IllegalOperationError);
}

/* Please see comment at top of the file why this test is disabled. */
TEST_F(Conversions, topic_entity_implicit)
{
    this->CreateTopic();

    /* Test implicit conversions to entity and back. */
    const dds::core::Entity& e(this->topic);

    dds::topic::Topic<Space::Type1> tpc1(e);
    dds::topic::Topic<Space::Type1> tpc2 = e;
    dds::core::Entity ent1(e);
    dds::core::Entity ent2 = e;

    ASSERT_EQ(this->topic, tpc1);
    ASSERT_EQ(this->topic, tpc2);
    ASSERT_EQ(this->topic, ent1);
    ASSERT_EQ(this->topic, ent2);

    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> tpc3 = e;
    }, dds::core::IllegalOperationError);
}

TEST_F(Conversions, writer_any_to)
{
    this->CreateWriter();

    /* Constructor */
    {
        dds::pub::AnyDataWriter any = dds::core::null;
        any = dds::pub::AnyDataWriter(this->writer);
        ASSERT_NE(any, dds::core::null);
        ASSERT_EQ(any, this->writer);
    }

    /* Assignment */
    {
        dds::pub::AnyDataWriter any = dds::core::null;
        any = this->writer;
        ASSERT_NE(any, dds::core::null);
        ASSERT_EQ(any, this->writer);
    }
}

TEST_F(Conversions, writer_any_from)
{
    this->CreateWriter();
    dds::pub::AnyDataWriter any(this->writer);

    /* Constructor */
    {
        dds::pub::DataWriter<Space::Type1> writer_from = dds::core::null;
        writer_from = dds::pub::DataWriter<Space::Type1>(any);
        ASSERT_NE(writer_from, dds::core::null);
        ASSERT_EQ(writer_from, this->writer);
    }

    /* Assignment */
    {
        dds::pub::DataWriter<Space::Type1> writer_from = dds::core::null;
        writer_from = any;
        ASSERT_NE(writer_from, dds::core::null);
        ASSERT_EQ(writer_from, this->writer);
    }
}

TEST_F(Conversions, writer_any_null)
{
    /* Constructor */
    {
        dds::pub::DataWriter<Space::Type1> wrt1(dds::core::null);
        ASSERT_EQ(wrt1, dds::core::null);

        dds::pub::AnyDataWriter any(wrt1);
        ASSERT_EQ(any, dds::core::null);

        dds::pub::DataWriter<Space::Type1> wrt2(any);
        ASSERT_EQ(wrt2, dds::core::null);
    }

    /* Assignment */
    {
        dds::pub::AnyDataWriter any = dds::core::null;
        dds::pub::DataWriter<Space::Type1> wrt2 = dds::core::null;

        dds::pub::DataWriter<Space::Type1> wrt1 = dds::core::null;
        ASSERT_EQ(wrt1, dds::core::null);

        any = wrt1;
        ASSERT_EQ(any, dds::core::null);

        wrt2 = any;
        ASSERT_EQ(wrt2, dds::core::null);
    }
}

TEST_F(Conversions, writer_any_invalid)
{
    this->CreateWriter();
    dds::pub::AnyDataWriter any(this->writer);

    /* Constructor */
    ASSERT_THROW({
        dds::pub::DataWriter<Space::Type2> writer_from(any);
    }, dds::core::IllegalOperationError);

    /* Assignment */
    ASSERT_THROW({
        dds::pub::DataWriter<Space::Type2> writer_from = dds::core::null;
        writer_from = any;
    }, dds::core::IllegalOperationError);
}

/* Please see comment at top of the file why this test is disabled. */
TEST_F(Conversions, writer_any_implicit)
{
    this->CreateWriter();

    /* Test implicit conversions to anydatawriter and back. */
    const dds::pub::AnyDataWriter& wr(this->writer);

    dds::pub::DataWriter<Space::Type1> wrt1(wr);
    dds::pub::DataWriter<Space::Type1> wrt2 = wr;
    dds::pub::AnyDataWriter any1(wr);
    dds::pub::AnyDataWriter any2 = wr;

    ASSERT_EQ(this->writer, wrt1);
    ASSERT_EQ(this->writer, wrt2);
    ASSERT_EQ(this->writer, any1);
    ASSERT_EQ(this->writer, any2);

    ASSERT_THROW({
        dds::pub::DataWriter<Space::Type2> tpc3 = wr;
    }, dds::core::IllegalOperationError);
}

TEST_F(Conversions, writer_entity_to)
{
    this->CreateWriter();

    /* Constructor */
    {
        dds::core::Entity entity = dds::core::null;
        entity = dds::core::Entity(this->writer);
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->writer);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        entity = this->writer;
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->writer);
    }
}

TEST_F(Conversions, writer_entity_from)
{
    this->CreateWriter();
    dds::core::Entity entity(this->writer);

    /* Constructor */
    {
        dds::pub::DataWriter<Space::Type1> writer_from = dds::core::null;
        writer_from = dds::pub::DataWriter<Space::Type1>(entity);
        ASSERT_NE(writer_from, dds::core::null);
        ASSERT_EQ(writer_from, this->writer);
    }

    /* Assignment */
    {
        dds::pub::DataWriter<Space::Type1> writer_from = dds::core::null;
        writer_from = entity;
        ASSERT_NE(writer_from, dds::core::null);
        ASSERT_EQ(writer_from, this->writer);
    }
}

TEST_F(Conversions, writer_entity_null)
{
    /* Constructor */
    {
        dds::pub::DataWriter<Space::Type1> wrt1(dds::core::null);
        ASSERT_EQ(wrt1, dds::core::null);

        dds::core::Entity entity(wrt1);
        ASSERT_EQ(entity, dds::core::null);

        dds::pub::DataWriter<Space::Type1> wrt2(entity);
        ASSERT_EQ(wrt2, dds::core::null);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        dds::pub::DataWriter<Space::Type1> wrt2 = dds::core::null;

        dds::pub::DataWriter<Space::Type1> wrt1 = dds::core::null;
        ASSERT_EQ(wrt1, dds::core::null);

        entity = wrt1;
        ASSERT_EQ(entity, dds::core::null);

        wrt2 = entity;
        ASSERT_EQ(wrt2, dds::core::null);
    }
}

TEST_F(Conversions, writer_entity_invalid)
{
    this->CreateWriter();
    dds::core::Entity entity(this->writer);

    /* Constructor */
    ASSERT_THROW({
        dds::pub::DataWriter<Space::Type2> writer_from(entity);
    }, dds::core::IllegalOperationError);

    /* Assignment */
    ASSERT_THROW({
        dds::pub::DataWriter<Space::Type2> writer_from = dds::core::null;
        writer_from = entity;
    }, dds::core::IllegalOperationError);
}

/* Please see comment at top of the file why this test is disabled. */
TEST_F(Conversions, writer_entity_implicit)
{
    this->CreateWriter();

    /* Test implicit conversions to entity and back. */
    const dds::core::Entity& e(this->writer);

    dds::pub::DataWriter<Space::Type1> wrt1(e);
    dds::pub::DataWriter<Space::Type1> wrt2 = e;
    dds::core::Entity ent1(e);
    dds::core::Entity ent2 = e;

    ASSERT_EQ(this->writer, wrt1);
    ASSERT_EQ(this->writer, wrt2);
    ASSERT_EQ(this->writer, ent1);
    ASSERT_EQ(this->writer, ent2);

    ASSERT_THROW({
        dds::pub::DataWriter<Space::Type2> tpc3 = e;
    }, dds::core::IllegalOperationError);
}

TEST_F(Conversions, reader_any_to)
{
    this->CreateReader();

    /* Constructor */
    {
        dds::sub::AnyDataReader any = dds::core::null;
        any = dds::sub::AnyDataReader(this->reader);
        ASSERT_NE(any, dds::core::null);
        ASSERT_EQ(any, this->reader);
    }

    /* Assignment */
    {
        dds::sub::AnyDataReader any = dds::core::null;
        any = this->reader;
        ASSERT_NE(any, dds::core::null);
        ASSERT_EQ(any, this->reader);
    }
}

TEST_F(Conversions, reader_any_from)
{
    this->CreateReader();
    dds::sub::AnyDataReader any(this->reader);

    /* Constructor */
    {
        dds::sub::DataReader<Space::Type1> reader_from = dds::core::null;
        reader_from = dds::sub::DataReader<Space::Type1>(any);
        ASSERT_NE(reader_from, dds::core::null);
        ASSERT_EQ(reader_from, this->reader);
    }

    /* Assignment */
    {
        dds::sub::DataReader<Space::Type1> reader_from = dds::core::null;
        reader_from = any;
        ASSERT_NE(reader_from, dds::core::null);
        ASSERT_EQ(reader_from, this->reader);
    }
}

TEST_F(Conversions, reader_any_null)
{
    /* Constructor */
    {
        dds::sub::DataReader<Space::Type1> wrt1(dds::core::null);
        ASSERT_EQ(wrt1, dds::core::null);

        dds::sub::AnyDataReader any(wrt1);
        ASSERT_EQ(any, dds::core::null);

        dds::sub::DataReader<Space::Type1> wrt2(any);
        ASSERT_EQ(wrt2, dds::core::null);
    }

    /* Assignment */
    {
        dds::sub::AnyDataReader any = dds::core::null;
        dds::sub::DataReader<Space::Type1> wrt2 = dds::core::null;

        dds::sub::DataReader<Space::Type1> wrt1 = dds::core::null;
        ASSERT_EQ(wrt1, dds::core::null);

        any = wrt1;
        ASSERT_EQ(any, dds::core::null);

        wrt2 = any;
        ASSERT_EQ(wrt2, dds::core::null);
    }
}

TEST_F(Conversions, reader_any_invalid)
{
    this->CreateReader();
    dds::sub::AnyDataReader any(this->reader);

    /* Constructor */
    ASSERT_THROW({
        dds::sub::DataReader<Space::Type2> reader_from(any);
    }, dds::core::IllegalOperationError);

    /* Assignment */
    ASSERT_THROW({
        dds::sub::DataReader<Space::Type2> reader_from = dds::core::null;
        reader_from = any;
    }, dds::core::IllegalOperationError);
}

/* Please see comment at top of the file why this test is disabled. */
TEST_F(Conversions, reader_any_implicit)
{
    this->CreateReader();

    /* Test implicit conversions to anydatareader and back. */
    const dds::sub::AnyDataReader& rd(this->reader);

    dds::sub::DataReader<Space::Type1> wrt1(rd);
    dds::sub::DataReader<Space::Type1> wrt2 = rd;
    dds::sub::AnyDataReader any1(rd);
    dds::sub::AnyDataReader any2 = rd;

    ASSERT_EQ(this->reader, wrt1);
    ASSERT_EQ(this->reader, wrt2);
    ASSERT_EQ(this->reader, any1);
    ASSERT_EQ(this->reader, any2);

    ASSERT_THROW({
        dds::sub::DataReader<Space::Type2> tpc3 = rd;
    }, dds::core::IllegalOperationError);
}

TEST_F(Conversions, reader_entity_to)
{
    this->CreateReader();

    /* Constructor */
    {
        dds::core::Entity entity = dds::core::null;
        entity = dds::core::Entity(this->reader);
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->reader);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        entity = this->reader;
        ASSERT_NE(entity, dds::core::null);
        ASSERT_EQ(entity, this->reader);
    }
}

TEST_F(Conversions, reader_entity_from)
{
    this->CreateReader();
    dds::core::Entity entity(this->reader);

    /* Constructor */
    {
        dds::sub::DataReader<Space::Type1> reader_from = dds::core::null;
        reader_from = dds::sub::DataReader<Space::Type1>(entity);
        ASSERT_NE(reader_from, dds::core::null);
        ASSERT_EQ(reader_from, this->reader);
    }

    /* Assignment */
    {
        dds::sub::DataReader<Space::Type1> reader_from = dds::core::null;
        reader_from = entity;
        ASSERT_NE(reader_from, dds::core::null);
        ASSERT_EQ(reader_from, this->reader);
    }
}

TEST_F(Conversions, reader_entity_null)
{
    /* Constructor */
    {
        dds::sub::DataReader<Space::Type1> wrt1(dds::core::null);
        ASSERT_EQ(wrt1, dds::core::null);

        dds::core::Entity entity(wrt1);
        ASSERT_EQ(entity, dds::core::null);

        dds::sub::DataReader<Space::Type1> wrt2(entity);
        ASSERT_EQ(wrt2, dds::core::null);
    }

    /* Assignment */
    {
        dds::core::Entity entity = dds::core::null;
        dds::sub::DataReader<Space::Type1> wrt2 = dds::core::null;

        dds::sub::DataReader<Space::Type1> wrt1 = dds::core::null;
        ASSERT_EQ(wrt1, dds::core::null);

        entity = wrt1;
        ASSERT_EQ(entity, dds::core::null);

        wrt2 = entity;
        ASSERT_EQ(wrt2, dds::core::null);
    }
}

TEST_F(Conversions, reader_entity_invalid)
{
    this->CreateReader();
    dds::core::Entity entity(this->reader);

    /* Constructor */
    ASSERT_THROW({
        dds::sub::DataReader<Space::Type2> reader_from(entity);
    }, dds::core::IllegalOperationError);

    /* Assignment */
    ASSERT_THROW({
        dds::sub::DataReader<Space::Type2> reader_from = dds::core::null;
        reader_from = entity;
    }, dds::core::IllegalOperationError);
}

/* Please see comment at top of the file why this test is disabled. */
TEST_F(Conversions, reader_entity_implicit)
{
    this->CreateReader();

    /* Test implicit conversions to entity and back. */
    const dds::core::Entity& e(this->reader);

    dds::sub::DataReader<Space::Type1> wrt1(e);
    dds::sub::DataReader<Space::Type1> wrt2 = e;
    dds::core::Entity ent1(e);
    dds::core::Entity ent2 = e;

    ASSERT_EQ(this->reader, wrt1);
    ASSERT_EQ(this->reader, wrt2);
    ASSERT_EQ(this->reader, ent1);
    ASSERT_EQ(this->reader, ent2);

    ASSERT_THROW({
        dds::sub::DataReader<Space::Type2> tpc3 = e;
    }, dds::core::IllegalOperationError);
}
