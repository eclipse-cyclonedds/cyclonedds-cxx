// Copyright(c) 2006 to 2020 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <org/eclipse/cyclonedds/topic/find.hpp>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace topic
{

dds::topic::TopicDescription
find_topic_description(
    const dds::domain::DomainParticipant& dp,
    const std::string& topic_name)
{
    dds::topic::TopicDescription t = dds::core::null;

    org::eclipse::cyclonedds::core::ObjectDelegate::ref_type entity = dp.delegate()->find_topic(topic_name);
    if (!entity) {
        entity = dp.delegate()->find_cfTopic(topic_name);
    }
    if (entity) {
        dds::topic::TopicDescription::DELEGATE_REF_T ref =
                ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::topic::TopicDescriptionDelegate>(entity);
        t = dds::topic::TopicDescription(ref);
    }

    return t;
}

dds::topic::AnyTopic
find_any_topic(
    const dds::domain::DomainParticipant& dp,
    const std::string& topic_name)
{
    dds::topic::AnyTopic t = dds::core::null;

    org::eclipse::cyclonedds::core::EntityDelegate::ref_type entity = dp.delegate()->find_topic(topic_name);
    if (entity) {
        dds::topic::AnyTopic::DELEGATE_REF_T ref =
                ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::topic::AnyTopicDelegate>(entity);
        t = dds::topic::AnyTopic(ref);
    }

    return t;
}


}
}
}
}
