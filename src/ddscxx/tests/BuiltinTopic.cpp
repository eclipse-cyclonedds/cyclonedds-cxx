/*
 * Copyright(c) 2006 to 2021 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <map>

#include "dds/dds.hpp"
#include "Space.hpp"

using namespace org::eclipse::cyclonedds;

/**
 * Fixture for the BuiltinTopic tests
 */
class BuiltinTopic : public ::testing::Test
{
public:
    const std::string topic_name;

    static const std::string type_name;

    static const std::string test_prefix;

    static const std::map<std::string, std::string> builtins;

    static const size_t max_samples = 16;

    dds::domain::DomainParticipant participant;
    dds::topic::Topic<Space::Type1> topic;

    dds::pub::Publisher publisher;
    dds::pub::DataWriter<Space::Type1> writer;

    dds::sub::Subscriber subscriber;
    dds::sub::DataReader<Space::Type1> reader;

    const dds::sub::Subscriber builtinsubscriber;

    BuiltinTopic():
        topic_name(test_prefix +
            testing::UnitTest::GetInstance()->current_test_info()->test_suite_name() +
            "_" + testing::UnitTest::GetInstance()->current_test_info()->name()),
        participant(domain::default_id()),
        topic(participant, topic_name),
        publisher(participant),
        writer(publisher,topic),
        subscriber(participant),
        reader(subscriber,topic),
        builtinsubscriber(dds::sub::builtin_subscriber(participant))
    {
    }

    void GetPartReader(dds::sub::DataReader<dds::topic::ParticipantBuiltinTopicData> &participantreader)
    {
        const std::string name = "DCPSParticipant";
        std::vector<dds::sub::DataReader<dds::topic::ParticipantBuiltinTopicData> > dpv;
        size_t cnt = dds::sub::find<dds::sub::DataReader<dds::topic::ParticipantBuiltinTopicData>,
            std::back_insert_iterator<std::vector<dds::sub::DataReader<dds::topic::ParticipantBuiltinTopicData> > > >
                (builtinsubscriber, name,
                 std::back_inserter<std::vector<dds::sub::DataReader<dds::topic::ParticipantBuiltinTopicData> > >(dpv));
        ASSERT_GT(cnt,0);
        participantreader = dpv[0];
        ASSERT_NE(participantreader, dds::core::null);
    }

    void GetPubReader(dds::sub::DataReader<dds::topic::PublicationBuiltinTopicData> &publicationreader)
    {
        const std::string name = "DCPSPublication";
        std::vector<dds::sub::DataReader<dds::topic::PublicationBuiltinTopicData> > dpv;
        size_t cnt = dds::sub::find<dds::sub::DataReader<dds::topic::PublicationBuiltinTopicData>,
            std::back_insert_iterator<std::vector<dds::sub::DataReader<dds::topic::PublicationBuiltinTopicData> > > >
                (builtinsubscriber, name,
                std::back_inserter<std::vector<dds::sub::DataReader<dds::topic::PublicationBuiltinTopicData> > >(dpv));
        ASSERT_GT(cnt,0);
        publicationreader = dpv[0];
        ASSERT_NE(publicationreader, dds::core::null);
    }

    void GetSubReader(dds::sub::DataReader<dds::topic::SubscriptionBuiltinTopicData> &subscriptionreader)
    {
        const std::string name = "DCPSSubscription";
        std::vector<dds::sub::DataReader<dds::topic::SubscriptionBuiltinTopicData> > dpv;
        size_t cnt = dds::sub::find<dds::sub::DataReader<dds::topic::SubscriptionBuiltinTopicData>,
            std::back_insert_iterator<std::vector<dds::sub::DataReader<dds::topic::SubscriptionBuiltinTopicData> > > >
                (builtinsubscriber, name,
                std::back_inserter<std::vector<dds::sub::DataReader<dds::topic::SubscriptionBuiltinTopicData> > >(dpv));
        ASSERT_GT(cnt,0);
        subscriptionreader = dpv[0];
        ASSERT_NE(subscriptionreader, dds::core::null);
    }

    void GetTopReader(dds::sub::DataReader<dds::topic::TopicBuiltinTopicData> &topicreader)
    {
        const std::string name = "DCPSTopic";
        std::vector<dds::sub::DataReader<dds::topic::TopicBuiltinTopicData> > dpv;

        /** when topic discovery is not enabled, creating a reader for the builtin topic topic will fail in cyclonedds */
        try {
            size_t cnt = dds::sub::find<dds::sub::DataReader<dds::topic::TopicBuiltinTopicData>,
                  std::back_insert_iterator<std::vector<dds::sub::DataReader<dds::topic::TopicBuiltinTopicData> > > >
                      (builtinsubscriber, name,
                      std::back_inserter<std::vector<dds::sub::DataReader<dds::topic::TopicBuiltinTopicData> > >(dpv));
            #if defined(DDS_HAS_TOPIC_DISCOVERY)
            ASSERT_GT(cnt,0);
            topicreader = dpv[0];
            ASSERT_NE(topicreader, dds::core::null);
            #else
            ASSERT_EQ(cnt,0);
            topicreader = dds::core::null;
            ASSERT_TRUE(false);
            #endif
        } catch (const dds::core::UnsupportedError& ex) {
            #if defined(DDS_HAS_TOPIC_DISCOVERY)
            ASSERT_FALSE(true);
            #else
            ASSERT_EQ(std::string(ex.what()).find("Error Unsupported - Could not create DataReader."), 0);
            #endif
        }
    }

    void SetUp()
    {
        while (writer.publication_matched_status().current_count() == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
        }

        ASSERT_NE(participant, dds::core::null);
        ASSERT_NE(topic, dds::core::null);

        ASSERT_NE(publisher, dds::core::null);
        ASSERT_NE(writer, dds::core::null);

        ASSERT_NE(subscriber, dds::core::null);
        ASSERT_NE(reader, dds::core::null);

        ASSERT_NE(builtinsubscriber, dds::core::null);
    }

    void TearDown()
    {
        /** setting the participant to null should clean up all its children (topic etc.) */
        participant = dds::core::null;
    }

    template<typename H>
    void validate_participants(const H &samples, size_t n_expect)
    {
        auto length = static_cast<size_t>(std::distance(samples.begin(), samples.end()));
        if (n_expect) {
            ASSERT_GE(length, n_expect);
        } else {
            ASSERT_EQ(length, 0);
        }

        for (const auto & sam:samples) {
            auto info = sam.info();
            ASSERT_TRUE(info.valid());
        }
    }

    template<typename H>
    void validate_topics(const H &samples, size_t n_expect)
    {
        auto length = static_cast<size_t>(std::distance(samples.begin(), samples.end()));
        if (n_expect) {
            ASSERT_GE(length, n_expect);
        } else {
            ASSERT_EQ(length, 0);
        }

        size_t seen = 0;
        for (const auto & sam:samples) {
            auto info = sam.info();
            ASSERT_TRUE(info.valid());
            auto data = sam.data();
            //none of the builting topic should appear here
            ASSERT_EQ(builtins.find(data.name()), builtins.end());

            if (data.name() == topic_name) {
                ASSERT_EQ(data.type_name(), type_name);
                seen++;
            }
        }
        ASSERT_GE(seen, n_expect);
    }

    template<typename H>
    void validate_pubs(const H &samples, size_t n_expect)
    {
        auto length = static_cast<size_t>(std::distance(samples.begin(), samples.end()));
        if (n_expect) {
            ASSERT_GE(length, n_expect);
        } else {
            ASSERT_EQ(length, 0);
        }

        size_t topic_found = 0;
        for (const auto & sam:samples) {
            auto info = sam.info();
            ASSERT_TRUE(info.valid());
            auto data = sam.data();

            if (data.topic_name() == topic_name) {
                ASSERT_EQ(data.type_name(), type_name);
                topic_found++;
            }
        }
        ASSERT_GE(topic_found, n_expect);
    }

    template<typename H>
    void validate_subs(const H &samples, size_t n_expect)
    {
        auto length = static_cast<size_t>(std::distance(samples.begin(), samples.end()));
        if (n_expect) {
            ASSERT_GE(length, n_expect);
        } else {
            ASSERT_EQ(length, 0);
        }

        size_t topic_found = 0, sub_found = 0;
        for (const auto & sam:samples) {
            auto info = sam.info();
            ASSERT_TRUE(info.valid());
            auto data = sam.data();

            if (data.topic_name() == topic_name) {
                ASSERT_EQ(data.type_name(), type_name);
                topic_found++;
            }

            if (data.topic_name() == "DCPSSubscription") {
                ASSERT_EQ(data.type_name(), "org::eclipse::cyclonedds::builtin::DCPSSubscription");
                sub_found++;
            }
        }
        ASSERT_GE(topic_found, n_expect);
        ASSERT_GE(sub_found, n_expect);
    }
};

const std::map<std::string, std::string> BuiltinTopic::builtins =
    { {"DCPSPublication",   "org::eclipse::cyclonedds::builtin::DCPSPublication"},
      {"DCPSSubscription",  "org::eclipse::cyclonedds::builtin::DCPSSubscription"},
      {"DCPSTopic",         "org::eclipse::cyclonedds::builtin::DCPSTopic"},
      {"DCPSParticipant",   "org::eclipse::cyclonedds::builtin::DCPSParticipant"} };

const std::string BuiltinTopic::type_name = org::eclipse::cyclonedds::topic::TopicTraits<Space::Type1>::getTypeName();

const std::string BuiltinTopic::test_prefix = "Builtin_test_";

#define test_loaned_samples(TYPE, GFNCTN, VFNCTN)\
dds::sub::DataReader<TYPE> reader(dds::core::null);\
GFNCTN(reader);\
auto samples = reader.read();\
VFNCTN(samples, 1);\
samples = reader.read();\
VFNCTN(samples, 1);\
samples = reader.take();\
VFNCTN(samples, 1);\
samples = reader.take();\
VFNCTN(samples, 0);

#define test_forward_iterators(TYPE, GFNCTN, VFNCTN)\
dds::sub::DataReader<TYPE> reader(dds::core::null);\
GFNCTN(reader);\
{   std::vector<dds::sub::Sample<TYPE> > samples(max_samples);\
    samples.resize(reader.read(samples.begin(), max_samples));\
    VFNCTN(samples, 1);}\
{   std::vector<dds::sub::Sample<TYPE> > samples(max_samples);\
    samples.resize(reader.read(samples.begin(), max_samples));\
    VFNCTN(samples, 1);}\
{   std::vector<dds::sub::Sample<TYPE> > samples(max_samples);\
    samples.resize(reader.take(samples.begin(), max_samples));\
    VFNCTN(samples, 1);}\
{   std::vector<dds::sub::Sample<TYPE> > samples(max_samples);\
    samples.resize(reader.take(samples.begin(), max_samples));\
    VFNCTN(samples, 0);}

#define test_back_insert_iterators(TYPE, GFNCTN, VFNCTN)\
dds::sub::DataReader<TYPE> reader(dds::core::null);\
GFNCTN(reader);\
{   std::vector<dds::sub::Sample<TYPE> > samples;\
    std::back_insert_iterator<std::vector<dds::sub::Sample<TYPE> > > it(samples);\
    samples.resize(reader.read(it));\
    VFNCTN(samples, 1);}\
{   std::vector<dds::sub::Sample<TYPE> > samples;\
    std::back_insert_iterator<std::vector<dds::sub::Sample<TYPE> > > it(samples);\
    samples.resize(reader.read(it));\
    VFNCTN(samples, 1);}\
{   std::vector<dds::sub::Sample<TYPE> > samples;\
    std::back_insert_iterator<std::vector<dds::sub::Sample<TYPE> > > it(samples);\
    samples.resize(reader.take(it));\
    VFNCTN(samples, 1);}\
{   std::vector<dds::sub::Sample<TYPE> > samples;\
    std::back_insert_iterator<std::vector<dds::sub::Sample<TYPE> > > it(samples);\
    samples.resize(reader.take(it));\
    VFNCTN(samples, 0);}

/**
 * Testing builtin topic for participants
 */
TEST_F(BuiltinTopic, participant)
{
    test_loaned_samples(dds::topic::ParticipantBuiltinTopicData, GetPartReader, validate_participants);
}

TEST_F(BuiltinTopic, participant_forward_iterator)
{
    test_forward_iterators(dds::topic::ParticipantBuiltinTopicData, GetPartReader, validate_participants);
}

TEST_F(BuiltinTopic, participant_back_insert_iterator)
{
    test_back_insert_iterators(dds::topic::ParticipantBuiltinTopicData, GetPartReader, validate_participants);
}

/**
 * Testing builtin topic for topics
 */
TEST_F(BuiltinTopic, topic)
{
    #if defined(DDS_HAS_TOPIC_DISCOVERY)
    test_loaned_samples(dds::topic::TopicBuiltinTopicData, GetTopReader, validate_topics);
    #else
    dds::sub::DataReader<dds::topic::TopicBuiltinTopicData> topicreader(dds::core::null);
    GetTopReader(topicreader);
    #endif
}

TEST_F(BuiltinTopic, topic_forward_iterator)
{
    #if defined(DDS_HAS_TOPIC_DISCOVERY)
    test_forward_iterators(dds::topic::TopicBuiltinTopicData, GetTopReader, validate_topics);
    #else
    dds::sub::DataReader<dds::topic::TopicBuiltinTopicData> topicreader(dds::core::null);
    GetTopReader(topicreader);
    #endif
}

TEST_F(BuiltinTopic, topic_back_insert_iterator)
{
    #if defined(DDS_HAS_TOPIC_DISCOVERY)
    test_back_insert_iterators(dds::topic::TopicBuiltinTopicData, GetTopReader, validate_topics);
    #else
    dds::sub::DataReader<dds::topic::TopicBuiltinTopicData> topicreader(dds::core::null);
    GetTopReader(topicreader);
    #endif
}

/**
 * Testing builtin topic for publication
 */
TEST_F(BuiltinTopic, publication)
{
    test_loaned_samples(dds::topic::PublicationBuiltinTopicData, GetPubReader, validate_pubs);
}

TEST_F(BuiltinTopic, publication_forward_iterator)
{
    test_forward_iterators(dds::topic::PublicationBuiltinTopicData, GetPubReader, validate_pubs);
}

TEST_F(BuiltinTopic, publication_back_insert_iterator)
{
    test_back_insert_iterators(dds::topic::PublicationBuiltinTopicData, GetPubReader, validate_pubs);
}

/**
 * Testing builtin topic for subscription
 */
TEST_F(BuiltinTopic, subscription)
{
    test_loaned_samples(dds::topic::SubscriptionBuiltinTopicData, GetSubReader, validate_subs);
}

TEST_F(BuiltinTopic, subscription_forward_iterator)
{
    test_forward_iterators(dds::topic::SubscriptionBuiltinTopicData, GetSubReader, validate_subs);
}

TEST_F(BuiltinTopic, subscription_back_insert_iterator)
{
    test_back_insert_iterators(dds::topic::SubscriptionBuiltinTopicData, GetSubReader, validate_subs);
}

#undef test_loaned_samples
#undef test_forward_iterators
#undef test_back_insert_iterators
