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

#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/topic/qos/TopicQosDelegate.hpp>

#include <cassert>

#include "dds/ddsi/ddsi_plist.h"

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace topic
{
namespace qos
{

TopicQosDelegate::TopicQosDelegate()
{
    ddsc_qos(&ddsi_default_qos_topic);
    present() &= ~DDSI_QP_DATA_REPRESENTATION;
    check();
}

void
TopicQosDelegate::policy(const dds::core::policy::TopicData& topic_data)
{
    topic_data.delegate().check();
    present_ |= DDSI_QP_TOPIC_DATA;
    topic_data_ = topic_data;
}

void
TopicQosDelegate::policy(const dds::core::policy::Durability& durability)
{
    durability.delegate().check();
    present_ |= DDSI_QP_DURABILITY;
    durability_ = durability;
}

#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
void
TopicQosDelegate::policy(const dds::core::policy::DurabilityService& durability_service)
{
    durability_service.delegate().check();
    present_ |= DDSI_QP_DURABILITY_SERVICE;
    durability_service_ = durability_service;
}
#endif  // OMG_DDS_PERSISTENCE_SUPPORT

void
TopicQosDelegate::policy(const dds::core::policy::Deadline& deadline)
{
    deadline.delegate().check();
    present_ |= DDSI_QP_DEADLINE;
    deadline_ = deadline;
}

void
TopicQosDelegate::policy(const dds::core::policy::LatencyBudget&  budget)
{
    budget.delegate().check();
    present_ |= DDSI_QP_LATENCY_BUDGET;
    budget_ = budget;
}

void
TopicQosDelegate::policy(const dds::core::policy::Liveliness& liveliness)
{
    liveliness.delegate().check();
    present_ |= DDSI_QP_LIVELINESS;
    liveliness_ = liveliness;
}

void
TopicQosDelegate::policy(const dds::core::policy::Reliability& reliability)
{
    reliability.delegate().check();
    present_ |= DDSI_QP_RELIABILITY;
    reliability_ = reliability;
}

void
TopicQosDelegate::policy(const dds::core::policy::DestinationOrder& order)
{
    order.delegate().check();
    present_ |= DDSI_QP_DESTINATION_ORDER;
    order_ = order;
}

void
TopicQosDelegate::policy(const dds::core::policy::History& history)
{
    history.delegate().check();
    present_ |= DDSI_QP_HISTORY;
    history_ = history;
}

void
TopicQosDelegate::policy(const dds::core::policy::ResourceLimits& resources)
{
    resources.delegate().check();
    present_ |= DDSI_QP_RESOURCE_LIMITS;
    resources_ = resources;
}

void
TopicQosDelegate::policy(const dds::core::policy::TransportPriority& priority)
{
    priority.delegate().check();
    present_ |= DDSI_QP_TRANSPORT_PRIORITY;
    priority_ = priority;
}

void
TopicQosDelegate::policy(const dds::core::policy::Lifespan& lifespan)
{
    lifespan.delegate().check();
    present_ |= DDSI_QP_LIFESPAN;
    lifespan_ = lifespan;
}

void
TopicQosDelegate::policy(const dds::core::policy::Ownership& ownership)
{
    ownership.delegate().check();
    present_ |= DDSI_QP_OWNERSHIP;
    ownership_ = ownership;
}

#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
void
TopicQosDelegate::policy(const dds::core::policy::DataRepresentation& datarepresentation)
{
    datarepresentation.delegate().check();
    present_ |= DDSI_QP_DATA_REPRESENTATION;
    datarepresentation_ = datarepresentation;
}

void
TopicQosDelegate::policy(const dds::core::policy::TypeConsistencyEnforcement& typeconsistencyenforcement)
{
    typeconsistencyenforcement.delegate().check();
    present_ |= DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT;
    typeconsistencyenforcement_ = typeconsistencyenforcement;
}
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

dds_qos_t*
TopicQosDelegate::ddsc_qos() const
{
    dds_qos_t* qos = dds_create_qos();
    if (!qos) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_OUT_OF_RESOURCES_ERROR, "Could not create internal QoS.");
    }
    if (present_ & DDSI_QP_TOPIC_DATA)
        topic_data_  .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_DURABILITY)
        durability_  .delegate().set_c_policy(qos);
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    if (present_ & DDSI_QP_DURABILITY_SERVICE)
        durability_service_.delegate().set_c_policy(qos);
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
    if (present_ & DDSI_QP_DEADLINE)
        deadline_    .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_LATENCY_BUDGET)
        budget_      .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_LIVELINESS)
        liveliness_  .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_RELIABILITY)
        reliability_ .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_DESTINATION_ORDER)
        order_       .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_HISTORY)
        history_     .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_RESOURCE_LIMITS)
        resources_   .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_TRANSPORT_PRIORITY)
        priority_    .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_LIFESPAN)
        lifespan_    .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_OWNERSHIP)
        ownership_   .delegate().set_c_policy(qos);
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    if (present_ & DDSI_QP_DATA_REPRESENTATION)
        datarepresentation_.delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT)
        typeconsistencyenforcement_.delegate().set_c_policy(qos);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    return qos;
}

void
TopicQosDelegate::ddsc_qos(const dds_qos_t* qos)
{
    assert(qos);
    present_ = qos->present;
    if (present_ & DDSI_QP_TOPIC_DATA)
        topic_data_  .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_DURABILITY)
        durability_  .delegate().set_iso_policy(qos);
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    if (present_ & DDSI_QP_DURABILITY_SERVICE)
        durability_service_.delegate().set_iso_policy(qos);
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
    if (present_ & DDSI_QP_DEADLINE)
        deadline_    .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_LATENCY_BUDGET)
        budget_      .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_LIVELINESS)
        liveliness_  .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_RELIABILITY)
        reliability_ .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_DESTINATION_ORDER)
        order_       .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_HISTORY)
        history_     .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_RESOURCE_LIMITS)
        resources_   .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_TRANSPORT_PRIORITY)
        priority_    .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_LIFESPAN)
        lifespan_    .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_OWNERSHIP)
        ownership_   .delegate().set_iso_policy(qos);
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    if (present_ & DDSI_QP_DATA_REPRESENTATION)
        datarepresentation_.delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT)
        typeconsistencyenforcement_.delegate().set_iso_policy(qos);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
}

void
TopicQosDelegate::named_qos(const struct _DDS_NamedTopicQos &qos)
{
    (void)qos;
#if 0
    /* We only need the QoS part of the named QoS. */
    const struct _DDS_TopicQos *q = &qos.topic_qos;
    /* The idl policies are aligned the same as the ddsc/builtin representation.
     * So, cast and use the ddsc policy translations (or builtin when available). */
    topic_data_  .delegate().v_policy((v_builtinTopicDataPolicy&)(q->topic_data)        );
    durability_  .delegate().v_policy((v_durabilityPolicy&)      (q->durability)        );
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    durability_service_.delegate().v_policy((v_durabilityServicePolicy&)(q->durability_service));
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
    deadline_    .delegate().v_policy((v_deadlinePolicy&)        (q->deadline)          );
    budget_      .delegate().v_policy((v_latencyPolicy&)         (q->latency_budget)    );
    liveliness_  .delegate().v_policy((v_livelinessPolicy&)      (q->liveliness)        );
    reliability_ .delegate().v_policy((v_reliabilityPolicy&)     (q->reliability)       );
    order_       .delegate().v_policy((v_orderbyPolicy&)         (q->destination_order) );
    history_     .delegate().v_policy((v_historyPolicy&)         (q->history)           );
    resources_   .delegate().v_policy((v_resourcePolicy&)        (q->resource_limits)   );
    priority_    .delegate().v_policy((v_transportPolicy&)       (q->transport_priority));
    lifespan_    .delegate().v_policy((v_lifespanPolicy&)        (q->lifespan)          );
    ownership_   .delegate().v_policy((v_ownershipPolicy&)       (q->ownership)         );
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    datarepresentation_.delegate().v_policy((v_dataRepresentationPolicy&)(q->datarepresentation));
    typeconsistencyenforcement_.delegate().v_policy((v_TypeConsistencyEnforcementPolicy&)(q->typeconsistencyenforcement));
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
#endif
}

void
TopicQosDelegate::check() const
{
    /* Policies are checked when set.
     * But consistency between policies needs to be checked. */
    history_.delegate().check_against(resources_.delegate());
}

bool
TopicQosDelegate::operator ==(const TopicQosDelegate& other) const
{
    return other.present_     == present_ &&
           other.topic_data_  == topic_data_  &&
           other.durability_  == durability_  &&
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
           other.durability_service_ == durability_service_ &&
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
           other.deadline_    == deadline_    &&
           other.budget_      == budget_      &&
           other.liveliness_  == liveliness_  &&
           other.reliability_ == reliability_ &&
           other.order_       == order_       &&
           other.history_     == history_     &&
           other.resources_   == resources_   &&
           other.priority_    == priority_    &&
           other.lifespan_    == lifespan_    &&
           other.ownership_   == ownership_
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
        && other.datarepresentation_ == datarepresentation_
        && other.typeconsistencyenforcement_ == typeconsistencyenforcement_
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
           ;
}

template<> dds::core::policy::TopicData&
TopicQosDelegate::policy<dds::core::policy::TopicData>()
{
    present_ |= DDSI_QP_TOPIC_DATA;
    return topic_data_;
}

template<> dds::core::policy::Durability&
TopicQosDelegate::policy<dds::core::policy::Durability>()
{
    present_ |= DDSI_QP_DURABILITY;
    return durability_;
}

#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
template<> dds::core::policy::DurabilityService&
TopicQosDelegate::policy<dds::core::policy::DurabilityService>()
{
    present_ |= DDSI_QP_DURABILITY_SERVICE;
    return durability_service_;
}
#endif  // OMG_DDS_PERSISTENCE_SUPPORT

template<> dds::core::policy::Deadline&
TopicQosDelegate::policy<dds::core::policy::Deadline>()
{
    present_ |= DDSI_QP_DEADLINE;
    return deadline_;
}

template<> dds::core::policy::LatencyBudget&
TopicQosDelegate::policy<dds::core::policy::LatencyBudget>()
{
    present_ |= DDSI_QP_LATENCY_BUDGET;
    return budget_;
}

template<> dds::core::policy::Liveliness&
TopicQosDelegate::policy<dds::core::policy::Liveliness>()
{
    present_ |= DDSI_QP_LIVELINESS;
    return liveliness_;
}

template<> dds::core::policy::Reliability&
TopicQosDelegate::policy<dds::core::policy::Reliability>()
{
    present_ |= DDSI_QP_RELIABILITY;
    return reliability_;
}

template<> dds::core::policy::DestinationOrder&
TopicQosDelegate::policy<dds::core::policy::DestinationOrder>()
{
    present_ |= DDSI_QP_DESTINATION_ORDER;
    return order_;
}

template<> dds::core::policy::History&
TopicQosDelegate::policy<dds::core::policy::History>()
{
    present_ |= DDSI_QP_HISTORY;
    return history_;
}

template<> dds::core::policy::ResourceLimits&
TopicQosDelegate::policy<dds::core::policy::ResourceLimits>()
{
    present_ |= DDSI_QP_RESOURCE_LIMITS;
    return resources_;
}

template<> dds::core::policy::TransportPriority&
TopicQosDelegate::policy<dds::core::policy::TransportPriority>()
{
    present_ |= DDSI_QP_TRANSPORT_PRIORITY;
    return priority_;
}

template<> dds::core::policy::Lifespan&
TopicQosDelegate::policy<dds::core::policy::Lifespan>()
{
    present_ |= DDSI_QP_LIFESPAN;
    return lifespan_;
}

template<> dds::core::policy::Ownership&
TopicQosDelegate::policy<dds::core::policy::Ownership>()
{
    present_ |= DDSI_QP_OWNERSHIP;
    return ownership_;
}

#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
template<> dds::core::policy::DataRepresentation&
TopicQosDelegate::policy<dds::core::policy::DataRepresentation>()
{
    present_ |= DDSI_QP_DATA_REPRESENTATION;
    return datarepresentation_;
}

template<> dds::core::policy::TypeConsistencyEnforcement&
TopicQosDelegate::policy<dds::core::policy::TypeConsistencyEnforcement>()
{
    present_ |= DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT;
    return typeconsistencyenforcement_;
}
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

}
}
}
}
}
