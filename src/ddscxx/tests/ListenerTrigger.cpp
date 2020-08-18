#include <chrono>
#include <thread>

#include "dds/dds.hpp"
#include "dds/ddscxx/test.h"

#include "dds/ddsrt/cdtors.h"
#include "dds/ddsrt/misc.h"
#include "dds/ddsrt/process.h"
#include "dds/ddsrt/sync.h"
#include "dds/ddsrt/threads.h"

#include "HelloWorldData_DCPS.hpp"

static uint32_t cb_called = 0;
static ddsrt_mutex_t g_mutex;
static ddsrt_cond_t g_cond;


namespace lite { namespace ddscxx { namespace tests { namespace listener_trigger {

class TestDomainParticipantListener : public virtual dds::domain::NoOpDomainParticipantListener
{
public:
    dds::sub::Subscriber data_on_readers_subscriber;

    dds::sub::AnyDataReader data_available_reader;

    TestDomainParticipantListener() :
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

class TestTopicListener : public virtual dds::topic::NoOpTopicListener<HelloWorldData::Msg>
{ };

class TestPublisherListener : public virtual dds::pub::NoOpPublisherListener
{
public:
    dds::pub::AnyDataWriter publication_matched_writer;
    dds::core::status::PublicationMatchedStatus publication_matched_status;

    TestPublisherListener() :
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

class TestSubscriberListener : public virtual dds::sub::NoOpSubscriberListener
{
public:
    dds::sub::AnyDataReader subscription_matched_reader;
    dds::core::status::SubscriptionMatchedStatus subscription_matched_status;

    dds::sub::Subscriber data_on_readers_subscriber;

    dds::sub::AnyDataReader data_available_reader;

    TestSubscriberListener() :
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

class TestDataWriterListener : public virtual dds::pub::NoOpDataWriterListener<HelloWorldData::Msg>
{
public:
    dds::pub::DataWriter<HelloWorldData::Msg> offered_incompatible_qos_writer;
    dds::core::status::OfferedIncompatibleQosStatus offered_incompatible_qos_status;

    dds::pub::DataWriter<HelloWorldData::Msg> publication_matched_writer;
    dds::core::status::PublicationMatchedStatus publication_matched_status;

    TestDataWriterListener() :
        offered_incompatible_qos_writer(dds::core::null),
        publication_matched_writer(dds::core::null) { }

protected:
    virtual void on_offered_incompatible_qos(dds::pub::DataWriter<HelloWorldData::Msg>& writer,
        const dds::core::status::OfferedIncompatibleQosStatus& status)
    {
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

class TestDataReaderListener : public virtual dds::sub::NoOpDataReaderListener<HelloWorldData::Msg>
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

    TestDataReaderListener() :
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

} } } }


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

/**
 * Fixture for listener trigger tests
 */
class ddscxx_listener_trigger : public ::testing::Test
{
public:
    dds::domain::DomainParticipant dp;
    dds::topic::Topic<HelloWorldData::Msg> topic;
    dds::pub::Publisher pub;
    dds::sub::Subscriber sub;
    dds::sub::DataReader<HelloWorldData::Msg> dr;
    dds::pub::DataWriter<HelloWorldData::Msg> dw;
    dds::sub::qos::DataReaderQos readerQos;
    dds::pub::qos::DataWriterQos writerQos;
    dds::core::status::StatusMask dpMask;
    dds::core::status::StatusMask pubMask;
    dds::core::status::StatusMask subMask;
    dds::core::status::StatusMask writerMask;
    dds::core::status::StatusMask readerMask;

    lite::ddscxx::tests::listener_trigger::TestDomainParticipantListener participantListener;
    lite::ddscxx::tests::listener_trigger::TestTopicListener topicListener;
    lite::ddscxx::tests::listener_trigger::TestPublisherListener pubListener;
    lite::ddscxx::tests::listener_trigger::TestSubscriberListener subListener;
    lite::ddscxx::tests::listener_trigger::TestDataReaderListener readerListener;
    lite::ddscxx::tests::listener_trigger::TestDataWriterListener writerListener;

    ddscxx_listener_trigger() :
        dp(dds::core::null),
        topic(dds::core::null),
        pub(dds::core::null),
        sub(dds::core::null),
        dr(dds::core::null),
        dw(dds::core::null),
        dpMask(dds::core::status::StatusMask()),
        pubMask(dds::core::status::StatusMask()),
        subMask(dds::core::status::StatusMask()),
        writerMask(dds::core::status::StatusMask()),
        readerMask(dds::core::status::StatusMask())
    {
        readerQos = dds::sub::qos::DataReaderQos()
                    << dds::core::policy::Reliability::Reliable(dds::core::Duration(1, 0))
                    << dds::core::policy::History::KeepAll();
        writerQos = dds::pub::qos::DataWriterQos()
            << dds::core::policy::Reliability::Reliable(dds::core::Duration(1, 0))
            << dds::core::policy::History::KeepAll();
    }

    void SetUp() {
        ddsrt_mutex_init(&g_mutex);
        ddsrt_cond_init(&g_cond);

        // Create participant
        dp = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(dp, dds::core::null);

        // Create topic
        topic = dds::topic::Topic<HelloWorldData::Msg>(dp, "topic");
        ASSERT_NE(topic, dds::core::null);

        // Create publisher
        pub = dds::pub::Publisher(dp);
        ASSERT_NE(pub, dds::core::null);

        // Create subscriber
        sub = dds::sub::Subscriber(dp);
        ASSERT_NE(sub, dds::core::null);

        reset_cb();
    }

    void TearDown() {
        if (dr != dds::core::null) {
            dr.close();
            dr = dds::core::null;
        }
        if (dw != dds::core::null) {
            dw.close();
            dw = dds::core::null;
        }
        sub = dds::core::null;
        pub = dds::core::null;

        ddsrt_cond_destroy(&g_cond);
        ddsrt_mutex_destroy(&g_mutex);
    }
};


// TODO:
// this test does not work because listener on publisher is triggered when writer is created
// but before ddsc handle for writer is stored in the writer entity.
DDSCXX_TEST_F(ddscxx_listener_trigger, DISABLED_propagation)
{
    // Set listener on participant
    dpMask << dds::core::status::StatusMask::data_on_readers();
    dp.listener(&participantListener, dpMask);

    // Set listener on publisher
    pubMask << dds::core::status::StatusMask::publication_matched();
    pub.listener(&pubListener, pubMask);

    // Set listener on subscriber
    subMask << dds::core::status::StatusMask::subscription_matched();
    sub.listener(&subListener, subMask);

    // Create reader and writer without listener
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos);
    ASSERT_NE(dr, dds::core::null);

    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos);
    ASSERT_NE(dw, dds::core::null);

    // Publication and Subscription should be matched.
    uint32_t triggered = waitfor_cb(DDS_PUBLICATION_MATCHED_STATUS | DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_SUBSCRIPTION_MATCHED_STATUS, DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_PUBLICATION_MATCHED_STATUS,  DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(pubListener.publication_matched_writer.delegate(), dw.delegate());
    ASSERT_EQ(subListener.subscription_matched_reader.delegate(), dr.delegate());

    // Write sample
    HelloWorldData::Msg msgInstance(1, "test");
    dw << msgInstance;

    // Data on readers should be triggered with the right status.
    triggered = waitfor_cb(DDS_DATA_ON_READERS_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_ON_READERS_STATUS, DDS_DATA_ON_READERS_STATUS);
    ASSERT_EQ(pubListener.publication_matched_writer.delegate(), dw.delegate());
    ASSERT_EQ(participantListener.data_on_readers_subscriber.delegate(), sub.delegate());
}

DDSCXX_TEST_F(ddscxx_listener_trigger, publication_subscription_matched)
{
    // Create reader with listener
    readerMask << dds::core::status::StatusMask::subscription_matched();
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos, &readerListener, readerMask);
    ASSERT_NE(dr, dds::core::null);

    // Create writer with listener
    writerMask << dds::core::status::StatusMask::publication_matched();
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos, &writerListener, writerMask);
    ASSERT_NE(dw, dds::core::null);

    // Publication and Subscription should be matched.
    uint32_t triggered = waitfor_cb(DDS_SUBSCRIPTION_MATCHED_STATUS | DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_SUBSCRIPTION_MATCHED_STATUS, DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_PUBLICATION_MATCHED_STATUS, DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(writerListener.publication_matched_writer.delegate(), dw.delegate());
    ASSERT_EQ(readerListener.subscription_matched_reader.delegate(), dr.delegate());
}

DDSCXX_TEST_F(ddscxx_listener_trigger, publication_matched)
{
    // Create writer with listener
    writerMask << dds::core::status::StatusMask::publication_matched();
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos, &writerListener, writerMask);
    ASSERT_NE(dw, dds::core::null);

    // Create reader
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos);
    ASSERT_NE(dr, dds::core::null);

    dds_return_t ret;
    dds_instance_handle_t dr_instance_handle;
    ret = dds_get_instance_handle(dr.delegate()->get_ddsc_entity(), &dr_instance_handle);
    ASSERT_EQ(ret, DDS_RETCODE_OK);

    uint32_t triggered = waitfor_cb(DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_PUBLICATION_MATCHED_STATUS, DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(writerListener.publication_matched_writer.delegate(), dw.delegate());
    ASSERT_EQ(writerListener.publication_matched_status.current_count(), 1);
    ASSERT_EQ(writerListener.publication_matched_status.current_count_change(), 1);
    ASSERT_EQ(writerListener.publication_matched_status.total_count(), 1);
    ASSERT_EQ(writerListener.publication_matched_status.total_count_change(), 1);
    ASSERT_EQ(writerListener.publication_matched_status.last_subscription_handle(), dr_instance_handle);

    /* The listener should have reset the count_change variables. */
    dds::core::status::PublicationMatchedStatus status = dw.publication_matched_status();
    ASSERT_EQ(status.current_count(), 1);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_subscription_handle(), dr_instance_handle);

    /* Reset the trigger flags. */
    reset_cb();

    /* Un-match the publication by deleting the reader. */
    dr = dds::core::null;

    /* Publication matched should be triggered with the right status. */
    triggered = waitfor_cb(DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_PUBLICATION_MATCHED_STATUS, DDS_PUBLICATION_MATCHED_STATUS);
    ASSERT_EQ(writerListener.publication_matched_writer.delegate(), dw.delegate());
    ASSERT_EQ(writerListener.publication_matched_status.current_count(), 0);
    ASSERT_EQ(writerListener.publication_matched_status.current_count_change(), -1);
    ASSERT_EQ(writerListener.publication_matched_status.total_count(), 1);
    ASSERT_EQ(writerListener.publication_matched_status.total_count_change(), 0);
    ASSERT_EQ(writerListener.publication_matched_status.last_subscription_handle(), dr_instance_handle);

    /* The listener should have reset the count_change variables. */
    status = dw.publication_matched_status();
    ASSERT_EQ(status.current_count(), 0);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_subscription_handle(), dr_instance_handle);
}

DDSCXX_TEST_F(ddscxx_listener_trigger, subscription_matched)
{
    // Create reader with listener
    readerMask << dds::core::status::StatusMask::subscription_matched();
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos, &readerListener, readerMask);
    ASSERT_NE(dr, dds::core::null);

    // Create writer
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos);
    ASSERT_NE(dw, dds::core::null);

    dds_return_t ret;
    dds_instance_handle_t dw_instance_handle;
    ret = dds_get_instance_handle(dw.delegate()->get_ddsc_entity(), &dw_instance_handle);
    ASSERT_EQ(ret, DDS_RETCODE_OK);

    uint32_t triggered = waitfor_cb(DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_SUBSCRIPTION_MATCHED_STATUS, DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(readerListener.subscription_matched_reader.delegate(), dr.delegate());
    ASSERT_EQ(readerListener.subscription_matched_status.current_count(), 1);
    ASSERT_EQ(readerListener.subscription_matched_status.current_count_change(), 1);
    ASSERT_EQ(readerListener.subscription_matched_status.total_count(), 1);
    ASSERT_EQ(readerListener.subscription_matched_status.total_count_change(), 1);
    ASSERT_EQ(readerListener.subscription_matched_status.last_publication_handle(), dw_instance_handle);

    /* The listener should have reset the count_change variables. */
    dds::core::status::SubscriptionMatchedStatus status = dr.subscription_matched_status();
    ASSERT_EQ(status.current_count(), 1);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), dw_instance_handle);

    /* Reset the trigger flags. */
    reset_cb();

    /* Un-match the subscription by deleting the writer. */
    dw = dds::core::null;

    /* Subscription matched should be triggered with the right status. */
    triggered = waitfor_cb(DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(triggered & DDS_SUBSCRIPTION_MATCHED_STATUS, DDS_SUBSCRIPTION_MATCHED_STATUS);
    ASSERT_EQ(readerListener.subscription_matched_reader.delegate(), dr.delegate());
    ASSERT_EQ(readerListener.subscription_matched_status.current_count(), 0);
    ASSERT_EQ(readerListener.subscription_matched_status.current_count_change(), -1);
    ASSERT_EQ(readerListener.subscription_matched_status.total_count(), 1);
    ASSERT_EQ(readerListener.subscription_matched_status.total_count_change(), 0);
    ASSERT_EQ(readerListener.subscription_matched_status.last_publication_handle(), dw_instance_handle);

    /* The listener should have reset the count_change variables. */
    status = dr.subscription_matched_status();
    ASSERT_EQ(status.current_count(), 0);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), dw_instance_handle);
}

DDSCXX_TEST_F(ddscxx_listener_trigger, incompatible_qos)
{
    // Create writer with listener
    writerMask << dds::core::status::StatusMask::offered_incompatible_qos();
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos, &writerListener, writerMask);
    ASSERT_NE(dw, dds::core::null);

    // Create reader with listener
    readerMask << dds::core::status::StatusMask::requested_incompatible_qos();
    readerQos << dds::core::policy::Durability::Persistent();
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos, &readerListener, readerMask);
    ASSERT_NE(dr, dds::core::null);

    /* Incompatible QoS should be triggered with the right status. */
    uint32_t triggered = waitfor_cb(DDS_OFFERED_INCOMPATIBLE_QOS_STATUS | DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS);
    ASSERT_EQ(triggered & DDS_OFFERED_INCOMPATIBLE_QOS_STATUS, DDS_OFFERED_INCOMPATIBLE_QOS_STATUS);
    ASSERT_EQ(triggered & DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS, DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS);
    ASSERT_EQ(writerListener.offered_incompatible_qos_writer.delegate(), dw.delegate());
    ASSERT_EQ(writerListener.offered_incompatible_qos_status.total_count(), 1);
    ASSERT_EQ(writerListener.offered_incompatible_qos_status.total_count_change(), 1);
    ASSERT_EQ(writerListener.offered_incompatible_qos_status.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);
    ASSERT_EQ(readerListener.requested_incompatible_qos_reader.delegate(), dr.delegate());
    ASSERT_EQ(readerListener.requested_incompatible_qos_status.total_count(), 1);
    ASSERT_EQ(readerListener.requested_incompatible_qos_status.total_count_change(), 1);
    ASSERT_EQ(readerListener.requested_incompatible_qos_status.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);

    // Test if status is swallowed
    dds::core::status::StatusMask writerStatus = dw.status_changes();
    ASSERT_FALSE(writerStatus.test(DDS_OFFERED_INCOMPATIBLE_QOS_STATUS_ID));
    dds::core::status::StatusMask readerStatus = dr.status_changes();
    ASSERT_FALSE(readerStatus.test(DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS_ID));

    /* The listener should have reset the count_change variables. */
    dds::core::status::OfferedIncompatibleQosStatus w_status = dw.offered_incompatible_qos_status();
    ASSERT_EQ(w_status.total_count(), 1);
    ASSERT_EQ(w_status.total_count_change(), 0);
    ASSERT_EQ(w_status.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);
    dds::core::status::RequestedIncompatibleQosStatus r_status = dr.requested_incompatible_qos_status();
    ASSERT_EQ(r_status.total_count(), 1);
    ASSERT_EQ(r_status.total_count_change(), 0);
    ASSERT_EQ(r_status.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);
}

DDSCXX_TEST_F(ddscxx_listener_trigger, data_available)
{
    uint32_t triggered;

    // Create reader with listener
    readerMask << dds::core::status::StatusMask::data_available();
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos, &readerListener, readerMask);
    ASSERT_NE(dr, dds::core::null);

    // Create writer
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos);
    ASSERT_NE(dw, dds::core::null);

    // Write sample
    HelloWorldData::Msg msgInstance(1, "test");
    dw << msgInstance;

    // Data available should be triggered on reader
    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(readerListener.data_available_reader.delegate(), dr.delegate());

    // Reset the trigger flags.
    reset_cb();
    readerListener.data_available_reader = dds::core::null;

    // Add data-available listener to subscriber
    subMask << dds::core::status::StatusMask::data_available();
    sub.listener(&subListener, subMask);

    // Write another sample
    dw << msgInstance;

    // Data available should still be triggered on reader, not on subscriber
    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(readerListener.data_available_reader.delegate(), dr.delegate());
    ASSERT_EQ(subListener.data_available_reader, dds::core::null);

    // Reset the trigger flags.
    reset_cb();
    readerListener.data_available_reader = dds::core::null;

    // Deleting the writer should trigger DATA_AVAILABLE as well
    dw = dds::core::null;

    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(readerListener.data_available_reader.delegate(), dr.delegate());

    // The listener should have swallowed the status on reader
    dds::core::status::StatusMask readerStatus = dr.status_changes();
    ASSERT_FALSE(readerStatus.test(DDS_DATA_AVAILABLE_STATUS_ID));
}

DDSCXX_TEST_F(ddscxx_listener_trigger, data_available_subscriber)
{
    // Add data-available listener to subscriber
    subMask << dds::core::status::StatusMask::data_available();
    sub.listener(&subListener, subMask);

    // Create reader without listener
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos);
    ASSERT_NE(dr, dds::core::null);

    // Create writer
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos);
    ASSERT_NE(dw, dds::core::null);

    // Write sample
    HelloWorldData::Msg msgInstance(1, "test");
    dw << msgInstance;

    // Data available should be triggered on subscriber
    uint32_t triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(subListener.data_available_reader.delegate(), dr.delegate());

    // Event should not be triggered on reader
    ASSERT_EQ(readerListener.data_available_reader, dds::core::null);
}

DDSCXX_TEST_F(ddscxx_listener_trigger, data_available_participant)
{
    uint32_t triggered;

    // Add data-available listener to participant
    dpMask << dds::core::status::StatusMask::data_available();
    dp.listener(&participantListener, dpMask);

    // Create reader without listener
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos);
    ASSERT_NE(dr, dds::core::null);

    // Create writer
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos);
    ASSERT_NE(dw, dds::core::null);

    // Write sample
    HelloWorldData::Msg msgInstance(1, "test");
    dw << msgInstance;

    // Data available should be triggered on participant
    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(participantListener.data_available_reader.delegate(), dr.delegate());

    // Event should not be triggered on reader or subscriber
    ASSERT_EQ(readerListener.data_available_reader, dds::core::null);
    ASSERT_EQ(subListener.data_available_reader, dds::core::null);

    // Reset the trigger flags.
    reset_cb();
    participantListener.data_available_reader = dds::core::null;

    // Add data-available listener to subscriber
    subMask << dds::core::status::StatusMask::data_available();
    sub.listener(&subListener, subMask);

    // Write sample
    dw << msgInstance;

    // Data available should be triggered on subscriber, not on participant
    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(subListener.data_available_reader.delegate(), dr.delegate());
    ASSERT_EQ(participantListener.data_available_reader, dds::core::null);

    // Reset the trigger flags.
    reset_cb();
    subListener.data_available_reader = dds::core::null;

    // Remove data-available listener from subscriber
    sub.listener(NULL, dds::core::status::StatusMask::none());

    // Write sample
    dw << msgInstance;

    // Data available should be triggered on participant again
    triggered = waitfor_cb(DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
    ASSERT_EQ(participantListener.data_available_reader.delegate(), dr.delegate());
}


DDSCXX_TEST_F(ddscxx_listener_trigger, data_on_readers)
{
    // Add listener to subscriber
    subMask << dds::core::status::StatusMask::data_on_readers();
    sub.listener(&subListener, subMask);

    // Create reader with data-available listener (this should not sabotage
    // the data_on_readers on the subscriber because these events are mutual exclusive)
    readerMask << dds::core::status::StatusMask::data_available();
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos, &readerListener, readerMask);
    ASSERT_NE(dr, dds::core::null);

    // Create writer
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos);
    ASSERT_NE(dw, dds::core::null);

    // Write sample
    HelloWorldData::Msg msgInstance(1, "test");
    dw << msgInstance;

    // Data on readers should be triggered with the right status
    uint32_t triggered = waitfor_cb(DDS_DATA_ON_READERS_STATUS);
    ASSERT_EQ(triggered & DDS_DATA_ON_READERS_STATUS, DDS_DATA_ON_READERS_STATUS);
    ASSERT_EQ(subListener.data_on_readers_subscriber.delegate(), sub.delegate());
    ASSERT_NE(triggered & DDS_DATA_AVAILABLE_STATUS, DDS_DATA_AVAILABLE_STATUS);
}

DDSCXX_TEST_F(ddscxx_listener_trigger, sample_lost)
{
    // Create reader with listener
    readerMask << dds::core::status::StatusMask::sample_lost();
    readerQos << dds::core::policy::DestinationOrder::SourceTimestamp();
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos, &readerListener, readerMask);
    ASSERT_NE(dr, dds::core::null);

    // Create writer
    writerQos << dds::core::policy::DestinationOrder::SourceTimestamp();
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos);
    ASSERT_NE(dw, dds::core::null);

    // Write sample
    HelloWorldData::Msg msg(1, "test1");
    dds::core::Time current_time = dp.current_time();
    dds::core::Time the_past = current_time - dds::core::Duration::from_secs(1000000);
    dw.write(msg, current_time);
    dw.write(msg, the_past);

    // Sample lost should be triggered with the right status
    uint32_t triggered = waitfor_cb(DDS_SAMPLE_LOST_STATUS);
    ASSERT_EQ(triggered & DDS_SAMPLE_LOST_STATUS, DDS_SAMPLE_LOST_STATUS);
    ASSERT_EQ(readerListener.sample_lost_reader.delegate(), dr.delegate());
    ASSERT_EQ(readerListener.sample_lost_status.total_count(), 1);
    ASSERT_EQ(readerListener.sample_lost_status.total_count_change(), 1);

    // The listener should have swallowed the status on reader
    dds::core::status::StatusMask readerStatus = dr.status_changes();
    ASSERT_FALSE(readerStatus.test(DDS_SAMPLE_LOST_STATUS_ID));

    /* The listener should have reset the count_change variables. */
    dds::core::status::SampleLostStatus status = dr.sample_lost_status();
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
}

DDSCXX_TEST_F(ddscxx_listener_trigger, sample_rejected)
{
    dds::core::status::SampleRejectedState reason =
    dds::core::status::SampleRejectedState::rejected_by_samples_limit();

    // Create reader with listener
    readerMask << dds::core::status::StatusMask::sample_rejected();
    readerQos
        << dds::core::policy::Reliability::BestEffort(dds::core::Duration(0, DDS_MSECS(100)))
        << dds::core::policy::ResourceLimits(1, 1, 1);
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos, &readerListener, readerMask);
    ASSERT_NE(dr, dds::core::null);

    // Create writer
    writerQos
        << dds::core::policy::Reliability::BestEffort(dds::core::Duration(0, DDS_MSECS(100)));
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos);
    ASSERT_NE(dw, dds::core::null);

    // Write samples (more than resource limit)
    HelloWorldData::Msg msg(1, "test1");
    dw << msg;
    dw << msg;
    dw << msg;

    // Sample rejected should be triggered with the right status
    uint32_t triggered = waitfor_cb(DDS_SAMPLE_REJECTED_STATUS);
    ASSERT_EQ(triggered & DDS_SAMPLE_REJECTED_STATUS, DDS_SAMPLE_REJECTED_STATUS);
    ASSERT_EQ(readerListener.sample_rejected_reader.delegate(), dr.delegate());
    ASSERT_EQ(readerListener.sample_rejected_status.total_count(), 2);
    ASSERT_EQ(readerListener.sample_rejected_status.total_count_change(), 1);
    ASSERT_EQ(readerListener.sample_rejected_status.last_reason(), reason);
    ASSERT_EQ(readerListener.sample_rejected_status.last_instance_handle(), dr.lookup_instance(msg));

    // The listener should have swallowed the status on reader
    dds::core::status::StatusMask readerStatus = dr.status_changes();
    ASSERT_FALSE(readerStatus.test(DDS_SAMPLE_REJECTED_STATUS_ID));

    /* The listener should have reset the count_change variables. */
    dds::core::status::SampleRejectedStatus status = dr.sample_rejected_status();
    ASSERT_EQ(status.total_count(), 2);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_reason(), reason);
    ASSERT_EQ(status.last_instance_handle(), dr.lookup_instance(msg));
}


DDSCXX_TEST_F(ddscxx_listener_trigger, liveliness_changed)
{
    // Create reader with listener
    readerMask << dds::core::status::StatusMask::liveliness_changed();
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos, &readerListener, readerMask);
    ASSERT_NE(dr, dds::core::null);

    // Create writer
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos);
    ASSERT_NE(dw, dds::core::null);

    dds_return_t ret;
    dds_instance_handle_t dw_instance_handle;
    ret = dds_get_instance_handle(dw.delegate()->get_ddsc_entity(), &dw_instance_handle);
    ASSERT_EQ(ret, DDS_RETCODE_OK);

    // Liveliness changed should be triggered with the right status
    uint32_t triggered = waitfor_cb(DDS_LIVELINESS_CHANGED_STATUS);
    ASSERT_EQ(triggered & DDS_LIVELINESS_CHANGED_STATUS, DDS_LIVELINESS_CHANGED_STATUS);
    ASSERT_EQ(readerListener.liveliness_changed_reader.delegate(), dr.delegate());
    ASSERT_EQ(readerListener.liveliness_changed_status.alive_count(), 1);
    ASSERT_EQ(readerListener.liveliness_changed_status.alive_count_change(), 1);
    ASSERT_EQ(readerListener.liveliness_changed_status.not_alive_count(), 0);
    ASSERT_EQ(readerListener.liveliness_changed_status.not_alive_count_change(), 0);
    ASSERT_EQ(readerListener.liveliness_changed_status.last_publication_handle(), dw_instance_handle);

    /* The listener should have reset the count_change variables. */
    dds::core::status::LivelinessChangedStatus status = dr.liveliness_changed_status();
    ASSERT_EQ(status.alive_count(), 1);
    ASSERT_EQ(status.alive_count_change(), 0);
    ASSERT_EQ(status.not_alive_count(), 0);
    ASSERT_EQ(status.not_alive_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), dw_instance_handle);

    // The listener should have swallowed the status on reader
    dds::core::status::StatusMask readerStatus = dr.status_changes();
    ASSERT_FALSE(readerStatus.test(DDS_LIVELINESS_CHANGED_STATUS_ID));

    /* Reset the trigger flags. */
    reset_cb();

    /* Change liveliness again by deleting the writer. */
    dw = dds::core::null;

    // Liveliness changed should be triggered with the right status
    triggered = waitfor_cb(DDS_LIVELINESS_CHANGED_STATUS);
    ASSERT_EQ(triggered & DDS_LIVELINESS_CHANGED_STATUS, DDS_LIVELINESS_CHANGED_STATUS);
    ASSERT_EQ(readerListener.liveliness_changed_reader.delegate(), dr.delegate());
    ASSERT_EQ(readerListener.liveliness_changed_status.alive_count(), 0);
    ASSERT_EQ(readerListener.liveliness_changed_status.alive_count_change(), -1);
    ASSERT_EQ(readerListener.liveliness_changed_status.not_alive_count(), 0);
    ASSERT_EQ(readerListener.liveliness_changed_status.not_alive_count_change(), 0);
    ASSERT_EQ(readerListener.liveliness_changed_status.last_publication_handle(), dw_instance_handle);

    /* The listener should have reset the count_change variables. */
    status = dr.liveliness_changed_status();
    ASSERT_EQ(status.alive_count(), 0);
    ASSERT_EQ(status.alive_count_change(), 0);
    ASSERT_EQ(status.not_alive_count(), 0);
    ASSERT_EQ(status.not_alive_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), dw_instance_handle);
}

DDSCXX_TEST_F(ddscxx_listener_trigger, incorrect_listener_mask)
{
    // Create reader with reader listener, but using mask for writer event type
    readerMask << dds::core::status::StatusMask::publication_matched();
    dr = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, readerQos, &readerListener, readerMask);
    ASSERT_NE(dr, dds::core::null);

    // Create writer
    dw = dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic, writerQos);
    ASSERT_NE(dw, dds::core::null);

    // Subscription matched handler should not be triggered
    dds_time_t timeout = 500 * DDS_NSECS_IN_MSEC;
    uint32_t triggered = waitfor_cb(DDS_SUBSCRIPTION_MATCHED_STATUS, timeout);
    ASSERT_NE(triggered & DDS_SUBSCRIPTION_MATCHED_STATUS, DDS_SUBSCRIPTION_MATCHED_STATUS);
}

