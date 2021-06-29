/*
 * Copyright(c) 2021 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */


/**
 * @file
 */

#include <dds/topic/BuiltinTopic.hpp>

namespace dds
{

namespace topic
{

class default_builtin_topic_qos: public qos::TopicQos {
    public:
        default_builtin_topic_qos(): qos::TopicQos() {
            const char *partition = "__BUILT-IN PARTITION__";
            dds_qos_t *qos = dds_create_qos ();
            dds_qset_durability (qos, DDS_DURABILITY_TRANSIENT_LOCAL);
            dds_qset_presentation (qos, DDS_PRESENTATION_TOPIC, false, false);
            dds_qset_reliability (qos, DDS_RELIABILITY_RELIABLE, DDS_MSECS(100));
            dds_qset_partition (qos, 1, &partition);
            delegate().ddsc_qos(qos);
            dds_delete_qos(qos);
        }
};

template <>
Topic<ParticipantBuiltinTopicData, detail::Topic>::Topic(const dds::domain::DomainParticipant& dp,
                          const std::string& topic_name) :
::dds::core::Reference< detail::Topic<ParticipantBuiltinTopicData> >(new detail::Topic<ParticipantBuiltinTopicData>(
              dp,
              topic_name,
              "org::eclipse::cyclonedds::builtin::DCPSParticipant",
              default_builtin_topic_qos(),
              DDS_BUILTIN_TOPIC_DCPSPARTICIPANT))
{
    this->delegate()->init(this->impl_);
}

template <>
Topic<TopicBuiltinTopicData, detail::Topic>::Topic(const dds::domain::DomainParticipant& dp,
                          const std::string& topic_name) :
::dds::core::Reference< detail::Topic<TopicBuiltinTopicData> >(new detail::Topic<TopicBuiltinTopicData>(
              dp,
              topic_name,
              "org::eclipse::cyclonedds::builtin::DCPSTopic",
              default_builtin_topic_qos(),
              DDS_BUILTIN_TOPIC_DCPSTOPIC))
{
    this->delegate()->init(this->impl_);
}

template <>
Topic<SubscriptionBuiltinTopicData, detail::Topic>::Topic(const dds::domain::DomainParticipant& dp,
                          const std::string& topic_name) :
::dds::core::Reference< detail::Topic<SubscriptionBuiltinTopicData> >(new detail::Topic<SubscriptionBuiltinTopicData>(
              dp,
              topic_name,
              "org::eclipse::cyclonedds::builtin::DCPSSubscription",
              default_builtin_topic_qos(),
              DDS_BUILTIN_TOPIC_DCPSSUBSCRIPTION))
{
    this->delegate()->init(this->impl_);
}

template <>
Topic<PublicationBuiltinTopicData, detail::Topic>::Topic(const dds::domain::DomainParticipant& dp,
                          const std::string& topic_name) :
::dds::core::Reference< detail::Topic<PublicationBuiltinTopicData> >(new detail::Topic<PublicationBuiltinTopicData>(
              dp,
              topic_name,
              "org::eclipse::cyclonedds::builtin::DCPSPublication",
              default_builtin_topic_qos(),
              DDS_BUILTIN_TOPIC_DCPSPUBLICATION))
{
    this->delegate()->init(this->impl_);
}

}

}
