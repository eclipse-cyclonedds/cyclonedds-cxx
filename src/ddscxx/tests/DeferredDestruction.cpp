/*
 * Copyright(c) 2023 ZettaScale Technologies
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
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

std::string generate_unique_topic_name(const std::string &prefix)
{
    static std::atomic<uint64_t> count(0);

    return prefix + std::to_string(count.load()) + "_pid" +  std::to_string(ddsrt_getpid()) + "_tid" +  std::to_string(ddsrt_gettid());
}

std::string topicName_ = generate_unique_topic_name("DeferredDestruction");

template<typename T>
class FakeDataReaderListener : public dds::sub::NoOpDataReaderListener<T>
{
public:
    FakeDataReaderListener()
    {
        std::cout << "Creating data reader listener for topic: " << topicName_ << std::endl;
    }

    void on_subscription_matched([[maybe_unused]] dds::sub::DataReader<T>& reader,
                                 [[maybe_unused]] const dds::core::status::SubscriptionMatchedStatus& status) override
    {
        std::cout << "topic: " << topicName_ << " data reader listener: on_subscription_matched called" << std::endl;
    }

    void on_liveliness_changed([[maybe_unused]] dds::sub::DataReader<T>& reader,
                               [[maybe_unused]] const dds::core::status::LivelinessChangedStatus& status) override
    {
        std::cout << "topic: " << topicName_ << " data reader listener: on_liveliness_changed called "
                  << " alive count: " << status.alive_count() << " not alive count: " << status.not_alive_count()
                  << "alive count change: " << status.alive_count_change()
                  << " not alive count change: " << status.not_alive_count_change() << std::endl;
    }

    void on_data_available([[maybe_unused]] dds::sub::DataReader<T>& reader) override
    {
        std::cout << "topic: " << topicName_ << " data reader listener: on_data_available called" << std::endl;
    }
};

template<typename T>
class FakeDataWriterListener : public dds::pub::NoOpDataWriterListener<T>
{
public:
    FakeDataWriterListener()
    {
        std::cout << "Creating data writer listener for topic: " << topicName_ << std::endl;
    }

    void on_publication_matched([[maybe_unused]] dds::pub::DataWriter<T>& writer,
                                [[maybe_unused]] const dds::core::status::PublicationMatchedStatus& status) override
    {
        std::cout << "topic: " << topicName_ << " data writer listener: on_publication_matched called" << std::endl;
    }
    void on_liveliness_lost([[maybe_unused]] dds::pub::DataWriter<T>& writer,
                            [[maybe_unused]] const dds::core::status::LivelinessLostStatus& status) override
    {
        std::cout << "topic: " << topicName_ << " data writer listener: on_liveliness_lost called" << std::endl;
    }
};

// This function could run into deadlock
void multiReaderSingleWriter([[maybe_unused]] uint32_t readerCnt)
{

    // declare writer
    dds::domain::DomainParticipant participant{0};
    dds::topic::Topic<Msg> topic(participant, topicName_);
    dds::pub::Publisher pub{participant};
    dds::pub::qos::DataWriterQos writeQos{};
    FakeDataWriterListener<Msg> writeListener;
    dds::core::status::StatusMask mask{dds::core::status::StatusMask::all()};
    DDSCXX_STD_IMPL::optional<dds::pub::DataWriter<Msg>> writer;
    EXPECT_NO_THROW(writer.emplace(pub, topic, writeQos, &writeListener, mask));

    // declare readers in multithreads
    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < readerCnt; i++) {
        threads.emplace_back(std::thread([&participant, &topic] {
            using namespace std::literals;
            dds::sub::Subscriber sub{participant};
            dds::sub::qos::DataReaderQos readQos{};
            FakeDataReaderListener<Msg> readListener;
            dds::core::status::StatusMask readMask{dds::core::status::StatusMask::none()};
            auto reader = dds::sub::DataReader<Msg>{sub, topic, readQos, &readListener, readMask};
        }));
    }

    writer.reset();

    for (auto&& t : threads) {
        if (t.joinable()) {
            t.join();
        }
    }
}

// This function could cause assertation to false on exit
void multiWriterSingleReader([[maybe_unused]] uint32_t writerCount)
{
    // declare reader
    dds::domain::DomainParticipant participant{0};
    dds::topic::Topic<Msg> topic(participant, topicName_);

    dds::sub::Subscriber sub{participant};
    dds::sub::qos::DataReaderQos readQos{};
    FakeDataReaderListener<Msg> readListener;
    dds::core::status::StatusMask readMask{dds::core::status::StatusMask::all()};
    DDSCXX_STD_IMPL::optional<dds::sub::DataReader<Msg>> reader;
    EXPECT_NO_THROW(reader.emplace(sub, topic, readQos, &readListener, readMask));

    // declare writers in multithreads
    std::vector<std::thread> threads;
    for (uint32_t i = 0; i < writerCount; i++) {
        threads.emplace_back(std::thread([&participant, &topic] {
            using namespace std::literals;

            dds::pub::Publisher pub{participant};
            dds::pub::qos::DataWriterQos writeQos{};
            FakeDataWriterListener<Msg> writeListener;
            dds::core::status::StatusMask writeMask{dds::core::status::StatusMask::none()};
            dds::pub::DataWriter<Msg> writer(pub, topic, writeQos, &writeListener, writeMask);
        }));
    }

    reader.reset();

    for (auto&& t : threads) {
        if (t.joinable()) {
            t.join();
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