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
#include "dds/ddsrt/environ.h"
#include "Space_DCPS.hpp"


/*
 * For some reason, implicit casting can cause invalid castings on some
 * compilers (not all). For instance, dds::domain::DomainParticipant can
 * be implicitly cast to a dds::pub::Publisher. Obviously, this can cause
 * all kinds of problems. This should be investigated.
 * But for now, just know that it can happen on a few compilers and don't
 * test for those invalid castings by disabling the related tests.
 */

/* Class to test implicit conversion from DataWriter<T> to AnyDataWriter and vice versa. */
class AnyWriterStore
{
public:
    AnyWriterStore(const dds::pub::AnyDataWriter& base)
        : any(base)
    {
    }
    const dds::pub::AnyDataWriter& get() {
        return any;
    }
private:
    const dds::pub::AnyDataWriter& any;
};

/* Class to test implicit conversion from DataReader<T> to AnyDataReader and vice versa. */
class AnyReaderStore
{
public:
    AnyReaderStore(const dds::sub::AnyDataReader& base)
        : any(base)
    {
    }
    const dds::sub::AnyDataReader& get() {
        return any;
    }
private:
    const dds::sub::AnyDataReader& any;
};

/* Class to test implicit conversion from Topic<T> to AnyTopic and vice versa. */
class AnyTopicStore
{
public:
    AnyTopicStore(const dds::topic::AnyTopic& base)
        : any(base)
    {
    }
    const dds::topic::AnyTopic& get() {
        return any;
    }
private:
    const dds::topic::AnyTopic& any;
};

/* Class to test implicit conversion from Topic<T> to TopicDescription and vice versa. */
class TopicDescriptionStore
{
public:
    TopicDescriptionStore(const dds::topic::TopicDescription& base)
        : any(base)
    {
    }
    const dds::topic::TopicDescription& get() {
        return any;
    }
private:
    const dds::topic::TopicDescription& any;
};

/* Class to test implicit conversion from DataReader<T> to Entity and vice versa. */
class EntityStore
{
public:
    EntityStore(const dds::core::Entity& e)
        : entity(e)
    {
    }
    const dds::core::Entity& get() {
        return entity;
    }
private:
    const dds::core::Entity& entity;
};




/**
 * Fixture for the Topic tests
 */
class ddscxx_Conversions : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::pub::Publisher publisher;
    dds::sub::Subscriber subscriber;
    dds::topic::Topic<Space::Type1> topic;
    dds::pub::DataWriter<Space::Type1> writer;
    dds::sub::DataReader<Space::Type1> reader;

    ddscxx_Conversions() :
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

DDSCXX_TEST_F(ddscxx_Conversions, participant_entity_to)
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


DDSCXX_TEST_F(ddscxx_Conversions, participant_entity_from)
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


DDSCXX_TEST_F(ddscxx_Conversions, participant_entity_null)
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


DDSCXX_TEST_F(ddscxx_Conversions, participant_entity_invalid)
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
DDSCXX_TEST_F(ddscxx_Conversions, DISABLED_participant_entity_implicit)
{
    this->CreateParticipant();

    /* Test implicit conversions by using a store. */
    EntityStore store(this->participant);

    dds::domain::DomainParticipant par1(store.get());
    dds::domain::DomainParticipant par2 = store.get();
    dds::core::Entity ent1(store.get());
    dds::core::Entity ent2 = store.get();

    ASSERT_EQ(this->participant, par1);
    ASSERT_EQ(this->participant, par2);
    ASSERT_EQ(this->participant, ent1);
    ASSERT_EQ(this->participant, ent2);

    ASSERT_THROW({
        dds::pub::Publisher par3(store.get());
    }, dds::core::IllegalOperationError);
}


DDSCXX_TEST_F(ddscxx_Conversions, publisher_entity_to)
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


DDSCXX_TEST_F(ddscxx_Conversions, publisher_entity_from)
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


DDSCXX_TEST_F(ddscxx_Conversions, publisher_entity_null)
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


DDSCXX_TEST_F(ddscxx_Conversions, publisher_entity_invalid)
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
DDSCXX_TEST_F(ddscxx_Conversions, DISABLED_publisher_entity_implicit)
{
    this->CreatePublisher();

    /* Test implicit conversions by using a store. */
    EntityStore store(this->publisher);

    dds::pub::Publisher pub1(store.get());
    dds::pub::Publisher pub2 = store.get();
    dds::core::Entity ent1(store.get());
    dds::core::Entity ent2 = store.get();

    ASSERT_EQ(this->publisher, pub1);
    ASSERT_EQ(this->publisher, pub2);
    ASSERT_EQ(this->publisher, ent1);
    ASSERT_EQ(this->publisher, ent2);

    ASSERT_THROW({
        dds::pub::Publisher pub3(store.get());
    }, dds::core::IllegalOperationError);
}


DDSCXX_TEST_F(ddscxx_Conversions, subscriber_entity_to)
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


DDSCXX_TEST_F(ddscxx_Conversions, subscriber_entity_from)
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


DDSCXX_TEST_F(ddscxx_Conversions, subscriber_entity_null)
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


DDSCXX_TEST_F(ddscxx_Conversions, subscriber_entity_invalid)
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
DDSCXX_TEST_F(ddscxx_Conversions, DISABLED_subscriber_entity_implicit)
{
    this->CreateSubscriber();

    /* Test implicit conversions by using a store. */
    EntityStore store(this->subscriber);

    dds::sub::Subscriber sub1(store.get());
    dds::sub::Subscriber sub2 = store.get();
    dds::core::Entity ent1(store.get());
    dds::core::Entity ent2 = store.get();

    ASSERT_EQ(this->subscriber, sub1);
    ASSERT_EQ(this->subscriber, sub2);
    ASSERT_EQ(this->subscriber, ent1);
    ASSERT_EQ(this->subscriber, ent2);

    ASSERT_THROW({
        dds::sub::Subscriber sub3(store.get());
    }, dds::core::IllegalOperationError);
}


DDSCXX_TEST_F(ddscxx_Conversions, topic_any_to)
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


DDSCXX_TEST_F(ddscxx_Conversions, topic_any_from)
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


DDSCXX_TEST_F(ddscxx_Conversions, topic_any_null)
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


DDSCXX_TEST_F(ddscxx_Conversions, topic_any_invalid)
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
DDSCXX_TEST_F(ddscxx_Conversions, DISABLED_topic_any_implicit)
{
    this->CreateTopic();

    /* Test implicit conversions by using a store. */
    AnyTopicStore store(this->topic);

    dds::topic::Topic<Space::Type1> tpc1(store.get());
    dds::topic::Topic<Space::Type1> tpc2 = store.get();
    dds::topic::AnyTopic any1(store.get());
    dds::topic::AnyTopic any2 = store.get();

    ASSERT_EQ(this->topic, tpc1);
    ASSERT_EQ(this->topic, tpc2);
    ASSERT_EQ(this->topic, any1);
    ASSERT_EQ(this->topic, any2);

    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> tpc3 = store.get();
    }, dds::core::IllegalOperationError);
}


DDSCXX_TEST_F(ddscxx_Conversions, topic_description_to)
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


DDSCXX_TEST_F(ddscxx_Conversions, topic_description_from)
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


DDSCXX_TEST_F(ddscxx_Conversions, topic_description_null)
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


DDSCXX_TEST_F(ddscxx_Conversions, topic_description_invalid)
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
DDSCXX_TEST_F(ddscxx_Conversions, DISABLED_topic_description_implicit)
{
    this->CreateTopic();

    /* Test implicit conversions by using a store. */
    TopicDescriptionStore store(this->topic);

    dds::topic::Topic<Space::Type1> tpc1(store.get());
    dds::topic::Topic<Space::Type1> tpc2 = store.get();
    dds::topic::TopicDescription desc1(store.get());
    dds::topic::TopicDescription desc2 = store.get();

    ASSERT_EQ(this->topic, tpc1);
    ASSERT_EQ(this->topic, tpc2);
    ASSERT_EQ(this->topic, desc1);
    ASSERT_EQ(this->topic, desc2);

    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> tpc3 = store.get();
    }, dds::core::IllegalOperationError);
}


DDSCXX_TEST_F(ddscxx_Conversions, topic_entity_to)
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


DDSCXX_TEST_F(ddscxx_Conversions, topic_entity_from)
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


DDSCXX_TEST_F(ddscxx_Conversions, topic_entity_null)
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


DDSCXX_TEST_F(ddscxx_Conversions, topic_entity_invalid)
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
DDSCXX_TEST_F(ddscxx_Conversions, DISABLED_topic_entity_implicit)
{
    this->CreateTopic();

    /* Test implicit conversions by using a store. */
    EntityStore store(this->topic);

    dds::topic::Topic<Space::Type1> tpc1(store.get());
    dds::topic::Topic<Space::Type1> tpc2 = store.get();
    dds::core::Entity ent1(store.get());
    dds::core::Entity ent2 = store.get();

    ASSERT_EQ(this->topic, tpc1);
    ASSERT_EQ(this->topic, tpc2);
    ASSERT_EQ(this->topic, ent1);
    ASSERT_EQ(this->topic, ent2);

    ASSERT_THROW({
        dds::topic::Topic<Space::Type2> tpc3 = store.get();
    }, dds::core::IllegalOperationError);
}


DDSCXX_TEST_F(ddscxx_Conversions, writer_any_to)
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


DDSCXX_TEST_F(ddscxx_Conversions, writer_any_from)
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


DDSCXX_TEST_F(ddscxx_Conversions, writer_any_null)
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


DDSCXX_TEST_F(ddscxx_Conversions, writer_any_invalid)
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
DDSCXX_TEST_F(ddscxx_Conversions, DISABLED_writer_any_implicit)
{
    this->CreateWriter();

    /* Test implicit conversions by using a store. */
    AnyWriterStore store(this->writer);

    dds::pub::DataWriter<Space::Type1> wrt1(store.get());
    dds::pub::DataWriter<Space::Type1> wrt2 = store.get();
    dds::pub::AnyDataWriter any1(store.get());
    dds::pub::AnyDataWriter any2 = store.get();

    ASSERT_EQ(this->writer, wrt1);
    ASSERT_EQ(this->writer, wrt2);
    ASSERT_EQ(this->writer, any1);
    ASSERT_EQ(this->writer, any2);

    ASSERT_THROW({
        dds::pub::DataWriter<Space::Type2> tpc3 = store.get();
    }, dds::core::IllegalOperationError);
}


DDSCXX_TEST_F(ddscxx_Conversions, writer_entity_to)
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


DDSCXX_TEST_F(ddscxx_Conversions, writer_entity_from)
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


DDSCXX_TEST_F(ddscxx_Conversions, writer_entity_null)
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


DDSCXX_TEST_F(ddscxx_Conversions, writer_entity_invalid)
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
DDSCXX_TEST_F(ddscxx_Conversions, DISABLED_writer_entity_implicit)
{
    this->CreateWriter();

    /* Test implicit conversions by using a store. */
    EntityStore store(this->writer);

    dds::pub::DataWriter<Space::Type1> wrt1(store.get());
    dds::pub::DataWriter<Space::Type1> wrt2 = store.get();
    dds::core::Entity ent1(store.get());
    dds::core::Entity ent2 = store.get();

    ASSERT_EQ(this->writer, wrt1);
    ASSERT_EQ(this->writer, wrt2);
    ASSERT_EQ(this->writer, ent1);
    ASSERT_EQ(this->writer, ent2);

    ASSERT_THROW({
        dds::pub::DataWriter<Space::Type2> tpc3 = store.get();
    }, dds::core::IllegalOperationError);
}


DDSCXX_TEST_F(ddscxx_Conversions, reader_any_to)
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


DDSCXX_TEST_F(ddscxx_Conversions, reader_any_from)
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


DDSCXX_TEST_F(ddscxx_Conversions, reader_any_null)
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


DDSCXX_TEST_F(ddscxx_Conversions, reader_any_invalid)
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
DDSCXX_TEST_F(ddscxx_Conversions, DISABLED_reader_any_implicit)
{
    this->CreateReader();

    /* Test implicit conversions by using a store. */
    AnyReaderStore store(this->reader);

    dds::sub::DataReader<Space::Type1> wrt1(store.get());
    dds::sub::DataReader<Space::Type1> wrt2 = store.get();
    dds::sub::AnyDataReader any1(store.get());
    dds::sub::AnyDataReader any2 = store.get();

    ASSERT_EQ(this->reader, wrt1);
    ASSERT_EQ(this->reader, wrt2);
    ASSERT_EQ(this->reader, any1);
    ASSERT_EQ(this->reader, any2);

    ASSERT_THROW({
        dds::sub::DataReader<Space::Type2> tpc3 = store.get();
    }, dds::core::IllegalOperationError);
}


DDSCXX_TEST_F(ddscxx_Conversions, reader_entity_to)
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


DDSCXX_TEST_F(ddscxx_Conversions, reader_entity_from)
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


DDSCXX_TEST_F(ddscxx_Conversions, reader_entity_null)
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


DDSCXX_TEST_F(ddscxx_Conversions, reader_entity_invalid)
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
DDSCXX_TEST_F(ddscxx_Conversions, DISABLED_reader_entity_implicit)
{
    this->CreateReader();

    /* Test implicit conversions by using a store. */
    EntityStore store(this->reader);

    dds::sub::DataReader<Space::Type1> wrt1(store.get());
    dds::sub::DataReader<Space::Type1> wrt2 = store.get();
    dds::core::Entity ent1(store.get());
    dds::core::Entity ent2 = store.get();

    ASSERT_EQ(this->reader, wrt1);
    ASSERT_EQ(this->reader, wrt2);
    ASSERT_EQ(this->reader, ent1);
    ASSERT_EQ(this->reader, ent2);

    ASSERT_THROW({
        dds::sub::DataReader<Space::Type2> tpc3 = store.get();
    }, dds::core::IllegalOperationError);
}
