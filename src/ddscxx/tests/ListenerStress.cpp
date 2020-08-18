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

#define MAX_WRITERS 20
#define MAX_READERS 50

static ddsrt_mutex_t g_mutex;

namespace lite { namespace ddscxx { namespace tests { namespace listener_stress {

class TestDomainParticipantListener : public virtual dds::domain::NoOpDomainParticipantListener
{
public:
    TestDomainParticipantListener() { }

protected:
    virtual void on_data_available(dds::sub::AnyDataReader& reader)
    {
        (void)reader;
    }
};

class TestSubscriberListener : public virtual dds::sub::NoOpSubscriberListener
{
public:
    TestSubscriberListener() { }

protected:
    virtual void on_data_available(dds::sub::AnyDataReader& reader)
    {
        (void)reader;
    }
};

class TestDataReaderListener : public virtual dds::sub::NoOpDataReaderListener<HelloWorldData::Msg>
{
public:
    TestDataReaderListener() { }

protected:
    virtual void on_data_available(dds::sub::DataReader<HelloWorldData::Msg>& reader)
    {
        (void)reader;
    }
};

} } } }


static bool stop_writer_thread = false;
static std::vector< dds::pub::DataWriter<HelloWorldData::Msg> > writers;


static uint32_t writer_thread(void *arg)
{
    uint32_t i = (uint32_t)(uintptr_t)arg;

    ddsrt_mutex_lock(&g_mutex);
    dds::pub::DataWriter<HelloWorldData::Msg> writer = writers[i];
    ddsrt_mutex_unlock(&g_mutex);

    HelloWorldData::Msg testData(1, "test");

    while (!stop_writer_thread) {
        try {
            writer << testData;
        } catch (const dds::core::Exception& e) {
            std::cout << "Writer [" << i << "] write fails: " << e.what() << std::endl;
            stop_writer_thread = true;
        } catch (...) {
            std::cout << "Writer [" << i << "] write fails" << std::endl;
            stop_writer_thread = true;
        }
    }

    return 0;
}


/**
 * Fixture for listener stress tests
 */
class ddscxx_listener_stress : public ::testing::Test
{
public:
    dds::domain::DomainParticipant dp;
    dds::topic::Topic<HelloWorldData::Msg> topic;
    dds::pub::Publisher pub;
    dds::sub::Subscriber sub;

    lite::ddscxx::tests::listener_stress::TestDomainParticipantListener participantListener;
    lite::ddscxx::tests::listener_stress::TestSubscriberListener subListener;

    ddsrt_thread_t threadId[MAX_WRITERS];
    ddsrt_threadattr_t threadAttr;
    dds_duration_t delay = 200000000;  // 200ms

    ddscxx_listener_stress() :
        dp(dds::core::null),
        topic(dds::core::null),
        pub(dds::core::null),
        sub(dds::core::null)
    {
        ddsrt_threadattr_init(&threadAttr);
        memset(threadId, 0, sizeof(threadId));
    }

    void SetUp() {
        ddsrt_mutex_init(&g_mutex);

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

        // Create writer threads
        uint32_t i;
        for (i = 0; i < MAX_WRITERS; i++) {
            ddsrt_mutex_lock(&g_mutex);
            writers.push_back(dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic));
            (void)ddsrt_thread_create(&threadId[i], "writer_thread",
                            &threadAttr, writer_thread, (void*)(uintptr_t)i);
            ddsrt_mutex_unlock(&g_mutex);
        }

        dds_sleepfor(delay);
    }

    void create_readers(dds::sub::Subscriber& sub, dds::topic::Topic<HelloWorldData::Msg>& topic, bool withListener)
    {
        uint32_t i;
        dds_duration_t delay = 2000000;
        lite::ddscxx::tests::listener_stress::TestDataReaderListener readerListener;
        dds::sub::DataReader<HelloWorldData::Msg> reader = dds::core::null;

        // Create readers
        for (i = 0; i < MAX_READERS; i++) {
            try {
                if (withListener) {
                    reader = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic, sub.default_datareader_qos(),
                            &readerListener, dds::core::status::StatusMask::data_available());
                } else {
                    reader = dds::sub::DataReader<HelloWorldData::Msg>(sub, topic);
                }
                if (reader != dds::core::null) {
                    dds_sleepfor(delay);
                } else {
                    FAIL() << "reader = dds::core::null";
                }
            } catch (const dds::core::Exception& e) {
                FAIL() << "Exception: " << e.what();
            } catch (...) {
                FAIL() << "Unknown exception";
            }
        }
    }

    void TearDown() {
        // Clean up writers
        uint32_t i;
        stop_writer_thread = true;
        for (i = 0; i < MAX_WRITERS; i++) {
            (void)ddsrt_thread_join(threadId[i], NULL);
            ddsrt_mutex_lock(&g_mutex);
            writers[i] = dds::core::null;
            ddsrt_mutex_unlock(&g_mutex);
        }

        pub.close();
        sub.close();
        sub = dds::core::null;
        pub = dds::core::null;

        dp.close();
        dp = dds::core::null;

        ddsrt_mutex_destroy(&g_mutex);
    }
};

// TODO: disabled because test fails for current API implementation, as there is no
// ref to the entity on which the callback is called during callback execution, and
// therefore the check in EntityDelegate destructor fails.
DDSCXX_TEST_F(ddscxx_listener_stress, DISABLED_data_available_reader)
{
    create_readers(sub, topic, true);
}

DDSCXX_TEST_F(ddscxx_listener_stress, DISABLED_data_available_subscriber)
{
    // Add listener on subscriber
    sub.listener(&subListener, dds::core::status::StatusMask::data_available());

    // Create readers
    create_readers(sub, topic, false);
}

DDSCXX_TEST_F(ddscxx_listener_stress, DISABLED_data_available_participant)
{
    // Add listener on participant
    dp.listener(&participantListener, dds::core::status::StatusMask::data_available());

    // Create readers
    create_readers(sub, topic, false);
}
