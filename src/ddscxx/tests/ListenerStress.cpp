// Copyright(c) 2006 to 2021 ZettaScale Technology and others
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

#include "dds/dds.hpp"
#include "dds/ddsrt/sync.h"
#include "dds/ddsrt/threads.h"

#include <gtest/gtest.h>
#include "HelloWorldData.hpp"

#define MAX_WRITERS 20
#define MAX_READERS 50

static ddsrt_mutex_t g_mutex;

class TestDomainParticipantListener : public virtual dds::domain::NoOpDomainParticipantListener
{
public:
    TestDomainParticipantListener() { }

protected:
    virtual void on_data_available(dds::sub::AnyDataReader& ) { }
};

class TestSubscriberListener : public virtual dds::sub::NoOpSubscriberListener
{
public:
    TestSubscriberListener() { }

protected:
    virtual void on_data_available(dds::sub::AnyDataReader& ) { }
};

class TestDataReaderListener : public virtual dds::sub::NoOpDataReaderListener<HelloWorldData::Msg>
{
public:
    TestDataReaderListener() { }

protected:
    virtual void on_data_available(dds::sub::DataReader<HelloWorldData::Msg>& ) { }
};


static bool stop_writer_thread = false;
static std::vector< dds::pub::DataWriter<HelloWorldData::Msg> > writers;


static uint32_t writer_thread(void *arg)
{
    uintptr_t i = reinterpret_cast<uintptr_t>(arg);

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
class listener_stress : public ::testing::Test
{
public:
    dds::domain::DomainParticipant dp;
    dds::topic::Topic<HelloWorldData::Msg> topic;
    dds::pub::Publisher pub;
    dds::sub::Subscriber sub;

    TestDomainParticipantListener participantListener;
    TestSubscriberListener subListener;

    ddsrt_thread_t threadId[MAX_WRITERS];
    ddsrt_threadattr_t threadAttr;
    dds_duration_t delay = 200000000;  // 200ms

    listener_stress() :
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
        for (uintptr_t i = 0; i < MAX_WRITERS; i++) {
            ddsrt_mutex_lock(&g_mutex);
            writers.push_back(dds::pub::DataWriter<HelloWorldData::Msg>(pub, topic));
            (void)ddsrt_thread_create(
                &threadId[i], "writer_thread", &threadAttr, writer_thread, reinterpret_cast<void*>(i));
            ddsrt_mutex_unlock(&g_mutex);
        }

        dds_sleepfor(delay);
    }

    void create_readers(dds::sub::Subscriber& sub_, dds::topic::Topic<HelloWorldData::Msg>& topic_, bool withListener)
    {
        uint32_t i;
        TestDataReaderListener readerListener;
        dds::sub::DataReader<HelloWorldData::Msg> reader = dds::core::null;

        // Create readers
        for (i = 0; i < MAX_READERS; i++) {
            try {
                if (withListener) {
                    reader = dds::sub::DataReader<HelloWorldData::Msg>(sub_, topic_, sub.default_datareader_qos(),
                            &readerListener, dds::core::status::StatusMask::data_available());
                } else {
                    reader = dds::sub::DataReader<HelloWorldData::Msg>(sub_, topic_);
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
TEST_F(listener_stress, DISABLED_data_available_reader)
{
    create_readers(sub, topic, true);
}

TEST_F(listener_stress, DISABLED_data_available_subscriber)
{
    // Add listener on subscriber
    sub.listener(&subListener, dds::core::status::StatusMask::data_available());

    // Create readers
    create_readers(sub, topic, false);
}

TEST_F(listener_stress, DISABLED_data_available_participant)
{
    // Add listener on participant
    dp.listener(&participantListener, dds::core::status::StatusMask::data_available());

    // Create readers
    create_readers(sub, topic, false);
}
