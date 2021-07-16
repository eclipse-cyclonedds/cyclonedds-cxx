#ifndef OMG_DDS_TOPIC_BUILTIN_TOPIC_HPP_
#define OMG_DDS_TOPIC_BUILTIN_TOPIC_HPP_

/* Copyright 2010, Object Management Group, Inc.
 * Copyright 2010, PrismTech, Corp.
 * Copyright 2010, Real-Time Innovations, Inc.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <dds/topic/detail/BuiltinTopic.hpp>
#include <dds/topic/TopicTraits.hpp>

namespace dds
{
namespace topic
{

typedef dds::topic::detail::ParticipantBuiltinTopicData ParticipantBuiltinTopicData;

typedef dds::topic::detail::TopicBuiltinTopicData TopicBuiltinTopicData;

typedef dds::topic::detail::PublicationBuiltinTopicData PublicationBuiltinTopicData;

typedef dds::topic::detail::SubscriptionBuiltinTopicData SubscriptionBuiltinTopicData;

template <>
Topic<ParticipantBuiltinTopicData, detail::Topic>::Topic(
    const dds::domain::DomainParticipant& dp,
    const std::string& topic_name);

template <>
Topic<TopicBuiltinTopicData, detail::Topic>::Topic(
    const dds::domain::DomainParticipant& dp,
    const std::string& topic_name);

template <>
Topic<PublicationBuiltinTopicData, detail::Topic>::Topic(
    const dds::domain::DomainParticipant& dp,
    const std::string& topic_name);

template <>
Topic<SubscriptionBuiltinTopicData, detail::Topic>::Topic(
    const dds::domain::DomainParticipant& dp,
    const std::string& topic_name);

} /* topic */
} /* dds */

REGISTER_TOPIC_TYPE(ParticipantBuiltinTopicData)
REGISTER_TOPIC_TYPE(TopicBuiltinTopicData)
REGISTER_TOPIC_TYPE(PublicationBuiltinTopicData)
REGISTER_TOPIC_TYPE(SubscriptionBuiltinTopicData)

template<typename T>
using IsBuiltinTopicType = std::enable_if_t<
        std::is_same<T,dds::topic::ParticipantBuiltinTopicData>::value
     || std::is_same<T,dds::topic::TopicBuiltinTopicData>::value
     || std::is_same<T,dds::topic::SubscriptionBuiltinTopicData>::value
     || std::is_same<T,dds::topic::PublicationBuiltinTopicData>::value, bool >;

#endif /* OMG_DDS_TOPIC_BUILTIN_TOPIC_HPP_ */
