#include "dds/dds.hpp"
#include "dds/ddscxx/test.h"

#include "HelloWorldData_DCPS.hpp"

namespace lite { namespace ddscxx { namespace tests { namespace listener_setup {

class TestDomainParticipantListener : public virtual dds::domain::NoOpDomainParticipantListener
{ };

class TestTopicListener : public virtual dds::topic::NoOpTopicListener<HelloWorldData::Msg>
{ };

class TestPublisherListener : public virtual dds::pub::NoOpPublisherListener
{ };

class TestSubscriberListener : public virtual dds::sub::NoOpSubscriberListener
{ };

class TestDataReaderListener : public virtual dds::sub::NoOpDataReaderListener<HelloWorldData::Msg>
{ };

class TestDataWriterListener : public virtual dds::pub::NoOpDataWriterListener<HelloWorldData::Msg>
{ };

} } } }


/**
 * Fixture for listener tests
 */
class ddscxx_listener_setup : public ::testing::Test
{
public:
    dds::domain::DomainParticipant dp;
    dds::topic::Topic<HelloWorldData::Msg> topic;
    dds::pub::Publisher pub;
    dds::pub::DataWriter<HelloWorldData::Msg> dw;
    dds::sub::Subscriber sub;
    dds::sub::DataReader<HelloWorldData::Msg> dr;
    dds::core::status::StatusMask mask;
    lite::ddscxx::tests::listener_setup::TestDomainParticipantListener participantListener;
    lite::ddscxx::tests::listener_setup::TestTopicListener topicListener;
    lite::ddscxx::tests::listener_setup::TestPublisherListener pubListener;
    lite::ddscxx::tests::listener_setup::TestSubscriberListener subListener;
    lite::ddscxx::tests::listener_setup::TestDataWriterListener dwListener;
    lite::ddscxx::tests::listener_setup::TestDataReaderListener drListener;

    ddscxx_listener_setup() :
        dp(dds::core::null),
        topic(dds::core::null),
        pub(dds::core::null),
        dw(dds::core::null),
        sub(dds::core::null),
        dr(dds::core::null),
        mask(dds::core::status::StatusMask()) { }

    void SetUp() {
        // Create participant
        dp = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(dp, dds::core::null);

        // Create topic
        topic = dds::topic::Topic<HelloWorldData::Msg>(dp, "topic_name");
        ASSERT_NE(topic, dds::core::null);

        // Create Publisher
        pub = dds::pub::Publisher(dp);
        ASSERT_NE(pub, dds::core::null);

        // Create subscriber
        sub = dds::sub::Subscriber(dp);
        ASSERT_NE(sub, dds::core::null);
    }

    void createWriter() {
        // Create data writer
        dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic);
        ASSERT_NE(dw, dds::core::null);
    }

    void createReader() {
        // Create data reader
        dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic);
        ASSERT_NE(dr, dds::core::null);
    }

    void TearDown() {
    }
};

/*
    PARTICIPANT
*/

/**
 * Create participant without listener, set listener on participant, retrieve listener and
 * check if correct listener is returned. Then delete the participant with the listener.
 */
DDSCXX_TEST_F(ddscxx_listener_setup, participant_without_listener)
{
    // Listener should not be set
    dds::domain::DomainParticipantListener *listener1 = dp.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener
    dp.listener(&participantListener, mask);

    // Get listener and check if it equals listener used when setting
    dds::domain::DomainParticipantListener *listener2 = dp.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &participantListener);

    // Delete participant with listener
    dp = dds::core::null;
    ASSERT_EQ(dp, dds::core::null);
}


/**
 * Create participant with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
DDSCXX_TEST_F(ddscxx_listener_setup, participant_with_listener)
{
    // Create participant with a listener
    dp = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id(),
        dds::domain::qos::DomainParticipantQos(), &participantListener, mask);
    ASSERT_NE(dp, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::domain::DomainParticipantListener *listener1 = dp.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &participantListener);

    // Set listener to null
    dp.listener(NULL, mask);
    dds::domain::DomainParticipantListener *listener2 = dp.listener();
    ASSERT_EQ(listener2, nullptr);
}

/**
 * Set the listener on a domain participant after the entity has been closed
 */
DDSCXX_TEST_F(ddscxx_listener_setup, already_closed_participant)
{
    // Close the participant
    dp.close();

    ASSERT_THROW({
        // Set listener
        dp.listener(&participantListener, mask);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        dp.listener();
    }, dds::core::AlreadyClosedError);
}

/*
    TOPIC
*/

/**
 * Create topic without listener, set listener on the topic, retrieve listener and
 * check if correct listener is returned. Then delete the topic with the listener.
 */
DDSCXX_TEST_F(ddscxx_listener_setup, topic_without_listener)
{
    // Listener should not be set
    dds::topic::TopicListener<HelloWorldData::Msg> *listener1 = topic.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener on topic
    topic.listener(&topicListener, mask);

    // Get listener and check if it equals listener used when setting
    dds::topic::TopicListener<HelloWorldData::Msg> *listener2 = topic.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &topicListener);

    // Delete topic with listener
    topic = dds::core::null;
    ASSERT_EQ(topic, dds::core::null);
}


/**
 * Create topic with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
DDSCXX_TEST_F(ddscxx_listener_setup, topic_with_listener)
{
    // Currently, cyclonedds has some 'duplicate topic' deletion behaviour
    // that doesn't work for isocpp when assigning a new topic to the exact
    // same topic (which we try in this test). To circumvent that, remove
    // the first topic so that there is no duplicate.
    topic = dds::core::null;

    // Create topic with listener
    topic = dds::topic::Topic<HelloWorldData::Msg>(dp, "topic_name",
            dds::topic::qos::TopicQos(), &topicListener, mask);
    ASSERT_NE(topic, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::topic::TopicListener<HelloWorldData::Msg> *listener1 = topic.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &topicListener);

    // Set listener to null
    topic.listener(NULL, mask);
    dds::topic::TopicListener<HelloWorldData::Msg> *listener2 = topic.listener();
    ASSERT_EQ(listener2, nullptr);
}

/**
 * Set the listener on a topic after the entity has been closed
 */
DDSCXX_TEST_F(ddscxx_listener_setup, already_closed_topic)
{
    // Close the topic
    topic.close();

    ASSERT_THROW({
        // Set listener
        topic.listener(&topicListener, mask);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        topic.listener();
    }, dds::core::AlreadyClosedError);
}


/*
    PUBLISHER
*/

/**
 * Create publisher without listener, set listener on the publisher, retrieve listener and
 * check if correct listener is returned. Then delete the topic with the listener.
 */
DDSCXX_TEST_F(ddscxx_listener_setup, publisher_without_listener)
{
    // Listener should not be set
    dds::pub::PublisherListener *listener1 = pub.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener on publisher
    pub.listener(&pubListener, mask);

    // Get listener and check if it equals listener used when setting
    dds::pub::PublisherListener *listener2 = pub.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &pubListener);

    // Delete publisher with listener
    pub = dds::core::null;
    ASSERT_EQ(pub, dds::core::null);
}


/**
 * Create publisher with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
DDSCXX_TEST_F(ddscxx_listener_setup, publisher_with_listener)
{
    // Create publisher with listener
    pub = dds::pub::Publisher(dp, dds::pub::qos::PublisherQos(), &pubListener, mask);
    ASSERT_NE(pub, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::pub::PublisherListener *listener1 = pub.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &pubListener);

    // Set listener to null
    pub.listener(NULL, mask);
    dds::pub::PublisherListener *listener2 = pub.listener();
    ASSERT_EQ(listener2, nullptr);
}

/**
 * Set the listener on a publisher after the entity has been closed
 */
DDSCXX_TEST_F(ddscxx_listener_setup, already_closed_publisher)
{
    // Close the publisher
    pub.close();

    ASSERT_THROW({
        // Set listener
        pub.listener(&pubListener, mask);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        pub.listener();
    }, dds::core::AlreadyClosedError);
}

/*
    SUBSCRIBER
*/

/**
 * Create subscriber without listener, set listener on the subscriber, retrieve listener and
 * check if correct listener is returned. Then delete the topic with the listener.
 */
DDSCXX_TEST_F(ddscxx_listener_setup, subscriber_without_listener)
{
    // Listener should not be set
    dds::sub::SubscriberListener *listener1 = sub.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener on subscriber
    sub.listener(&subListener, mask);

    // Get listener and check if it equals listener used when setting
    dds::sub::SubscriberListener *listener2 = sub.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &subListener);

    // Delete subscriber with listener
    sub = dds::core::null;
    ASSERT_EQ(sub, dds::core::null);
}

/**
 * Create subscriber with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
DDSCXX_TEST_F(ddscxx_listener_setup, subscriber_with_listener)
{
    // Create subscriber with listener
    sub = dds::sub::Subscriber(dp, dds::sub::qos::SubscriberQos(), &subListener, mask);
    ASSERT_NE(sub, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::sub::SubscriberListener *listener1 = sub.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &subListener);

    // Set listener to null
    sub.listener(NULL, mask);
    dds::sub::SubscriberListener *listener2 = sub.listener();
    ASSERT_EQ(listener2, nullptr);
}

/**
 * Set the listener on a subscriber after the entity has been closed
 */
DDSCXX_TEST_F(ddscxx_listener_setup, already_closed_subscriber)
{
    // Close the subscriber
    sub.close();

    ASSERT_THROW({
        // Set listener
        sub.listener(&subListener, mask);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        sub.listener();
    }, dds::core::AlreadyClosedError);
}

/*
    WRITER
*/

/**
 * Create data writer without listener, set listener on the writer, retrieve listener and
 * check if correct listener is returned. Then delete the topic with the listener.
 */
DDSCXX_TEST_F(ddscxx_listener_setup, writer_without_listener)
{
    createWriter();

    // Listener should not be set
    dds::pub::DataWriterListener<HelloWorldData::Msg> *listener1 = dw.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener on writer
    dw.listener(&dwListener, mask);

    // Get listener and check if it equals listener used when setting
    dds::pub::DataWriterListener<HelloWorldData::Msg> *listener2 = dw.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &dwListener);

    // Delete data writer with listener
    dw = dds::core::null;
    ASSERT_EQ(dw, dds::core::null);
}


/**
 * Create data writer with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
DDSCXX_TEST_F(ddscxx_listener_setup, writer_with_listener)
{
    // Create data writer with listener
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic,
            dds::pub::qos::DataWriterQos(), &dwListener, mask);
    ASSERT_NE(dw, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::pub::DataWriterListener<HelloWorldData::Msg> *listener1 = dw.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &dwListener);

    // Set listener to null
    dw.listener(NULL, mask);
    dds::pub::DataWriterListener<HelloWorldData::Msg> *listener2 = dw.listener();
    ASSERT_EQ(listener2, nullptr);
}


/**
 * Set the listener on a writer after the entity has been closed
 */
DDSCXX_TEST_F(ddscxx_listener_setup, already_closed_writer)
{
    createWriter();

    // Close the writer
    dw.close();

    ASSERT_THROW({
        // Set listener
        dw.listener(&dwListener, mask);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        dw.listener();
    }, dds::core::AlreadyClosedError);
}


/*
    READER
*/

/**
 * Create reader without listener, set listener on the reader, retrieve listener and
 * check if correct listener is returned. Then delete the topic with the listener.
 */
DDSCXX_TEST_F(ddscxx_listener_setup, reader_without_listener)
{
    createReader();

    // Listener should not be set
    dds::sub::DataReaderListener<HelloWorldData::Msg> *listener1 = dr.listener();
    ASSERT_EQ(listener1, nullptr);

    // Set listener on reader
    dr.listener(&drListener, mask);

    // Get listener and check if it equals listener used when setting
    dds::sub::DataReaderListener<HelloWorldData::Msg> *listener2 = dr.listener();
    ASSERT_NE(listener2, nullptr);
    ASSERT_EQ(listener2, &drListener);

    // Delete data reader with listener
    dr = dds::core::null;
    ASSERT_EQ(dr, dds::core::null);
}


/**
 * Create data reader with listener, retrieve listener and check if correct
 * listener is returned. Then set the listener to null and check if no listener
 * is returned
 */
DDSCXX_TEST_F(ddscxx_listener_setup, reader_with_listener)
{
    // Create data reader with listener
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic,
            dds::sub::qos::DataReaderQos(), &drListener, mask);
    ASSERT_NE(dr, dds::core::null);

    // Get listener and check if it equals listener used when setting
    dds::sub::DataReaderListener<HelloWorldData::Msg> *listener1 = dr.listener();
    ASSERT_NE(listener1, nullptr);
    ASSERT_EQ(listener1, &drListener);

    // Set listener to null
    dr.listener(NULL, mask);
    dds::sub::DataReaderListener<HelloWorldData::Msg> *listener2 = dr.listener();
    ASSERT_EQ(listener2, nullptr);
}


/**
 * Set the listener on a reader after the entity has been closed
 */
DDSCXX_TEST_F(ddscxx_listener_setup, already_closed_reader)
{
    createReader();

    // Close the reader
    dr.close();

    ASSERT_THROW({
        // Set listener
        dr.listener(&drListener, mask);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        // Get listener
        dr.listener();
    }, dds::core::AlreadyClosedError);
}



