// Copyright(c) 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <chrono>
#include <thread>
#include <gtest/gtest.h>

#include "dds/dds.hpp"
#include "dds/ddsrt/process.h"
#include "dds/ddsrt/sync.h"
#include "dds/ddsrt/threads.h"

#include "Util.hpp"
#include "HelloWorldData.hpp"

static uint32_t cb_called = 0;
static ddsrt_mutex_t g_mutex;
static ddsrt_cond_t g_cond;

class DomainParticipantListener : public virtual dds::domain::NoOpDomainParticipantListener
{
public:
    dds::sub::Subscriber data_on_readers_subscriber;

    dds::sub::AnyDataReader data_available_reader;

    DomainParticipantListener() :
        data_on_readers_subscriber(dds::core::null),
        data_available_reader(dds::core::null) { }

protected:
    virtual void on_data_on_readers(dds::sub::Subscriber& subs)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_DATA_ON_READERS_STATUS;
        this->data_on_readers_subscriber = subs;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }

    virtual void on_data_available(dds::sub::AnyDataReader& reader)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_DATA_AVAILABLE_STATUS;
        this->data_available_reader = reader;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }
};

class TopicListener : public virtual dds::topic::NoOpTopicListener<HelloWorldData::Msg>
{ };

class PublisherListener : public virtual dds::pub::NoOpPublisherListener
{
public:
    dds::pub::AnyDataWriter publication_matched_writer;
    dds::core::status::PublicationMatchedStatus publication_matched_status;

    PublisherListener() :
        publication_matched_writer(dds::core::null) { }

protected:
    virtual void on_publication_matched(dds::pub::AnyDataWriter& writer,
            const ::dds::core::status::PublicationMatchedStatus& status)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_PUBLICATION_MATCHED_STATUS;
        this->publication_matched_writer = writer;
        this->publication_matched_status = status;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }
};

class SubscriberListener : public virtual dds::sub::NoOpSubscriberListener
{
public:
    dds::sub::AnyDataReader subscription_matched_reader;
    dds::core::status::SubscriptionMatchedStatus subscription_matched_status;

    dds::sub::Subscriber data_on_readers_subscriber;

    dds::sub::AnyDataReader data_available_reader;

    SubscriberListener() :
        subscription_matched_reader(dds::core::null),
        data_on_readers_subscriber(dds::core::null),
        data_available_reader(dds::core::null) { }

protected:
    virtual void on_subscription_matched(dds::sub::AnyDataReader& reader,
            const dds::core::status::SubscriptionMatchedStatus& status)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_SUBSCRIPTION_MATCHED_STATUS;
        this->subscription_matched_reader = reader;
        this->subscription_matched_status = status;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }

    virtual void on_data_on_readers(dds::sub::Subscriber& subs)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_DATA_ON_READERS_STATUS;
        this->data_on_readers_subscriber = subs;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }

    virtual void on_data_available(dds::sub::AnyDataReader& reader)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_DATA_AVAILABLE_STATUS;
        this->data_available_reader = reader;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }
};

class DataWriterListener : public virtual dds::pub::NoOpDataWriterListener<HelloWorldData::Msg>
{
public:
    dds::pub::DataWriter<HelloWorldData::Msg> offered_incompatible_qos_writer;
    dds::core::status::OfferedIncompatibleQosStatus offered_incompatible_qos_status;

    dds::pub::DataWriter<HelloWorldData::Msg> publication_matched_writer;
    dds::core::status::PublicationMatchedStatus publication_matched_status;

    DataWriterListener() :
        offered_incompatible_qos_writer(dds::core::null),
        publication_matched_writer(dds::core::null) { }

protected:
    virtual void on_offered_incompatible_qos(dds::pub::DataWriter<HelloWorldData::Msg>& writer,
        const dds::core::status::OfferedIncompatibleQosStatus& status)
    {
        fprintf(stderr, "incompatible qos on writer\n");
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_OFFERED_INCOMPATIBLE_QOS_STATUS;
        this->offered_incompatible_qos_writer = writer;
        this->offered_incompatible_qos_status = status;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }

    virtual void on_publication_matched(dds::pub::DataWriter<HelloWorldData::Msg>& writer,
            const ::dds::core::status::PublicationMatchedStatus& status)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_PUBLICATION_MATCHED_STATUS;
        this->publication_matched_writer = writer;
        this->publication_matched_status = status;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }
};

class DataReaderListener : public virtual dds::sub::NoOpDataReaderListener<HelloWorldData::Msg>
{
public:
    dds::sub::DataReader<HelloWorldData::Msg> requested_incompatible_qos_reader;
    dds::core::status::RequestedIncompatibleQosStatus requested_incompatible_qos_status;

    dds::sub::DataReader<HelloWorldData::Msg> subscription_matched_reader;
    dds::core::status::SubscriptionMatchedStatus subscription_matched_status;

    dds::sub::DataReader<HelloWorldData::Msg> data_available_reader;

    dds::sub::DataReader<HelloWorldData::Msg> sample_lost_reader;
    dds::core::status::SampleLostStatus sample_lost_status;

    dds::sub::DataReader<HelloWorldData::Msg> sample_rejected_reader;
    dds::core::status::SampleRejectedStatus sample_rejected_status;

    dds::sub::DataReader<HelloWorldData::Msg> liveliness_changed_reader;
    dds::core::status::LivelinessChangedStatus liveliness_changed_status;

    DataReaderListener() :
        requested_incompatible_qos_reader(dds::core::null),
        subscription_matched_reader(dds::core::null),
        data_available_reader(dds::core::null),
        sample_lost_reader(dds::core::null),
        sample_rejected_reader(dds::core::null),
        liveliness_changed_reader(dds::core::null) { }

protected:
    virtual void on_requested_incompatible_qos(dds::sub::DataReader<HelloWorldData::Msg>& reader,
            const dds::core::status::RequestedIncompatibleQosStatus& status)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS;
        this->requested_incompatible_qos_reader = reader;
        this->requested_incompatible_qos_status = status;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }

    virtual void on_subscription_matched(dds::sub::DataReader<HelloWorldData::Msg>& reader,
            const dds::core::status::SubscriptionMatchedStatus& status)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_SUBSCRIPTION_MATCHED_STATUS;
        this->subscription_matched_reader = reader;
        this->subscription_matched_status = status;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }

    virtual void on_data_available(dds::sub::DataReader<HelloWorldData::Msg>& reader)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_DATA_AVAILABLE_STATUS;
        this->data_available_reader = reader;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }

    virtual void on_sample_lost(dds::sub::DataReader<HelloWorldData::Msg>& reader,
            const dds::core::status::SampleLostStatus& status)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_SAMPLE_LOST_STATUS;
        this->sample_lost_reader = reader;
        this->sample_lost_status = status;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }

    virtual void on_sample_rejected(dds::sub::DataReader<HelloWorldData::Msg>& reader,
            const dds::core::status::SampleRejectedStatus& status)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_SAMPLE_REJECTED_STATUS;
        this->sample_rejected_reader = reader;
        this->sample_rejected_status = status;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }

    virtual void on_liveliness_changed(dds::sub::DataReader<HelloWorldData::Msg>& reader,
            const dds::core::status::LivelinessChangedStatus& status)
    {
        ddsrt_mutex_lock(&g_mutex);
        cb_called |= DDS_LIVELINESS_CHANGED_STATUS;
        this->liveliness_changed_reader = reader;
        this->liveliness_changed_status = status;
        ddsrt_cond_broadcast(&g_cond);
        ddsrt_mutex_unlock(&g_mutex);
    }
};


static uint32_t waitfor_cb(uint32_t expected, dds_time_t timeout)
{
    bool signalled = true;
    ddsrt_mutex_lock(&g_mutex);
    while (((cb_called & expected) != expected) && (signalled)) {
        signalled = ddsrt_cond_waitfor(&g_cond, &g_mutex, timeout);
    }
    ddsrt_mutex_unlock(&g_mutex);
    return cb_called;
}

static uint32_t waitfor_cb(uint32_t expected)
{
    dds_time_t timeout = 5 * DDS_NSECS_IN_SEC;
    return waitfor_cb(expected, timeout);
}

static void reset_cb()
{
    ddsrt_mutex_lock(&g_mutex);
    cb_called = 0;
    ddsrt_mutex_unlock(&g_mutex);
}

/*
 * Fixture for listener trigger tests
 */
class Listener : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant;
    dds::topic::Topic<HelloWorldData::Msg> topic;
    dds::pub::Publisher publisher;
    dds::sub::Subscriber subscriber;

    Listener() :
        participant(dds::core::null),
        topic(dds::core::null),
        publisher(dds::core::null),
        subscriber(dds::core::null)
    {
        // Empty
    }

    void SetUp() {
        char name[32];

        ddsrt_mutex_init(&g_mutex);
        ddsrt_cond_init(&g_cond);

        // Create participant
        participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(participant, dds::core::null);

        // Create topic
        create_unique_topic_name("Listener", name, sizeof(name));
        topic = dds::topic::Topic<HelloWorldData::Msg>(participant, name);
        ASSERT_NE(topic, dds::core::null);

        // Create publisher
        publisher = dds::pub::Publisher(participant);
        ASSERT_NE(publisher, dds::core::null);

        // Create subscriber
        subscriber = dds::sub::Subscriber(participant);
        ASSERT_NE(subscriber, dds::core::null);

        reset_cb();
    }

    void TearDown() {
        subscriber = dds::core::null;
        publisher = dds::core::null;
        topic = dds::core::null;
        participant = dds::core::null;

        ddsrt_cond_destroy(&g_cond);
        ddsrt_mutex_destroy(&g_mutex);
    }
};

/*
 * PARTICIPANT
 */

/*
 * Create participant with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
TEST_F(Listener, participant_with_listener)
{
    DomainParticipantListener listener;

    // Create participant with a listener
    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id(),
        dds::domain::qos::DomainParticipantQos(), &listener, dds::core::status::StatusMask());
    ASSERT_NE(participant, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::domain::DomainParticipantListener *listener1 = participant.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &listener);

    // Set listener to null
    participant.listener(NULL, dds::core::status::StatusMask());
    dds::domain::DomainParticipantListener *listener2 = participant.listener();
    ASSERT_EQ(listener2, nullptr);
}

/*
 * Set the listener on a domain participant after the entity has been closed
 */
TEST_F(Listener, already_closed_participant)
{
    DomainParticipantListener listener;

    // Close the participant
    participant.close();

    ASSERT_THROW({
        // Set listener
        participant.listener(&listener, dds::core::status::StatusMask());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        participant.listener();
    }, dds::core::AlreadyClosedError);
}

/*
 * Create participant without listener, set listener on participant, retrieve listener and
 * check if correct listener is returned. Then delete the participant with the listener.
 */
TEST_F(Listener, participant_without_listener)
{
    DomainParticipantListener listener;

    // Listener should not be set
    dds::domain::DomainParticipantListener *listener1 = participant.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener
    participant.listener(&listener, dds::core::status::StatusMask());

    // Get listener and check if it equals listener used when setting
    dds::domain::DomainParticipantListener *listener2 = participant.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &listener);

    // Delete participant with listener
    participant = dds::core::null;
    ASSERT_EQ(participant, dds::core::null);
}


/*
 * TOPIC
 */

/*
 * Create topic without listener, set listener on the topic, retrieve listener and
 * check if correct listener is returned. Then delete the topic with the listener.
 */
TEST_F(Listener, topic_without_listener)
{
    TopicListener listener;

    // Listener should not be set
    dds::topic::TopicListener<HelloWorldData::Msg> *listener1 = topic.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener on topic
    topic.listener(&listener, dds::core::status::StatusMask());

    // Get listener and check if it equals listener used when setting
    dds::topic::TopicListener<HelloWorldData::Msg> *listener2 = topic.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &listener);

    // Delete topic with listener
    topic = dds::core::null;
    ASSERT_EQ(topic, dds::core::null);
}

/*
 * Create topic with listener, retrieve listener and check if correct listener
 * is returned. Then set the listener to null and check if no listener is
 * returned
 */
TEST_F(Listener, topic_with_listener)
{
    TopicListener listener;

    // Currently, cyclonedds has some 'duplicate topic' deletion behaviour
    // that doesn't work for isocpp when assigning a new topic to the exact
    // same topic (which we try in this test). To circumvent that, remove
    // the first topic so that there is no duplicate.
    topic = dds::core::null;

    // Create topic with listener
    topic = dds::topic::Topic<HelloWorldData::Msg>(
        participant, "topic", dds::topic::qos::TopicQos(), &listener, dds::core::status::StatusMask());
    ASSERT_NE(topic, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::topic::TopicListener<HelloWorldData::Msg> *listener1 = topic.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &listener);

    // Set listener to null
    topic.listener(NULL, dds::core::status::StatusMask());
    dds::topic::TopicListener<HelloWorldData::Msg> *listener2 = topic.listener();
    ASSERT_EQ(listener2, nullptr);
}

/*
 * Set the listener on a topic after the entity has been closed
 */
TEST_F(Listener, already_closed_topic)
{
    TopicListener listener;

    // Close the topic
    topic.close();

    ASSERT_THROW({
        // Set listener
        topic.listener(&listener, dds::core::status::StatusMask());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        topic.listener();
    }, dds::core::AlreadyClosedError);
}


/*
 * PUBLISHER
 */

/*
 * Create publisher without listener, set listener on the publisher, retrieve listener and
 * check if correct listener is returned. Then delete the topic with the listener.
 */
TEST_F(Listener, publisher_without_listener)
{
    PublisherListener listener;

    // Listener should not be set
    dds::pub::PublisherListener *listener1 = publisher.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener on publisher
    publisher.listener(&listener, dds::core::status::StatusMask());

    // Get listener and check if it equals listener used when setting
    dds::pub::PublisherListener *listener2 = publisher.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &listener);

    // Delete publisher with listener
    publisher = dds::core::null;
    ASSERT_EQ(publisher, dds::core::null);
}

/*
 * Create publisher with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
TEST_F(Listener, publisher_with_listener)
{
    PublisherListener listener;

    // Create publisher with listener
    publisher = dds::pub::Publisher(
        participant, dds::pub::qos::PublisherQos(), &listener, dds::core::status::StatusMask());
    ASSERT_NE(publisher, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::pub::PublisherListener *listener1 = publisher.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &listener);

    // Set listener to null
    publisher.listener(NULL, dds::core::status::StatusMask());
    dds::pub::PublisherListener *listener2 = publisher.listener();
    ASSERT_EQ(listener2, nullptr);
}

/*
 * Set the listener on a publisher after the entity has been closed
 */
TEST_F(Listener, already_closed_publisher)
{
    PublisherListener listener;

    // Close the publisher
    publisher.close();

    ASSERT_THROW({
        // Set listener
        publisher.listener(&listener, dds::core::status::StatusMask());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        publisher.listener();
    }, dds::core::AlreadyClosedError);
}


/*
 * SUBSCRIBER
 */

/*
 * Create subscriber without listener, set listener on the subscriber,
 * retrieve listener and check if correct listener is returned. Then delete
 * the topic with the listener.
 */
TEST_F(Listener, subscriber_without_listener)
{
    SubscriberListener listener;

    // Listener should not be set
    dds::sub::SubscriberListener *listener1 = subscriber.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener on subscriber
    subscriber.listener(&listener, dds::core::status::StatusMask());

    // Get listener and check if it equals listener used when setting
    dds::sub::SubscriberListener *listener2 = subscriber.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &listener);

    // Delete subscriber with listener
    subscriber = dds::core::null;
    ASSERT_EQ(subscriber, dds::core::null);
}

/*
 * Create subscriber with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
TEST_F(Listener, subscriber_with_listener)
{
    SubscriberListener listener;

    // Create subscriber with listener
    subscriber = dds::sub::Subscriber(
        participant, dds::sub::qos::SubscriberQos(), &listener, dds::core::status::StatusMask());
    ASSERT_NE(subscriber, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::sub::SubscriberListener *listener1 = subscriber.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &listener);

    // Set listener to null
    subscriber.listener(NULL, dds::core::status::StatusMask());
    dds::sub::SubscriberListener *listener2 = subscriber.listener();
    ASSERT_EQ(listener2, nullptr);
}

/*
 * Set the listener on a subscriber after the entity has been closed
 */
TEST_F(Listener, already_closed_subscriber)
{
    SubscriberListener listener;

    // Close the subscriber
    subscriber.close();

    ASSERT_THROW({
        // Set listener
        subscriber.listener(&listener, dds::core::status::StatusMask());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        subscriber.listener();
    }, dds::core::AlreadyClosedError);
}


/*
 * WRITER
 */

/*
 * Create data writer without listener, set listener on the writer, retrieve
 * listener and check if correct listener is returned. Then delete the topic
 * with the listener.
 */
TEST_F(Listener, writer_without_listener)
{
    DataWriterListener listener;

    // Create data writer
    dds::pub::DataWriter<HelloWorldData::Msg> writer(publisher, topic);
    ASSERT_NE(writer, dds::core::null);

    // Listener should not be set
    dds::pub::DataWriterListener<HelloWorldData::Msg> *listener1 = writer.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener on writer
    writer.listener(&listener, dds::core::status::StatusMask());

    // Get listener and check if it equals listener used when setting
    dds::pub::DataWriterListener<HelloWorldData::Msg> *listener2 = writer.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &listener);

    // Delete data writer with listener
    writer = dds::core::null;
    ASSERT_EQ(writer, dds::core::null);
}

/*
 * Create data writer with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
TEST_F(Listener, writer_with_listener)
{
    DataWriterListener listener;

    // Create data writer with listener
    dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic, dds::pub::qos::DataWriterQos(), &listener, dds::core::status::StatusMask());
    ASSERT_NE(writer, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::pub::DataWriterListener<HelloWorldData::Msg> *listener1 = writer.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &listener);

    // Set listener to null
    writer.listener(NULL, dds::core::status::StatusMask());
    dds::pub::DataWriterListener<HelloWorldData::Msg> *listener2 = writer.listener();
    ASSERT_EQ(listener2, nullptr);
}

/*
 * Set the listener on a writer after the entity has been closed
 */
TEST_F(Listener, already_closed_writer)
{
    DataWriterListener listener;

    // Create data writer
    dds::pub::DataWriter<HelloWorldData::Msg> writer(publisher, topic);
    ASSERT_NE(writer, dds::core::null);

    // Close the writer
    writer.close();

    ASSERT_THROW({
        // Set listener
        writer.listener(&listener, dds::core::status::StatusMask());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        writer.listener();
    }, dds::core::AlreadyClosedError);
}


/*
 * READER
 */

/*
 * Create reader without listener, set listener on the reader, retrieve listener and
 * check if correct listener is returned. Then delete the topic with the listener.
 */
TEST_F(Listener, reader_without_listener)
{
    DataReaderListener listener;

    // Create data reader with listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, dds::sub::qos::DataReaderQos(), nullptr, dds::core::status::StatusMask());
    ASSERT_NE(reader, dds::core::null);

    // Listener should not be set
    dds::sub::DataReaderListener<HelloWorldData::Msg> *listener1 = reader.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener on reader
    reader.listener(&listener, dds::core::status::StatusMask());

    // Get listener and check if it equals listener used when setting
    dds::sub::DataReaderListener<HelloWorldData::Msg> *listener2 = reader.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &listener);

    // Delete data reader with listener
    reader = dds::core::null;
    ASSERT_EQ(reader, dds::core::null);
}

/*
 * Create data reader with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
TEST_F(Listener, reader_with_listener)
{
    DataReaderListener listener;

    // Create data reader with listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, dds::sub::qos::DataReaderQos(), &listener, dds::core::status::StatusMask());
    ASSERT_NE(reader, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::sub::DataReaderListener<HelloWorldData::Msg> *listener1 = reader.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &listener);

    // Set listener to null
    reader.listener(NULL, dds::core::status::StatusMask());
    dds::sub::DataReaderListener<HelloWorldData::Msg> *listener2 = reader.listener();
    ASSERT_EQ(listener2, nullptr);
}

/*
 * Set the listener on a reader after the entity has been closed
 */
TEST_F(Listener, already_closed_reader)
{
    DataReaderListener listener;

    // Create data reader with listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(subscriber, topic);
    ASSERT_NE(reader, dds::core::null);

    // Close the reader
    reader.close();

    ASSERT_THROW({
        // Set listener
        reader.listener(&listener, dds::core::status::StatusMask());
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        reader.listener();
    }, dds::core::AlreadyClosedError);

    reader = dds::core::null;
}



TEST_F(Listener, incorrect_listener_mask)
{
    DataReaderListener listener;
    dds::core::status::StatusMask mask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::publication_matched();

    // Create reader with reader listener, but using mask for writer event type
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, dds::sub::qos::DataReaderQos(), &listener, mask);
    ASSERT_NE(reader, dds::core::null);

    // Create writer
     dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic, dds::pub::qos::DataWriterQos());
     ASSERT_NE(writer, dds::core::null);

    // Subscription matched handler should not be triggered
    dds_time_t timeout = 500 * DDS_NSECS_IN_MSEC;
    uint32_t triggered = waitfor_cb(DDS_SUBSCRIPTION_MATCHED_STATUS, timeout);
    ASSERT_NE(triggered & DDS_SUBSCRIPTION_MATCHED_STATUS, DDS_SUBSCRIPTION_MATCHED_STATUS);
}

TEST_F(Listener, sample_lost)
{
    DataReaderListener listener;
    dds::core::status::StatusMask mask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::sample_lost();
    dds::sub::qos::DataReaderQos readerQos =
        dds::sub::qos::DataReaderQos() <<
        dds::core::policy::DestinationOrder::SourceTimestamp();
    dds::pub::qos::DataWriterQos writerQos =
        dds::pub::qos::DataWriterQos() <<
        dds::core::policy::DestinationOrder::SourceTimestamp();

    // Create reader with listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, readerQos, &listener, mask);
    ASSERT_NE(reader, dds::core::null);

    // Create writer
    dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic, writerQos);
    ASSERT_NE(writer, dds::core::null);

    // Write sample
    HelloWorldData::Msg sample(1, "test1");
    dds::core::Time current_time = participant.current_time();
    dds::core::Time the_past = current_time - dds::core::Duration::from_secs(1000000);
    writer.write(sample, current_time);
    writer.write(sample, the_past);

    // Sample lost should be triggered with the right status
    uint32_t triggered = waitfor_cb(DDS_SAMPLE_LOST_STATUS);
    ASSERT_EQ(triggered & DDS_SAMPLE_LOST_STATUS, DDS_SAMPLE_LOST_STATUS);
    ASSERT_EQ(listener.sample_lost_reader.delegate(), reader.delegate());
    ASSERT_EQ(listener.sample_lost_status.total_count(), 1);
    ASSERT_EQ(listener.sample_lost_status.total_count_change(), 1);

    // The listener should have swallowed the status on reader
    ASSERT_FALSE(reader.status_changes().test(DDS_SAMPLE_LOST_STATUS_ID));

    /* The listener should have reset the count_change variables. */
    ASSERT_EQ(reader.sample_lost_status().total_count(), 1);
    ASSERT_EQ(reader.sample_lost_status().total_count_change(), 0);
}

TEST_F(Listener, sample_rejected)
{
    DataReaderListener listener;
    dds::core::status::StatusMask mask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::sample_rejected();
    dds::sub::qos::DataReaderQos readerQos =
        dds::sub::qos::DataReaderQos() <<
        dds::core::policy::History::KeepAll() <<
        dds::core::policy::Reliability::BestEffort(dds::core::Duration(0, DDS_MSECS(100))) <<
        dds::core::policy::ResourceLimits(1, 1, 1);
    dds::pub::qos::DataWriterQos writerQos =
        dds::pub::qos::DataWriterQos() <<
        dds::core::policy::History::KeepAll() <<
        dds::core::policy::Reliability::BestEffort(dds::core::Duration(0, DDS_MSECS(100)));

    // Create reader with listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, readerQos, &listener, mask);
    ASSERT_NE(reader, dds::core::null);

    // Create writer
    dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic, writerQos);
    ASSERT_NE(writer, dds::core::null);

    // Write samples (more than resource limit)
    HelloWorldData::Msg sample(1, "test1");
    writer << sample;
    writer << sample;
    writer << sample;

    // Sample rejected should be triggered with the right status
    uint32_t triggered = waitfor_cb(DDS_SAMPLE_REJECTED_STATUS);
    dds::core::status::SampleRejectedState reason =
        dds::core::status::SampleRejectedState::rejected_by_samples_limit();
    ASSERT_EQ(triggered & DDS_SAMPLE_REJECTED_STATUS, DDS_SAMPLE_REJECTED_STATUS);
    ASSERT_EQ(listener.sample_rejected_reader.delegate(), reader.delegate());
    ASSERT_EQ(listener.sample_rejected_status.total_count(), 2);
    ASSERT_EQ(listener.sample_rejected_status.total_count_change(), 1);
    ASSERT_EQ(listener.sample_rejected_status.last_reason(), reason);
    ASSERT_EQ(listener.sample_rejected_status.last_instance_handle(), reader.lookup_instance(sample));

    // The listener should have swallowed the status on reader
    ASSERT_FALSE(reader.status_changes().test(DDS_SAMPLE_REJECTED_STATUS_ID));

    /* The listener should have reset the count_change variables. */
    dds::core::status::SampleRejectedStatus status = reader.sample_rejected_status();
    ASSERT_EQ(status.total_count(), 2);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_reason(), reason);
    ASSERT_EQ(status.last_instance_handle(), reader.lookup_instance(sample));
}

TEST_F(Listener, liveliness_changed)
{
    DataReaderListener listener;
    dds::core::status::StatusMask mask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::liveliness_changed();
    dds_return_t retcode;
    dds_instance_handle_t instance_handle;

    // Create reader with listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, dds::sub::qos::DataReaderQos(), &listener, mask);
    ASSERT_NE(reader, dds::core::null);

    // Create writer
    dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic);
    ASSERT_NE(writer, dds::core::null);

    retcode = dds_get_instance_handle(
        writer.delegate()->get_ddsc_entity(), &instance_handle);
    ASSERT_EQ(retcode, DDS_RETCODE_OK);

    // Liveliness changed should be triggered with the right status
    uint32_t triggered = waitfor_cb(DDS_LIVELINESS_CHANGED_STATUS);
    ASSERT_EQ(triggered & DDS_LIVELINESS_CHANGED_STATUS, DDS_LIVELINESS_CHANGED_STATUS);
    ASSERT_EQ(listener.liveliness_changed_reader.delegate(), reader.delegate());
    ASSERT_EQ(listener.liveliness_changed_status.alive_count(), 1);
    ASSERT_EQ(listener.liveliness_changed_status.alive_count_change(), 1);
    ASSERT_EQ(listener.liveliness_changed_status.not_alive_count(), 0);
    ASSERT_EQ(listener.liveliness_changed_status.not_alive_count_change(), 0);
    ASSERT_EQ(listener.liveliness_changed_status.last_publication_handle(), instance_handle);

    /* The listener should have reset the count_change variables. */
    dds::core::status::LivelinessChangedStatus status = reader.liveliness_changed_status();
    ASSERT_EQ(status.alive_count(), 1);
    ASSERT_EQ(status.alive_count_change(), 0);
    ASSERT_EQ(status.not_alive_count(), 0);
    ASSERT_EQ(status.not_alive_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), instance_handle);

    // The listener should have swallowed the status on reader
    ASSERT_FALSE(reader.status_changes().test(DDS_LIVELINESS_CHANGED_STATUS_ID));

    /* Reset the trigger flags. */
    reset_cb();

    /* Change liveliness again by deleting the writer. */
    writer = dds::core::null;

    // Liveliness changed should be triggered with the right status
    triggered = waitfor_cb(DDS_LIVELINESS_CHANGED_STATUS);
    ASSERT_EQ(triggered & DDS_LIVELINESS_CHANGED_STATUS, DDS_LIVELINESS_CHANGED_STATUS);
    ASSERT_EQ(listener.liveliness_changed_reader.delegate(), reader.delegate());
    ASSERT_EQ(listener.liveliness_changed_status.alive_count(), 0);
    ASSERT_EQ(listener.liveliness_changed_status.alive_count_change(), -1);
    ASSERT_EQ(listener.liveliness_changed_status.not_alive_count(), 0);
    ASSERT_EQ(listener.liveliness_changed_status.not_alive_count_change(), 0);
    ASSERT_EQ(listener.liveliness_changed_status.last_publication_handle(), instance_handle);

    /* The listener should have reset the count_change variables. */
    status = reader.liveliness_changed_status();
    ASSERT_EQ(status.alive_count(), 0);
    ASSERT_EQ(status.alive_count_change(), 0);
    ASSERT_EQ(status.not_alive_count(), 0);
    ASSERT_EQ(status.not_alive_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), instance_handle);
}

TEST_F(Listener, DISABLED_incompatible_qos)
{
    DataReaderListener readerListener;
    DataWriterListener writerListener;
    dds::core::status::StatusMask readerMask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::requested_incompatible_qos();
    dds::core::status::StatusMask writerMask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::offered_incompatible_qos();
    dds::pub::qos::DataWriterQos writerQos =
        dds::pub::qos::DataWriterQos() <<
        dds::core::policy::History::KeepAll();
    dds::sub::qos::DataReaderQos readerQos =
        dds::sub::qos::DataReaderQos() <<
        dds::core::policy::History::KeepAll() <<
        dds::core::policy::Durability::Persistent();

    // Create writer with listener
    dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic, writerQos, &writerListener, writerMask);
    ASSERT_NE(writer, dds::core::null);

    // Create reader with listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, readerQos, &readerListener, readerMask);
    ASSERT_NE(reader, dds::core::null);

    // Incompatible QoS should be triggered with the right status
    uint32_t triggered = waitfor_cb(DDS_OFFERED_INCOMPATIBLE_QOS_STATUS | DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS);
    ASSERT_EQ(triggered & DDS_OFFERED_INCOMPATIBLE_QOS_STATUS, DDS_OFFERED_INCOMPATIBLE_QOS_STATUS);
    ASSERT_EQ(triggered & DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS, DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS);
    ASSERT_EQ(writerListener.offered_incompatible_qos_writer.delegate(), writer.delegate());
    ASSERT_EQ(writerListener.offered_incompatible_qos_status.total_count(), 1);
    ASSERT_EQ(writerListener.offered_incompatible_qos_status.total_count_change(), 1);
    ASSERT_EQ(writerListener.offered_incompatible_qos_status.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);
    ASSERT_EQ(readerListener.requested_incompatible_qos_reader.delegate(), reader.delegate());
    ASSERT_EQ(readerListener.requested_incompatible_qos_status.total_count(), 1);
    ASSERT_EQ(readerListener.requested_incompatible_qos_status.total_count_change(), 1);
    ASSERT_EQ(readerListener.requested_incompatible_qos_status.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);

    // Test if status is swallowed
    ASSERT_FALSE(writer.status_changes().test(DDS_OFFERED_INCOMPATIBLE_QOS_STATUS_ID));
    ASSERT_FALSE(reader.status_changes().test(DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS_ID));

    // The listener should have reset the count_change variables
    dds::core::status::OfferedIncompatibleQosStatus writerStatus = writer.offered_incompatible_qos_status();
    ASSERT_EQ(writerStatus.total_count(), 1);
    ASSERT_EQ(writerStatus.total_count_change(), 0);
    ASSERT_EQ(writerStatus.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);
    dds::core::status::RequestedIncompatibleQosStatus readerStatus = reader.requested_incompatible_qos_status();
    ASSERT_EQ(readerStatus.total_count(), 1);
    ASSERT_EQ(readerStatus.total_count_change(), 0);
    ASSERT_EQ(readerStatus.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);
}

TEST_F(Listener, publication_matched)
{
    DataWriterListener listener;
    dds::core::status::StatusMask mask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::publication_matched();
    dds::pub::qos::DataWriterQos writerQos =
        dds::pub::qos::DataWriterQos() <<
        dds::core::policy::History::KeepAll();
    dds::sub::qos::DataReaderQos readerQos =
        dds::sub::qos::DataReaderQos() <<
        dds::core::policy::History::KeepAll();
    dds_return_t retcode;
    dds_instance_handle_t instance_handle;

    // Create writer with listener
    dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic, writerQos, &listener, mask);
    ASSERT_NE(writer, dds::core::null);

    // Create reader
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, readerQos);
    ASSERT_NE(reader, dds::core::null);

    retcode = dds_get_instance_handle(reader.delegate()->get_ddsc_entity(), &instance_handle);
    ASSERT_EQ(retcode, DDS_RETCODE_OK);

    uint32_t triggered = waitfor_cb(DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_PUBLICATION_MATCHED_STATUS, DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(listener.publication_matched_writer.delegate(), writer.delegate());
    ASSERT_EQ(listener.publication_matched_status.current_count(), 1);
    ASSERT_EQ(listener.publication_matched_status.current_count_change(), 1);
    ASSERT_EQ(listener.publication_matched_status.total_count(), 1);
    ASSERT_EQ(listener.publication_matched_status.total_count_change(), 1);
    ASSERT_EQ(listener.publication_matched_status.last_subscription_handle(), instance_handle);

    /* The listener should have reset the count_change variables. */
    dds::core::status::PublicationMatchedStatus status = writer.publication_matched_status();
    ASSERT_EQ(status.current_count(), 1);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_subscription_handle(), instance_handle);

    /* Reset the trigger flags. */
    reset_cb();

    /* Un-match the publication by deleting the reader. */
    reader = dds::core::null;

    /* Publication matched should be triggered with the right status. */
    triggered = waitfor_cb(DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_PUBLICATION_MATCHED_STATUS, DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(listener.publication_matched_writer.delegate(), writer.delegate());
    ASSERT_EQ(listener.publication_matched_status.current_count(), 0);
    ASSERT_EQ(listener.publication_matched_status.current_count_change(), -1);
    ASSERT_EQ(listener.publication_matched_status.total_count(), 1);
    ASSERT_EQ(listener.publication_matched_status.total_count_change(), 0);
    ASSERT_EQ(listener.publication_matched_status.last_subscription_handle(), instance_handle);

    /* The listener should have reset the count_change variables. */
    status = writer.publication_matched_status();
    ASSERT_EQ(status.current_count(), 0);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_subscription_handle(), instance_handle);
}

TEST_F(Listener, subscription_matched)
{
    DataReaderListener listener;
    dds::core::status::StatusMask mask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::subscription_matched();
    dds::pub::qos::DataWriterQos writerQos =
        dds::pub::qos::DataWriterQos() <<
        dds::core::policy::History::KeepAll();
    dds::sub::qos::DataReaderQos readerQos =
        dds::sub::qos::DataReaderQos() <<
        dds::core::policy::History::KeepAll();
    dds_return_t retcode;
    dds_instance_handle_t instance_handle;

    // Create reader with listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, readerQos, &listener, mask);
    ASSERT_NE(reader, dds::core::null);

    // Create writer
    dds::pub::DataWriter<HelloWorldData::Msg> writer(publisher, topic, writerQos);
    ASSERT_NE(writer, dds::core::null);

    retcode = dds_get_instance_handle(writer.delegate()->get_ddsc_entity(), &instance_handle);
    ASSERT_EQ(retcode, DDS_RETCODE_OK);

    uint32_t triggered = waitfor_cb(DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_SUBSCRIPTION_MATCHED_STATUS, DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(listener.subscription_matched_reader.delegate(), reader.delegate());
    ASSERT_EQ(listener.subscription_matched_status.current_count(), 1);
    ASSERT_EQ(listener.subscription_matched_status.current_count_change(), 1);
    ASSERT_EQ(listener.subscription_matched_status.total_count(), 1);
    ASSERT_EQ(listener.subscription_matched_status.total_count_change(), 1);
    ASSERT_EQ(listener.subscription_matched_status.last_publication_handle(), instance_handle);

    /* The listener should have reset the count_change variables. */
    dds::core::status::SubscriptionMatchedStatus status = reader.subscription_matched_status();
    ASSERT_EQ(status.current_count(), 1);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), instance_handle);

    /* Reset the trigger flags. */
    reset_cb();

    /* Un-match the subscription by deleting the writer. */
    writer = dds::core::null;

    /* Subscription matched should be triggered with the right status. */
    triggered = waitfor_cb(DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_SUBSCRIPTION_MATCHED_STATUS, DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(listener.subscription_matched_reader.delegate(), reader.delegate());
    ASSERT_EQ(listener.subscription_matched_status.current_count(), 0);
    ASSERT_EQ(listener.subscription_matched_status.current_count_change(), -1);
    ASSERT_EQ(listener.subscription_matched_status.total_count(), 1);
    ASSERT_EQ(listener.subscription_matched_status.total_count_change(), 0);
    ASSERT_EQ(listener.subscription_matched_status.last_publication_handle(), instance_handle);

    /* The listener should have reset the count_change variables. */
    status = reader.subscription_matched_status();
    ASSERT_EQ(status.current_count(), 0);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), instance_handle);
}

TEST_F(Listener, publication_subscription_matched)
{
    DataReaderListener readerListener;
    DataWriterListener writerListener;
    dds::core::status::StatusMask readerMask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::subscription_matched();
    dds::core::status::StatusMask writerMask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::publication_matched();
    dds::pub::qos::DataWriterQos writerQos =
        dds::pub::qos::DataWriterQos() <<
        dds::core::policy::History::KeepAll();
    dds::sub::qos::DataReaderQos readerQos =
        dds::sub::qos::DataReaderQos() <<
        dds::core::policy::History::KeepAll();

    dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic, writerQos, &writerListener, writerMask);
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, readerQos, &readerListener, readerMask);

    // Create reader with listener
    ASSERT_NE(reader, dds::core::null);

    // Create writer with listener
    ASSERT_NE(writer, dds::core::null);

    // Publication and Subscription should be matched.
    uint32_t triggered = waitfor_cb(DDS_SUBSCRIPTION_MATCHED_STATUS | DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_SUBSCRIPTION_MATCHED_STATUS, DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_PUBLICATION_MATCHED_STATUS, DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(writerListener.publication_matched_writer.delegate(), writer.delegate());
    ASSERT_EQ(readerListener.subscription_matched_reader.delegate(), reader.delegate());
}

TEST_F(Listener, data_available)
{
    SubscriberListener subscriberListener;
    DataReaderListener readerListener;
    dds::core::status::StatusMask mask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::data_available();
    dds::sub::qos::DataReaderQos qos =
        dds::sub::qos::DataReaderQos();
    uint32_t triggered;

    // Create reader with listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, qos, &readerListener, mask);
    ASSERT_NE(reader, dds::core::null);

    // Create writer
    dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic);
    ASSERT_NE(writer, dds::core::null);

    // Write sample
    HelloWorldData::Msg sample(1, "test");
    writer << sample;

    // Data available should be triggered on reader
    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(readerListener.data_available_reader.delegate(), reader.delegate());

    // Reset the trigger flags.
    reset_cb();
    readerListener.data_available_reader = dds::core::null;

    // Add data-available listener to subscriber
    subscriber.listener(&subscriberListener, mask);

    // Write another sample
    writer << sample;

    // Data available should still be triggered on reader, not on subscriber
    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(readerListener.data_available_reader.delegate(), reader.delegate());
    ASSERT_EQ(subscriberListener.data_available_reader, dds::core::null);

    // Reset the trigger flags.
    reset_cb();
    readerListener.data_available_reader = dds::core::null;

    // Deleting the writer should trigger DATA_AVAILABLE as well
    writer = dds::core::null;

    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(readerListener.data_available_reader.delegate(), reader.delegate());

    // The listener should have swallowed the status on reader
    ASSERT_FALSE(reader.status_changes().test(DDS_DATA_AVAILABLE_STATUS_ID));
}

TEST_F(Listener, data_available_subscriber)
{
    SubscriberListener subscriberListener;
    DataReaderListener readerListener;
    dds::core::status::StatusMask mask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::data_available();
    dds::pub::qos::DataWriterQos writerQos =
        dds::pub::qos::DataWriterQos() <<
        dds::core::policy::History::KeepAll();
    dds::sub::qos::DataReaderQos readerQos =
        dds::sub::qos::DataReaderQos() <<
        dds::core::policy::History::KeepAll();

    // Add data-available listener to subscriber
    subscriber.listener(&subscriberListener, mask);

    // Create reader without listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(subscriber, topic, readerQos);
    ASSERT_NE(reader, dds::core::null);

    // Create writer
    dds::pub::DataWriter<HelloWorldData::Msg> writer(publisher, topic, writerQos);
    ASSERT_NE(writer, dds::core::null);

    // Write sample
    HelloWorldData::Msg sample(1, "test");
    writer << sample;

    // Data available should be triggered on subscriber
    uint32_t triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(subscriberListener.data_available_reader.delegate(), reader.delegate());

    // Event should not be triggered on reader
    ASSERT_EQ(readerListener.data_available_reader, dds::core::null);
}

TEST_F(Listener, data_available_participant)
{
    DomainParticipantListener participantListener;
    SubscriberListener subscriberListener;
    DataReaderListener readerListener;
    dds::core::status::StatusMask mask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::data_available();
    dds::pub::qos::DataWriterQos writerQos =
        dds::pub::qos::DataWriterQos() <<
        dds::core::policy::History::KeepAll();
    dds::sub::qos::DataReaderQos readerQos =
        dds::sub::qos::DataReaderQos() <<
        dds::core::policy::History::KeepAll();

    // Add data-available listener to participant
    participant.listener(&participantListener, mask);

    // Create reader without listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic, readerQos);
    ASSERT_NE(reader, dds::core::null);

    // Create writer
    dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic, writerQos);
    ASSERT_NE(writer, dds::core::null);

    // Write sample
    HelloWorldData::Msg sample(1, "test");
    writer << sample;

    // Data available should be triggered on participant
    uint32_t triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(participantListener.data_available_reader.delegate(), reader.delegate());

    // Event should not be triggered on subscriber or reader
    ASSERT_EQ(subscriberListener.data_available_reader, dds::core::null);
    ASSERT_EQ(readerListener.data_available_reader, dds::core::null);

    // Reset the trigger flags
    reset_cb();
    participantListener.data_available_reader = dds::core::null;

    // Add data-available listener to subscriber
    subscriber.listener(&subscriberListener, mask);

    // Write sample
    writer << sample;

    // Data available should be triggered on subscriber, not on participant
    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(subscriberListener.data_available_reader.delegate(), reader.delegate());
    ASSERT_EQ(participantListener.data_available_reader, dds::core::null);

    // Reset the trigger flags.
    reset_cb();
    subscriberListener.data_available_reader = dds::core::null;

    // Remove data-available listener from subscriber
    subscriber.listener(NULL, dds::core::status::StatusMask::none());

    // Write sample
    writer << sample;

    // Data available should be triggered on participant again
    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(participantListener.data_available_reader.delegate(), reader.delegate());
}

// TODO:
// this test does not work because listener on publisher is triggered when writer is created
// but before ddsc handle for writer is stored in the writer entity.
TEST_F(Listener, DISABLED_propagation)
{
    DomainParticipantListener participantListener;
    SubscriberListener subscriberListener;
    PublisherListener publisherListener;
    dds::core::status::StatusMask participantMask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::data_on_readers();
    dds::core::status::StatusMask publisherMask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::publication_matched();
    dds::core::status::StatusMask subscriberMask =
        dds::core::status::StatusMask() <<
        dds::core::status::StatusMask::subscription_matched();

    // Set listener on participant
    participant.listener(&participantListener, participantMask);

    // Set listener on publisher
    publisher.listener(&publisherListener, publisherMask);

    // Set listener on subscriber
    subscriber.listener(&subscriberListener, subscriberMask);

    // Create reader and writer without listener
    dds::sub::DataReader<HelloWorldData::Msg> reader(
        subscriber, topic);
    ASSERT_NE(reader, dds::core::null);

    dds::pub::DataWriter<HelloWorldData::Msg> writer(
        publisher, topic);
    ASSERT_NE(writer, dds::core::null);

    // Publication and Subscription should be matched.
    uint32_t triggered = waitfor_cb(DDS_PUBLICATION_MATCHED_STATUS | DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_SUBSCRIPTION_MATCHED_STATUS, DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_PUBLICATION_MATCHED_STATUS,  DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(publisherListener.publication_matched_writer.delegate(), writer.delegate());
    ASSERT_EQ(subscriberListener.subscription_matched_reader.delegate(), reader.delegate());

    // Write sample
    HelloWorldData::Msg sample(1, "test");
    writer << sample;

    // Data on readers should be triggered with the right status.
    triggered = waitfor_cb(DDS_DATA_ON_READERS_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_ON_READERS_STATUS, DDS_DATA_ON_READERS_STATUS);
    ASSERT_EQ(publisherListener.publication_matched_writer.delegate(), writer.delegate());
    ASSERT_EQ(participantListener.data_on_readers_subscriber.delegate(), subscriber.delegate());
}
