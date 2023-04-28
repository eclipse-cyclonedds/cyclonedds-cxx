// Copyright(c) 2006 to 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "dds/dds.hpp"
#include <gtest/gtest.h>
#include "Space.hpp"

#include <iostream>


#define TOPIC1_NAME_1    "topic_Type1_1"
#define TOPIC1_NAME_2    "topic_Type1_2"
#define TOPIC1_NAME_3    "topic_Type1_3"

#define TOPIC__NAME      "topic_Type2"

#define CF_TOPIC1_NAME_1 "cf_topic_Type1_1"
#define CF_TOPIC1_NAME_2 "cf_topic_Type1_2"
#define CF_TOPIC1_NAME_3 "cf_topic_Type1_3"


/**
 * Fixture for the Topic finding and discovering tests
 */
class FindTopic : public ::testing::Test
{
public:
    dds::domain::DomainParticipant dp = dds::core::null;
    dds::domain::DomainParticipant dp2 = dds::core::null;

    dds::topic::Topic<Space::Type1> topic1 = dds::core::null;
    dds::topic::Topic<Space::Type1> topic2 = dds::core::null;
    dds::topic::Topic<Space::Type1> topic3 = dds::core::null;
    dds::topic::Topic<Space::Type2> topic_ = dds::core::null;
    dds::topic::ContentFilteredTopic<Space::Type1> cfTopic1 = dds::core::null;
    dds::topic::ContentFilteredTopic<Space::Type1> cfTopic2 = dds::core::null;
    dds::topic::ContentFilteredTopic<Space::Type1> cfTopic3 = dds::core::null;

    std::map<std::string, dds::topic::Topic<Space::Type1> >topic1Map;
    std::map<std::string, dds::topic::Topic<Space::Type2> >topic2Map;
    std::vector<dds::topic::Topic<Space::Type1> > topicList;

    void SetUp() {
        dp  = dds::domain::DomainParticipant(0);
        ASSERT_NE(dp, dds::core::null);
    }

    void CreateTopics() {
        topic1 = dds::topic::Topic<Space::Type1>(dp, TOPIC1_NAME_1);
        topic2 = dds::topic::Topic<Space::Type1>(dp, TOPIC1_NAME_2);
        topic3 = dds::topic::Topic<Space::Type1>(dp, TOPIC1_NAME_3);
        ASSERT_NE(topic1, dds::core::null);
        ASSERT_NE(topic2, dds::core::null);
        ASSERT_NE(topic3, dds::core::null);

        /*
         * Expect this one to appear when discovering list of any or
         * topics or topic descriptions. But not when specifically
         * using Type1 for the discovery of the list.
         */
        topic_ = dds::topic::Topic<Space::Type2>(dp, TOPIC__NAME);
        ASSERT_NE(topic_, dds::core::null);

        /*
         * This can be used to check if the discovered topics are
         * actually the expected ones.
         */
        topic1Map.insert(std::pair<std::string, dds::topic::Topic<Space::Type1> >(TOPIC1_NAME_1, topic1));
        topic1Map.insert(std::pair<std::string, dds::topic::Topic<Space::Type1> >(TOPIC1_NAME_2, topic2));
        topic1Map.insert(std::pair<std::string, dds::topic::Topic<Space::Type1> >(TOPIC1_NAME_3, topic3));
        topic2Map.insert(std::pair<std::string, dds::topic::Topic<Space::Type2> >(TOPIC__NAME  , topic_));

        /*
         * This can be used when discovering lists of topics.
         */
        for (uint32_t i = 0; i < (topic1Map.size() + topic2Map.size() + 10); i++) {
            topicList.push_back(dds::core::null);
        }
    }

#if 0 /* TODO: Add ContentFilteredTopic tests when implemented. */
    void CreateFilteredTopics() {
        std::vector<std::string> params;
        params.push_back("1");
        dds::topic::Filter filter("long_1=%0", params);

        this->CreateTopics();

        cfTopic1 = dds::topic::ContentFilteredTopic<Space::Type1>(topic1, CF_TOPIC1_NAME_1, filter);
        cfTopic2 = dds::topic::ContentFilteredTopic<Space::Type1>(topic2, CF_TOPIC1_NAME_2, filter);
        cfTopic3 = dds::topic::ContentFilteredTopic<Space::Type1>(topic3, CF_TOPIC1_NAME_3, filter);
    }
#endif

    void Create2ndParticipant() {
        dp2 = dds::domain::DomainParticipant(0);
        ASSERT_NE(dp2, dds::core::null);
    }
};



/**
 * Tests
 */

TEST_F(FindTopic, find_with_null)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    ASSERT_THROW({
        found = dds::topic::find<dds::topic::Topic<Space::Type1> >(dds::core::null, std::string(TOPIC1_NAME_1));
    }, dds::core::NullReferenceError);
}

TEST_F(FindTopic, find_with_empty)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    found = dds::topic::find<dds::topic::Topic<Space::Type1> >(this->dp, std::string(TOPIC1_NAME_1));
    ASSERT_EQ(found, dds::core::null);
}

TEST_F(FindTopic, find_with_other_type)
{
    dds::topic::Topic<Space::Type2> found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::find<dds::topic::Topic<Space::Type2> >(this->dp, TOPIC1_NAME_2);
    ASSERT_TRUE(found.is_nil());
}

TEST_F(FindTopic, find_nonexisting)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::find<dds::topic::Topic<Space::Type1> >(this->dp, std::string("non-existing"));
    ASSERT_EQ(found, dds::core::null);
}

TEST_F(FindTopic, find_first)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::find<dds::topic::Topic<Space::Type1> >(this->dp, TOPIC1_NAME_1);
    ASSERT_FALSE(found.is_nil());
    ASSERT_EQ(found, this->topic1);
}

TEST_F(FindTopic, find_last)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::find<dds::topic::Topic<Space::Type1> >(this->dp, TOPIC1_NAME_3);
    ASSERT_FALSE(found.is_nil());
    ASSERT_EQ(found, this->topic3);
}

TEST_F(FindTopic, find_any)
{
    dds::topic::AnyTopic found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::find<dds::topic::AnyTopic>(this->dp, TOPIC1_NAME_2);
    ASSERT_FALSE(found.is_nil());
    ASSERT_EQ(found, this->topic2);
}

TEST_F(FindTopic, find_description)
{
    dds::topic::TopicDescription found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::find<dds::topic::TopicDescription>(this->dp, TOPIC1_NAME_2);
    ASSERT_FALSE(found.is_nil());
    ASSERT_EQ(found, this->topic2);
}

TEST_F(FindTopic, discover_with_null)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    ASSERT_THROW({
        found = dds::topic::discover<dds::topic::Topic<Space::Type1> >(dds::core::null, std::string(TOPIC1_NAME_1));
    }, dds::core::NullReferenceError);
}

TEST_F(FindTopic, discover_with_empty)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    found = dds::topic::discover<dds::topic::Topic<Space::Type1> >(this->dp, std::string(TOPIC1_NAME_1), dds::core::Duration::from_millisecs(10));
    ASSERT_EQ(found, dds::core::null);
}

TEST_F(FindTopic, discover_with_other_type)
{
    dds::topic::Topic<Space::Type2> found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::discover<dds::topic::Topic<Space::Type2> >(this->dp, TOPIC1_NAME_2, dds::core::Duration::from_millisecs(10));
    ASSERT_TRUE(found.is_nil());
}

TEST_F(FindTopic, discover_nonexisting)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::discover<dds::topic::Topic<Space::Type1> >(this->dp, std::string("non-existing"), dds::core::Duration::zero());
    ASSERT_EQ(found, dds::core::null);
}

TEST_F(FindTopic, discover_first)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::discover<dds::topic::Topic<Space::Type1> >(this->dp, TOPIC1_NAME_1);
    ASSERT_FALSE(found.is_nil());
    /* New topic object. */
    ASSERT_NE(found, this->topic1);
    /* With the same name. */
    ASSERT_STREQ(found.name().c_str(), this->topic1.name().c_str());
}

TEST_F(FindTopic, discover_last)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::discover<dds::topic::Topic<Space::Type1> >(this->dp, TOPIC1_NAME_3);
    ASSERT_FALSE(found.is_nil());
    /* New topic object. */
    ASSERT_NE(found, this->topic3);
    /* With the same name. */
    ASSERT_STREQ(found.name().c_str(), this->topic3.name().c_str());
}

TEST_F(FindTopic, discover_any)
{
    dds::topic::AnyTopic found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::discover<dds::topic::AnyTopic>(this->dp, TOPIC1_NAME_2);
    /* New topic object. */
    ASSERT_NE(found, this->topic2);
    /* With the same name. */
    ASSERT_STREQ(found.name().c_str(), this->topic2.name().c_str());
}

TEST_F(FindTopic, discover_description)
{
    dds::topic::TopicDescription found = dds::core::null;
    this->CreateTopics();
    found = dds::topic::discover<dds::topic::TopicDescription>(this->dp, TOPIC1_NAME_2);
    /* New topic object. */
    ASSERT_NE(found, this->topic2);
    /* With the same name. */
    ASSERT_STREQ(found.name().c_str(), this->topic2.name().c_str());
}

TEST_F(FindTopic, discover_list_with_null)
{
    ASSERT_THROW({
        (void)dds::topic::discover<dds::topic::Topic<Space::Type1> >
                                                (dds::core::null,
                                                 this->topicList.begin(),
                                                 static_cast<uint32_t>(this->topicList.size()));
    }, dds::core::NullReferenceError);
}

TEST_F(FindTopic, discover_list)
{
    /* TODO: This has to be implemented.
     * When implemented, more tests have to be added. */
    ASSERT_THROW({
        (void)dds::topic::discover<dds::topic::Topic<Space::Type1> >
                                                (this->dp,
                                                 this->topicList.begin(),
                                                 static_cast<uint32_t>(this->topicList.size()));
    }, dds::core::UnsupportedError);
}

/* Disabled because dds_find_topic changed behaviour. */
TEST_F(FindTopic, discover_on_2nd)
{
    dds::topic::Topic<Space::Type1> found = dds::core::null;
    this->CreateTopics();
    this->Create2ndParticipant();
    found = dds::topic::discover<dds::topic::Topic<Space::Type1> >(this->dp2, TOPIC1_NAME_3);
    ASSERT_FALSE(found.is_nil());
    /* New topic object. */
    ASSERT_NE(found, this->topic3);
    /* With the same name. */
    ASSERT_STREQ(found.name().c_str(), this->topic3.name().c_str());
}

TEST_F(FindTopic, discover_any_on_2nd)
{
    dds::topic::AnyTopic found = dds::core::null;
    this->CreateTopics();
    this->Create2ndParticipant();
    found = dds::topic::discover<dds::topic::AnyTopic>(this->dp2, TOPIC1_NAME_2);
    /* New topic object. */
    ASSERT_NE(found, this->topic2);
    /* With the same name. */
    ASSERT_STREQ(found.name().c_str(), this->topic2.name().c_str());
}

TEST_F(FindTopic, discover_description_on_2nd)
{
    dds::topic::TopicDescription found = dds::core::null;
    this->CreateTopics();
    this->Create2ndParticipant();
    found = dds::topic::discover<dds::topic::TopicDescription>(this->dp2, TOPIC1_NAME_2);
    /* New topic object. */
    ASSERT_NE(found, this->topic2);
    /* With the same name. */
    ASSERT_STREQ(found.name().c_str(), this->topic2.name().c_str());
}
