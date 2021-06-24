#include <chrono>
#include <thread>

#include "dds/dds.hpp"

#include <gtest/gtest.h>

#include "Util.hpp"
#include "HelloWorldData.hpp"

/**
 * Fixture for entity status tests
 */
class EntityStatus : public ::testing::Test
{
public:
    dds::domain::DomainParticipant par;
    dds::pub::Publisher pub;
    dds::sub::Subscriber sub;
    dds::topic::Topic<HelloWorldData::Msg> topic;

    EntityStatus() :
        par(dds::core::null),
        pub(dds::core::null),
        sub(dds::core::null),
        topic(dds::core::null)
    {
    }

    void SetUp() {
        char name[32];

        this->par = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(this->par, dds::core::null);

        this->pub = dds::pub::Publisher(this->par);
        ASSERT_NE(this->pub, dds::core::null);

        this->sub = dds::sub::Subscriber(this->par);
        ASSERT_NE(this->sub, dds::core::null);

        create_unique_topic_name("EntityStatus", name, sizeof(name));
        this->topic = dds::topic::Topic<HelloWorldData::Msg>(this->par, name);
        ASSERT_NE(this->topic, dds::core::null);
    }

    void TearDown() {
        this->topic = dds::core::null;
        this->sub = dds::core::null;
        this->pub = dds::core::null;
        this->par = dds::core::null;
    }
};

TEST_F(EntityStatus, publication_matched)
{
    dds::core::status::PublicationMatchedStatus status;
    const dds_duration_t interval = DDS_MSECS(5);
    dds_duration_t timeout = DDS_SECS(2);
    dds::core::InstanceHandle reader_hdl;

    //
    // Test setup
    //
    dds::pub::DataWriter<HelloWorldData::Msg> writer(this->pub, this->topic);
    ASSERT_NE(writer, dds::core::null);

    dds::sub::DataReader<HelloWorldData::Msg> reader(this->sub, this->topic);
    ASSERT_NE(reader, dds::core::null);
    reader_hdl = reader.instance_handle();

    //
    // Get status change
    //
    while (timeout > 0) {
        status = writer.publication_matched_status();
        if (status.current_count() > 0) {
            ASSERT_EQ(status.current_count(), 1);
            ASSERT_EQ(status.current_count_change(), 1);
            ASSERT_EQ(status.total_count(), 1);
            ASSERT_EQ(status.total_count_change(), 1);
            ASSERT_EQ(status.last_subscription_handle(), reader_hdl);
            break;
        } else {
            dds_sleepfor(interval);
            timeout -= interval;
        }
    }
    ASSERT_GT(timeout, 0);

    // Get status a second time (change count(s) should have reset).
    status = writer.publication_matched_status();
    ASSERT_EQ(status.current_count(), 1);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_subscription_handle(), reader_hdl);

    //
    // Un-match the publication by deleting the reader
    //
    reader = dds::core::null;

    //
    // Get status change
    //
    while (timeout > 0) {
        status = writer.publication_matched_status();
        if (status.current_count() == 0) {
            ASSERT_EQ(status.current_count(), 0);
            ASSERT_EQ(status.current_count_change(), -1);
            ASSERT_EQ(status.total_count(), 1);
            ASSERT_EQ(status.total_count_change(), 0);
            ASSERT_EQ(status.last_subscription_handle(), reader_hdl);
            break;
        } else {
            dds_sleepfor(interval);
            timeout -= interval;
        }
    }
    ASSERT_GT(timeout, 0);

    // Get status a second time (change count(s) should have reset).
    status = writer.publication_matched_status();
    ASSERT_EQ(status.current_count(), 0);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_subscription_handle(), reader_hdl);

    // Cleanup
    writer = dds::core::null;
}

TEST_F(EntityStatus, subscription_matched)
{
    dds::core::status::SubscriptionMatchedStatus status;
    const dds_duration_t interval = DDS_MSECS(5);
    dds_duration_t timeout = DDS_SECS(2);
    dds::core::InstanceHandle writer_hdl;

    //
    // Test setup
    //
    dds::sub::DataReader<HelloWorldData::Msg> reader(this->sub, this->topic);
    ASSERT_NE(reader, dds::core::null);

    dds::pub::DataWriter<HelloWorldData::Msg> writer(this->pub, this->topic);
    ASSERT_NE(writer, dds::core::null);
    writer_hdl = writer.instance_handle();

    //
    // Get status change
    //
    while (timeout > 0) {
        status = reader.subscription_matched_status();
        if (status.current_count() > 0) {
            ASSERT_EQ(status.current_count(), 1);
            ASSERT_EQ(status.current_count_change(), 1);
            ASSERT_EQ(status.total_count(), 1);
            ASSERT_EQ(status.total_count_change(), 1);
            ASSERT_EQ(status.last_publication_handle(), writer_hdl);
            break;
        } else {
            dds_sleepfor(interval);
            timeout -= interval;
        }
    }
    ASSERT_GT(timeout, 0);

    // Get status a second time (change count(s) should have reset).
    status = reader.subscription_matched_status();
    ASSERT_EQ(status.current_count(), 1);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), writer_hdl);

    //
    // Un-match the publication by deleting the writer
    //
    writer = dds::core::null;

    //
    // Get status change
    //
    while (timeout > 0) {
        status = reader.subscription_matched_status();
        if (status.current_count() == 0) {
            ASSERT_EQ(status.current_count(), 0);
            ASSERT_EQ(status.current_count_change(), -1);
            ASSERT_EQ(status.total_count(), 1);
            ASSERT_EQ(status.total_count_change(), 0);
            ASSERT_EQ(status.last_publication_handle(), writer_hdl);
            break;
        } else {
            dds_sleepfor(interval);
            timeout -= interval;
        }
    }
    ASSERT_GT(timeout, 0);

    // Get status a second time (change count(s) should have reset).
    status = reader.subscription_matched_status();
    ASSERT_EQ(status.current_count(), 0);
    ASSERT_EQ(status.current_count_change(), 0);
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), writer_hdl);

    // Cleanup
    reader = dds::core::null;
}

TEST_F(EntityStatus, incompatible_qos)
{
    dds::core::status::RequestedIncompatibleQosStatus r_status;
    dds::core::status::OfferedIncompatibleQosStatus w_status;
    const dds_duration_t interval = DDS_MSECS(5);
    dds_duration_t timeout = DDS_SECS(2);
    dds::sub::qos::DataReaderQos rqos;

    //
    // Test setup
    //
    dds::pub::DataWriter<HelloWorldData::Msg> writer(this->pub, this->topic);
    ASSERT_NE(writer, dds::core::null);

    rqos << dds::core::policy::Durability::Persistent();
    dds::sub::DataReader<HelloWorldData::Msg> reader(this->sub, this->topic, rqos);
    ASSERT_NE(reader, dds::core::null);

    //
    // Get status change
    //
    while (timeout > 0) {
        w_status = writer.offered_incompatible_qos_status();
        if (w_status.total_count() > 0) {
            ASSERT_EQ(w_status.total_count(), 1);
            ASSERT_EQ(w_status.total_count_change(), 1);
            ASSERT_EQ(w_status.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);
            break;
        } else {
            dds_sleepfor(interval);
            timeout -= interval;
        }
    }
    while (timeout > 0) {
        r_status = reader.requested_incompatible_qos_status();
        if (r_status.total_count() > 0) {
            ASSERT_EQ(r_status.total_count(), 1);
            ASSERT_EQ(r_status.total_count_change(), 1);
            ASSERT_EQ(r_status.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);
            break;
        } else {
            dds_sleepfor(interval);
            timeout -= interval;
        }
    }
    ASSERT_GT(timeout, 0);

    // Get status a second time (change count(s) should have reset).
    w_status = writer.offered_incompatible_qos_status();
    ASSERT_EQ(w_status.total_count(), 1);
    ASSERT_EQ(w_status.total_count_change(), 0);
    ASSERT_EQ(w_status.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);

    r_status = reader.requested_incompatible_qos_status();
    ASSERT_EQ(r_status.total_count(), 1);
    ASSERT_EQ(r_status.total_count_change(), 0);
    ASSERT_EQ(r_status.last_policy_id(), DDS_DURABILITY_QOS_POLICY_ID);

    // Cleanup
    reader = dds::core::null;
    writer = dds::core::null;
}

TEST_F(EntityStatus, sample_lost)
{
    dds::core::status::SampleLostStatus status;
    const dds_duration_t interval = DDS_MSECS(5);
    dds_duration_t timeout = DDS_SECS(2);
    dds::sub::qos::DataReaderQos rqos;
    dds::pub::qos::DataWriterQos wqos;

    //
    // Test setup
    //
    wqos << dds::core::policy::Reliability::Reliable(dds::core::Duration(1, 0));
    wqos << dds::core::policy::DestinationOrder::SourceTimestamp();
    wqos << dds::core::policy::History::KeepAll();
    dds::pub::DataWriter<HelloWorldData::Msg> writer(this->pub, this->topic, wqos);
    ASSERT_NE(writer, dds::core::null);

    rqos << dds::core::policy::Reliability::Reliable(dds::core::Duration(1, 0));
    rqos << dds::core::policy::DestinationOrder::SourceTimestamp();
    rqos << dds::core::policy::History::KeepAll();
    dds::sub::DataReader<HelloWorldData::Msg> reader(this->sub, this->topic, rqos);
    ASSERT_NE(reader, dds::core::null);

    HelloWorldData::Msg msg(1, "test sample_lost");
    dds::core::Time current_time = this->par.current_time();
    dds::core::Time the_past = current_time - dds::core::Duration::from_secs(30);
    writer.write(msg, current_time);
    writer.write(msg, the_past);

    //
    // Get status change
    //
    while (timeout > 0) {
        status = reader.sample_lost_status();
        if (status.total_count() > 0) {
            ASSERT_EQ(status.total_count(), 1);
            ASSERT_EQ(status.total_count_change(), 1);
            break;
        } else {
            dds_sleepfor(interval);
            timeout -= interval;
        }
    }
    ASSERT_GT(timeout, 0);

    // Get status a second time (change count(s) should have reset).
    status = reader.sample_lost_status();
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);

    // Cleanup
    reader = dds::core::null;
    writer = dds::core::null;
}

TEST_F(EntityStatus, sample_rejected)
{
    dds::core::status::SampleRejectedStatus status;
    const dds_duration_t interval = DDS_MSECS(5);
    dds_duration_t timeout = DDS_SECS(2);
    dds::sub::qos::DataReaderQos rqos;
    dds::pub::qos::DataWriterQos wqos;

    dds::core::status::SampleRejectedState reason =
    dds::core::status::SampleRejectedState::rejected_by_samples_limit();

    //
    // Test setup
    //
    wqos << dds::core::policy::Reliability::BestEffort(dds::core::Duration(0, DDS_MSECS(100)));
    wqos << dds::core::policy::History::KeepAll();
    dds::pub::DataWriter<HelloWorldData::Msg> writer(this->pub, this->topic, wqos);
    ASSERT_NE(writer, dds::core::null);

    rqos << dds::core::policy::Reliability::BestEffort(dds::core::Duration(0, DDS_MSECS(100)));
    rqos << dds::core::policy::ResourceLimits(1, 1, 1);
    rqos << dds::core::policy::History::KeepAll();
    dds::sub::DataReader<HelloWorldData::Msg> reader(this->sub, this->topic, rqos);
    ASSERT_NE(reader, dds::core::null);

    HelloWorldData::Msg msg(1, "test sample_rejected");
    writer.write(msg);
    writer.write(msg);

    //
    // Get status change
    //
    while (timeout > 0) {
        status = reader.sample_rejected_status();
        if (status.total_count() > 0) {
            ASSERT_EQ(status.total_count(), 1);
            ASSERT_EQ(status.total_count_change(), 1);
            ASSERT_EQ(status.last_reason(), reason);
            ASSERT_EQ(status.last_instance_handle(), reader.lookup_instance(msg));
            break;
        } else {
            dds_sleepfor(interval);
            timeout -= interval;
        }
    }
    ASSERT_GT(timeout, 0);

    // Get status a second time (change count(s) should have reset).
    status = reader.sample_rejected_status();
    ASSERT_EQ(status.total_count(), 1);
    ASSERT_EQ(status.total_count_change(), 0);
    ASSERT_EQ(status.last_reason(), reason);
    ASSERT_EQ(status.last_instance_handle(), reader.lookup_instance(msg));

    // Cleanup
    reader = dds::core::null;
    writer = dds::core::null;
}

TEST_F(EntityStatus, liveliness_changed)
{
    dds::core::status::LivelinessChangedStatus status;
    const dds_duration_t interval = DDS_MSECS(5);
    dds_duration_t timeout = DDS_SECS(2);
    dds::core::InstanceHandle writer_hdl;

    //
    // Test setup
    //
    dds::sub::DataReader<HelloWorldData::Msg> reader(this->sub, this->topic);
    ASSERT_NE(reader, dds::core::null);

    dds::pub::DataWriter<HelloWorldData::Msg> writer(this->pub, this->topic);
    ASSERT_NE(writer, dds::core::null);
    writer_hdl = writer.instance_handle();

    //
    // Get status change
    //
    while (timeout > 0) {
        status = reader.liveliness_changed_status();
        if (status.alive_count() > 0) {
            ASSERT_EQ(status.alive_count(), 1);
            ASSERT_EQ(status.alive_count_change(), 1);
            ASSERT_EQ(status.not_alive_count(), 0);
            ASSERT_EQ(status.not_alive_count_change(), 0);
            ASSERT_EQ(status.last_publication_handle(), writer_hdl);
            break;
        } else {
            dds_sleepfor(interval);
            timeout -= interval;
        }
    }
    ASSERT_GT(timeout, 0);

    // Get status a second time (change count(s) should have reset).
    status = reader.liveliness_changed_status();
    ASSERT_EQ(status.alive_count(), 1);
    ASSERT_EQ(status.alive_count_change(), 0);
    ASSERT_EQ(status.not_alive_count(), 0);
    ASSERT_EQ(status.not_alive_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), writer_hdl);

    //
    // Change liveliness by deleting the writer
    //
    writer = dds::core::null;

    //
    // Get status change
    //
    while (timeout > 0) {
        status = reader.liveliness_changed_status();
        if (status.alive_count() == 0) {
            ASSERT_EQ(status.alive_count(), 0);
            ASSERT_EQ(status.alive_count_change(), -1);
            ASSERT_EQ(status.not_alive_count(), 0);
            ASSERT_EQ(status.not_alive_count_change(), 0);
            ASSERT_EQ(status.last_publication_handle(), writer_hdl);
            break;
        } else {
            dds_sleepfor(interval);
            timeout -= interval;
        }
    }
    ASSERT_GT(timeout, 0);

    // Get status a second time (change count(s) should have reset).
    status = reader.liveliness_changed_status();
    ASSERT_EQ(status.alive_count(), 0);
    ASSERT_EQ(status.alive_count_change(), 0);
    ASSERT_EQ(status.not_alive_count(), 0);
    ASSERT_EQ(status.not_alive_count_change(), 0);
    ASSERT_EQ(status.last_publication_handle(), writer_hdl);

    // Cleanup
    reader = dds::core::null;
}
