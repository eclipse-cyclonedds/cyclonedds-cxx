// Copyright(c) 2006 to 2021 ZettaScale Technology and others
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

#include <dds/topic/AnyTopic.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>
#include <org/eclipse/cyclonedds/core/EntityDelegate.hpp>
#include <org/eclipse/cyclonedds/topic/AnyTopicDelegate.hpp>
#include "dds/dds.h"

#define MAX_TOPIC_NAME_LENGTH 256

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace topic
{

/* For dynamic casting to AnyTopicDelegate to work for a few older compilers,
 * it is needed that (at least) the constructor is moved to the cpp file. */
AnyTopicDelegate::AnyTopicDelegate(
        const dds::topic::qos::TopicQos& qos,
        const dds::domain::DomainParticipant& dp,
        const std::string& name,
        const std::string& type_name)
    : org::eclipse::cyclonedds::core::EntityDelegate(),
      org::eclipse::cyclonedds::topic::TopicDescriptionDelegate(dp, name, type_name),
      qos_(qos),
      sample_(NULL)
{
}

AnyTopicDelegate::AnyTopicDelegate(
        const dds::topic::qos::TopicQos& qos,
        const dds::domain::DomainParticipant& dp,
        const std::string& name,
        const std::string& type_name,
        dds_entity_t ddsc_topic)
    : org::eclipse::cyclonedds::core::EntityDelegate(),
      org::eclipse::cyclonedds::topic::TopicDescriptionDelegate(dp, name, type_name),
      qos_(qos),
      sample_(NULL)
{
    this->ddsc_entity = ddsc_topic;
}


AnyTopicDelegate::~AnyTopicDelegate()
{
}

void
AnyTopicDelegate::init(ObjectDelegate::weak_ref_type weak_ref)
{
    /* Set weak_ref before passing ourselves to other isocpp objects. */
    this->set_weak_ref(weak_ref);
    /* Add weak_ref to the map of entities */
    this->add_to_entity_map(weak_ref);
    /* Register topic at participant. */
    this->myParticipant.delegate()->add_topic(*this);

}

dds::topic::qos::TopicQos
AnyTopicDelegate::qos() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    return qos_;
}

void
AnyTopicDelegate::qos(const dds::topic::qos::TopicQos& qos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds_return_t ddsc_ret;

    // get and validate the ddsc qos
    org::eclipse::cyclonedds::topic::qos::TopicQosDelegate tQos = qos.delegate();
    tQos.check();
    dds_qos_t* ddsc_topic = tQos.ddsc_qos();
    if (!ddsc_topic) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_OUT_OF_RESOURCES_ERROR, "Could not convert topic qos.");
    }
    ddsc_ret = dds_set_qos(ddsc_entity, ddsc_topic);
    dds_delete_qos(ddsc_topic);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_ret, "Could not set topic qos.");

    qos_ = qos;
}

std::string
AnyTopicDelegate::reader_expression() const
{
    std::string rExpr;
    rExpr += "select * from ";
    rExpr += org::eclipse::cyclonedds::topic::TopicDescriptionDelegate::myTopicName;
    return rExpr;
}

TEMP_TYPE
//@todo c_value *
AnyTopicDelegate::reader_parameters() const
{
    return NULL;
}

::dds::core::status::InconsistentTopicStatus
AnyTopicDelegate::inconsistent_topic_status() const
{
    org::eclipse::cyclonedds::core::ScopedLock<org::eclipse::cyclonedds::core::EntityDelegate> scopedLock(*this);

    dds_inconsistent_topic_status_t ddsc_stat;
    dds_return_t ddsc_ret = dds_get_inconsistent_topic_status(ddsc_entity, &ddsc_stat);

    ::dds::core::status::InconsistentTopicStatus status;
    status->ddsc_status(&ddsc_stat);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_ret, "dds_get_inconsistent_topic_status failed.");

    return status;
}

dds::topic::TAnyTopic<AnyTopicDelegate>
AnyTopicDelegate::wrapper_to_any()
{
    AnyTopicDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<AnyTopicDelegate>(this->get_strong_ref());
    dds::topic::AnyTopic any_topic(ref);
    return any_topic;
}

dds::topic::TAnyTopic<AnyTopicDelegate>
AnyTopicDelegate::discover_topic(
        const dds::domain::DomainParticipant& dp,
        const std::string& name,
        const dds::core::Duration& timeout)
{
    char nameBuf[MAX_TOPIC_NAME_LENGTH];

    dds_entity_t ddsc_topic = dp.delegate()->lookup_topic(name, NULL, timeout);

    if (ddsc_topic <= 0) {
        return dds::core::null;
    }

    dds_return_t ret = dds_get_type_name(ddsc_topic, nameBuf, MAX_TOPIC_NAME_LENGTH);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Failed to extract type_name from discovered topic");
    std::string type_name = nameBuf;

    dds_qos_t* ddsc_qos = dds_create_qos();
    ret = dds_get_qos(ddsc_topic, ddsc_qos);
    dds::topic::qos::TopicQos qos;
    if (ret == DDS_RETCODE_OK) {
        qos.delegate().ddsc_qos(ddsc_qos);
    }
    dds_delete_qos(ddsc_qos);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Failed to get the qos from discovered topic");

    ref_type ref(new AnyTopicDelegate(qos, dp, name, type_name, ddsc_topic));
    ref->init(ref);

    return dds::topic::TAnyTopic<AnyTopicDelegate>(ref);
}

void
AnyTopicDelegate::discover_topics(
        const dds::domain::DomainParticipant& dp,
        std::vector<dds::topic::TAnyTopic<AnyTopicDelegate> >& topics,
        uint32_t max_size)
{
    (void)dp;
    (void)topics;
    (void)max_size;
    topics.clear();
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Discovery of AnyTopics not implemented yet");
}

void AnyTopicDelegate::set_sample(void* sample)
{
    sample_ = sample;
}

void* AnyTopicDelegate::get_sample()
{
    return sample_;
}

}
}
}
}
