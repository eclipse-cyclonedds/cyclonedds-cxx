// Copyright(c) 2006 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

/**
 * @file
 */

#include <dds/sub/DataReader.hpp>
#include <dds/sub/Subscriber.hpp>
#include <dds/topic/BuiltinTopic.hpp>
#include <dds/topic/detail/find.hpp>
#include <dds/topic/detail/discovery.hpp>
#include <dds/sub/detail/TDataReaderImpl.hpp>

#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/sub/BuiltinSubscriberDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>

#include <org/eclipse/cyclonedds/topic/BuiltinTopicTraits.hpp>

#include "dds/dds.h"

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace sub
{
template <typename T>
dds::sub::DataReader<T>
create_builtin_reader(
    SubscriberDelegate& subscriber,
    const std::string& topic_name);

}
}
}
}

org::eclipse::cyclonedds::core::Mutex
org::eclipse::cyclonedds::sub::BuiltinSubscriberDelegate::builtinLock;

org::eclipse::cyclonedds::sub::BuiltinSubscriberDelegate::BuiltinSubscriberDelegate(
    const dds::domain::DomainParticipant& dp,
    const dds::sub::qos::SubscriberQos& qos) :
        SubscriberDelegate(dp, qos, NULL, dds::core::status::StatusMask::none())
{

}


std::vector<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type>
org::eclipse::cyclonedds::sub::BuiltinSubscriberDelegate::find_datareaders(
    const std::string& topic_name)
{
    std::vector<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type> list;

    list = SubscriberDelegate::find_datareaders(topic_name);
    if (list.size() == 0) {
        org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type reader = get_builtin_reader(*this, topic_name);
        if (reader) {
            list.push_back(reader);
        }
    }

    return list;
}

org::eclipse::cyclonedds::sub::SubscriberDelegate::ref_type
org::eclipse::cyclonedds::sub::BuiltinSubscriberDelegate::get_builtin_subscriber(
        const dds::domain::DomainParticipant& dp)
{
    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(builtinLock);

    SubscriberDelegate::ref_type builtin_subscriber;

    org::eclipse::cyclonedds::core::EntityDelegate::ref_type entity = dp.delegate()->builtin_subscriber();
    if (entity) {
        builtin_subscriber = ::std::dynamic_pointer_cast<SubscriberDelegate>(entity);
    } else {
        dds::sub::qos::SubscriberQos qos;

        qos << dds::core::policy::PresentationAccessScopeKind::TOPIC;
        qos << dds::core::policy::Partition("__BUILT-IN PARTITION__");

        builtin_subscriber.reset(new org::eclipse::cyclonedds::sub::BuiltinSubscriberDelegate(dp, qos));
        builtin_subscriber->init(builtin_subscriber);
        dp.delegate()->builtin_subscriber(builtin_subscriber);
    }

    return builtin_subscriber;
}


template <typename T>
dds::sub::DataReader<T>
org::eclipse::cyclonedds::sub::create_builtin_reader(
    SubscriberDelegate& subscriber,
    const std::string& topic_name)
{
    dds::sub::qos::DataReaderQos rQos;

    dds::topic::Topic<T> topic =
            dds::topic::find< dds::topic::Topic<T> >(subscriber.participant(), topic_name);
    if (topic.is_nil()) {
        topic = dds::topic::discover< dds::topic::Topic<T> >(subscriber.participant(), topic_name, dds::core::Duration::zero());
        if (topic.is_nil()) {
            ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "Could not find builtin topic \"%s\"", topic_name.c_str());
        }
    }
    subscriber.default_datareader_qos(rQos);
    rQos = topic.qos();
    dds::sub::DataReader<T> reader(subscriber.wrapper(), topic, rQos);

    return reader;
}

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace topic
{

template<>
ddsi_sertype *TopicTraits<dds::topic::ParticipantBuiltinTopicData>::getSerType(allowable_encodings_t)
{
    return nullptr;
}

template<>
ddsi_sertype *TopicTraits<dds::topic::TopicBuiltinTopicData>::getSerType(allowable_encodings_t)
{
    return nullptr;
}

template<>
ddsi_sertype *TopicTraits<dds::topic::PublicationBuiltinTopicData>::getSerType(allowable_encodings_t)
{
    return nullptr;
}

template<>
ddsi_sertype *TopicTraits<dds::topic::SubscriptionBuiltinTopicData>::getSerType(allowable_encodings_t)
{
    return nullptr;
}

} // topic
} // cyclonedds
} // eclipse
} // org
    
org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type
org::eclipse::cyclonedds::sub::BuiltinSubscriberDelegate::get_builtin_reader(
    SubscriberDelegate& subscriber,
    const std::string& topic_name)
{
    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(builtinLock);

    org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type builtin_reader;

    if (topic_name == "DCPSParticipant") {
        builtin_reader = create_builtin_reader<dds::topic::ParticipantBuiltinTopicData>(subscriber, topic_name).delegate();
    } else if (topic_name == "DCPSTopic") {
        builtin_reader = create_builtin_reader<dds::topic::TopicBuiltinTopicData>(subscriber, topic_name).delegate();
    } else if (topic_name == "DCPSPublication") {
        builtin_reader = create_builtin_reader<dds::topic::PublicationBuiltinTopicData>(subscriber, topic_name).delegate();
    } else if (topic_name == "DCPSSubscription") {
        builtin_reader = create_builtin_reader<dds::topic::SubscriptionBuiltinTopicData>(subscriber, topic_name).delegate();
    }

    return builtin_reader;
}
