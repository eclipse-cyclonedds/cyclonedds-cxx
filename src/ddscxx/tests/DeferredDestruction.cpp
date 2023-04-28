// Copyright(c) 2023 ZettaScale Technologies
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "HelloWorldData.hpp"

#include <chrono>
#include <dds/dds.hpp>
#include <gtest/gtest.h>
#include <thread>

#include "dds/ddsrt/process.h"
#include "dds/ddsrt/threads.h"

#if DDSCXX_USE_BOOST
#include <boost/optional.hpp>
#define DDSCXX_STD_IMPL boost
#else
#include <optional>
#define DDSCXX_STD_IMPL std
#endif

using HelloWorldData::Msg;
using namespace dds;

std::string generate_unique_topic_name(const std::string &prefix)
{
    static std::atomic<uint64_t> count(0);

    return prefix + std::to_string(count.load()) + "_pid" +  std::to_string(ddsrt_getpid()) + "_tid" +  std::to_string(ddsrt_gettid());
}

std::string topicName_ = generate_unique_topic_name("DeferredDestruction");

template<typename T>
class FakeDataReaderListener : public sub::NoOpDataReaderListener<T>
{
public:
    FakeDataReaderListener()
    {
        std::cout << "Creating data reader listener for topic: " << topicName_ << std::endl;
    }

    void on_subscription_matched(sub::DataReader<T>&,
                                 const core::status::SubscriptionMatchedStatus&)
    {
        std::cout << "topic: " << topicName_ << " data reader listener: on_subscription_matched called" << std::endl;
    }

    void on_liveliness_changed(sub::DataReader<T>&,
                               const core::status::LivelinessChangedStatus& status)
    {
        std::cout << "topic: " << topicName_ << " data reader listener: on_liveliness_changed called "
                  << " alive count: " << status.alive_count() << " not alive count: " << status.not_alive_count()
                  << "alive count change: " << status.alive_count_change()
                  << " not alive count change: " << status.not_alive_count_change() << std::endl;
    }

    void on_data_available(sub::DataReader<T>&)
    {
        std::cout << "topic: " << topicName_ << " data reader listener: on_data_available called" << std::endl;
    }
};

template<typename T>
class FakeDataWriterListener : public pub::NoOpDataWriterListener<T>
{
public:
    FakeDataWriterListener()
    {
        std::cout << "Creating data writer listener for topic: " << topicName_ << std::endl;
    }

    void on_publication_matched(pub::DataWriter<T>&,
                                const core::status::PublicationMatchedStatus&)
    {
        std::cout << "topic: " << topicName_ << " data writer listener: on_publication_matched called" << std::endl;
    }
    void on_liveliness_lost(pub::DataWriter<T>&,
                            const core::status::LivelinessLostStatus&)
    {
        std::cout << "topic: " << topicName_ << " data writer listener: on_liveliness_lost called" << std::endl;
    }
};

// This function could run into deadlock
void multiReaderSingleWriter(uint32_t readerCount)
{

    // declare writer
    domain::DomainParticipant participant(org::eclipse::cyclonedds::domain::default_id());
    topic::Topic<Msg> topic(participant, topicName_);
    pub::Publisher pub(participant);
    FakeDataWriterListener<Msg> writeListener;

    DDSCXX_STD_IMPL::optional<pub::DataWriter<Msg>> writer;
    EXPECT_NO_THROW(writer.emplace(pub, topic, pub::qos::DataWriterQos(), &writeListener, core::status::StatusMask::all()));

    // declare readers in multithreads
    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < readerCount; i++) {
        threads.emplace_back(std::thread([&participant, &topic] {
            using namespace std::literals;

            sub::Subscriber sub(participant);
            auto reader = sub::DataReader<Msg>(sub, topic);
        }));
    }

    //killing the writer on which callbacks are made
    writer->listener(nullptr, dds::core::status::StatusMask::none());
    writer.reset();

    //waiting for the reader threads to complete
    while (threads.size()) {
        for (auto it = threads.begin(); it != threads.end();) {
            if (it->joinable()) {
                it->join();
                it = threads.erase(it);
            } else {
                it++;
            }
        }
    }
}

// This function could cause assertation to false on exit
void multiWriterSingleReader(uint32_t writerCount)
{
    // declare reader
    domain::DomainParticipant participant(org::eclipse::cyclonedds::domain::default_id());
    topic::Topic<Msg> topic(participant, topicName_);

    sub::Subscriber sub(participant);
    FakeDataReaderListener<Msg> readListener;
    DDSCXX_STD_IMPL::optional<sub::DataReader<Msg>> reader;
    EXPECT_NO_THROW(reader.emplace(sub, topic, sub::qos::DataReaderQos(), &readListener, core::status::StatusMask::all()));

    // declare writers in multithreads
    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < writerCount; i++) {
        threads.emplace_back(std::thread([&participant, &topic] {
            using namespace std::literals;

            pub::Publisher pub(participant);
            pub::DataWriter<Msg> writer(pub, topic);
        }));
    }

    //killing the reader on which callbacks are made
    reader->listener(nullptr, dds::core::status::StatusMask::none());
    reader.reset();

    //waiting for the writer threads to complete
    while (threads.size()) {
        for (auto it = threads.begin(); it != threads.end();) {
            if (it->joinable()) {
                it->join();
                it = threads.erase(it);
            } else {
                it++;
            }
        }
    }
}

// could lead to assert(false)
TEST(DeferredDestruction, MultiWriterSingleReader)
{
    multiWriterSingleReader(50);
}

// could lead to a deadlock
TEST(DeferredDestruction, MultiReaderSingleWriter)
{
    multiReaderSingleWriter(50);
}