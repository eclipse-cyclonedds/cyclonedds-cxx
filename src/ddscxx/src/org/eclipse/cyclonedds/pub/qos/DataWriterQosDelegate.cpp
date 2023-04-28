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
#include <org/eclipse/cyclonedds/pub/qos/DataWriterQosDelegate.hpp>

#include <cassert>

#include "dds/ddsi/ddsi_plist.h"

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace pub
{
namespace qos
{

DataWriterQosDelegate::DataWriterQosDelegate()
{
    ddsc_qos(&ddsi_default_qos_writer);
    present() &= ~DDSI_QP_DATA_REPRESENTATION;
    check();
}

DataWriterQosDelegate::DataWriterQosDelegate(
    const org::eclipse::cyclonedds::topic::qos::TopicQosDelegate& tqos): DataWriterQosDelegate()
{
    *this = tqos;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::UserData& user_data)
{
    user_data.delegate().check();
    present_ |= DDSI_QP_USER_DATA;
    user_data_ = user_data;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Durability& durability)
{
    durability.delegate().check();
    present_ |= DDSI_QP_DURABILITY;
    durability_ = durability;
}

#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
void
DataWriterQosDelegate::policy(const dds::core::policy::DurabilityService& durability_service)
{
    durability_service.delegate().check();
    present_ |= DDSI_QP_DURABILITY_SERVICE;
    durability_service_ = durability_service;
}
#endif  // OMG_DDS_PERSISTENCE_SUPPORT

void
DataWriterQosDelegate::policy(const dds::core::policy::Deadline& deadline)
{
    deadline.delegate().check();
    present_ |= DDSI_QP_DEADLINE;
    deadline_ = deadline;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::LatencyBudget&  budget)
{
    budget.delegate().check();
    present_ |= DDSI_QP_LATENCY_BUDGET;
    budget_ = budget;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Liveliness& liveliness)
{
    liveliness.delegate().check();
    present_ |= DDSI_QP_LIVELINESS;
    liveliness_ = liveliness;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Reliability& reliability)
{
    reliability.delegate().check();
    present_ |= DDSI_QP_RELIABILITY;
    reliability_ = reliability;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::DestinationOrder& order)
{
    order.delegate().check();
    present_ |= DDSI_QP_DESTINATION_ORDER;
    order_ = order;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::History& history)
{
    history.delegate().check();
    present_ |= DDSI_QP_HISTORY;
    history_ = history;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::ResourceLimits& resources)
{
    resources.delegate().check();
    present_ |= DDSI_QP_RESOURCE_LIMITS;
    resources_ = resources;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::TransportPriority& priority)
{
    priority.delegate().check();
    present_ |= DDSI_QP_TRANSPORT_PRIORITY;
    priority_ = priority;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Lifespan& lifespan)
{
    lifespan.delegate().check();
    present_ |= DDSI_QP_LIFESPAN;
    lifespan_ = lifespan;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Ownership& ownership)
{
    ownership.delegate().check();
    present_ |= DDSI_QP_OWNERSHIP;
    ownership_ = ownership;
}

#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
void
DataWriterQosDelegate::policy(const dds::core::policy::OwnershipStrength& strength)
{
    strength.delegate().check();
    present_ |= DDSI_QP_OWNERSHIP_STRENGTH;
    strength_ = strength;
}
#endif  // OMG_DDS_OWNERSHIP_SUPPORT

void
DataWriterQosDelegate::policy(const dds::core::policy::WriterDataLifecycle& lifecycle)
{
    lifecycle.delegate().check();
    present_ |= DDSI_QP_ADLINK_WRITER_DATA_LIFECYCLE;
    lifecycle_ = lifecycle;
}

#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
void
DataWriterQosDelegate::policy(const dds::core::policy::DataRepresentation& datarepresentation)
{
    datarepresentation.delegate().check();
    present_ |= DDSI_QP_DATA_REPRESENTATION;
    datarepresentation_ = datarepresentation;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::TypeConsistencyEnforcement& typeconsistencyenforcement)
{
    typeconsistencyenforcement.delegate().check();
    present_ |= DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT;
    typeconsistencyenforcement_ = typeconsistencyenforcement;
}
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

dds_qos_t*
DataWriterQosDelegate::ddsc_qos() const
{
    dds_qos_t* qos = dds_create_qos();
    if (!qos) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_OUT_OF_RESOURCES_ERROR, "Could not create internal QoS.");
    }
    if (present_ & DDSI_QP_USER_DATA)
        user_data_   .delegate().set_c_policy(qos);
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
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
    if (present_ & DDSI_QP_OWNERSHIP_STRENGTH)
        strength_    .delegate().set_c_policy(qos);
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
    if (present_ & DDSI_QP_ADLINK_WRITER_DATA_LIFECYCLE)
        lifecycle_   .delegate().set_c_policy(qos);
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    if (present_ & DDSI_QP_DATA_REPRESENTATION)
        datarepresentation_.delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT)
        typeconsistencyenforcement_.delegate().set_c_policy(qos);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    return qos;
}

void
DataWriterQosDelegate::ddsc_qos(const dds_qos_t* qos)
{
    assert(qos);
    present_ = qos->present;
    if (present_ & DDSI_QP_USER_DATA)
        user_data_   .delegate().set_iso_policy(qos);
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
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
    if (present_ & DDSI_QP_OWNERSHIP_STRENGTH)
        strength_    .delegate().set_iso_policy(qos);
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
    if (present_ & DDSI_QP_ADLINK_WRITER_DATA_LIFECYCLE)
        lifecycle_   .delegate().set_iso_policy(qos);
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    if (present_ & DDSI_QP_DATA_REPRESENTATION)
        datarepresentation_.delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT)
        typeconsistencyenforcement_.delegate().set_iso_policy(qos);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
}

void
DataWriterQosDelegate::named_qos(const struct _DDS_NamedDataWriterQos &qos)
{
    (void)qos;
#if 0
    /* We only need the QoS part of the named QoS. */
    const struct _DDS_DataWriterQos *q = &qos.datawriter_qos;
    /* The idl policies are aligned the same as the ddsc/builtin representation.
     * So, cast and use the ddsc policy translations (or builtin when available). */
    user_data_   .delegate().v_policy((v_builtinUserDataPolicy&)(q->user_data)            );
    durability_  .delegate().v_policy((v_durabilityPolicy&)     (q->durability)           );
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    durability_service_.delegate().v_policy((v_durabilityServicePolicy&)(q->durability_service));
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
    deadline_    .delegate().v_policy((v_deadlinePolicy&)       (q->deadline)             );
    budget_      .delegate().v_policy((v_latencyPolicy&)        (q->latency_budget)       );
    liveliness_  .delegate().v_policy((v_livelinessPolicy&)     (q->liveliness)           );
    reliability_ .delegate().v_policy((v_reliabilityPolicy&)    (q->reliability)          );
    order_       .delegate().v_policy((v_orderbyPolicy&)        (q->destination_order)    );
    history_     .delegate().v_policy((v_historyPolicy&)        (q->history)              );
    resources_   .delegate().v_policy((v_resourcePolicy&)       (q->resource_limits)      );
    priority_    .delegate().v_policy((v_transportPolicy&)      (q->transport_priority)   );
    lifespan_    .delegate().v_policy((v_lifespanPolicy&)       (q->lifespan)             );
    ownership_   .delegate().v_policy((v_ownershipPolicy&)      (q->ownership)            );
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
    strength_    .delegate().v_policy((v_strengthPolicy&)       (q->ownership_strength)   );
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
    lifecycle_   .delegate().v_policy((v_writerLifecyclePolicy&)(q->writer_data_lifecycle));
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    datarepresentation_.delegate().v_policy((v_writerDataRepresentationPolicy&)(q->writer_datarepresentation));
    typeconsistencyenforcement_.delegate().v_policy((v_writerTypeConsistencyEnforcementPolicy&)(q->writer_typeconsistencyenforcement));
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
#endif
}

void
DataWriterQosDelegate::check() const
{
    /* Policies are checked when set.
     * But consistency between policies needs to be checked. */
    history_.delegate().check_against(resources_.delegate());
}

bool
DataWriterQosDelegate::operator ==(const DataWriterQosDelegate& other) const
{
    return other.present_     == present_     &&
           other.user_data_   == user_data_   &&
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
           other.priority_    ==  priority_   &&
           other.lifespan_    == lifespan_    &&
           other.ownership_   == ownership_   &&
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
           other.strength_    == strength_    &&
#endif
           other.lifecycle_   == lifecycle_
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
        && other.datarepresentation_ == datarepresentation_
        && other.typeconsistencyenforcement_ == typeconsistencyenforcement_
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
           ;
}

DataWriterQosDelegate&
DataWriterQosDelegate::operator =(const org::eclipse::cyclonedds::topic::qos::TopicQosDelegate& tqos)
{
    if (tqos.present() & DDSI_QP_DURABILITY)
      policy(tqos.policy<dds::core::policy::Durability>());
    if (tqos.present() & DDSI_QP_DEADLINE)
      policy(tqos.policy<dds::core::policy::Deadline>());
    if (tqos.present() & DDSI_QP_LATENCY_BUDGET)
      policy(tqos.policy<dds::core::policy::LatencyBudget>());
    if (tqos.present() & DDSI_QP_LIVELINESS)
      policy(tqos.policy<dds::core::policy::Liveliness>());
    if (tqos.present() & DDSI_QP_RELIABILITY)
      policy(tqos.policy<dds::core::policy::Reliability>());
    if (tqos.present() & DDSI_QP_DESTINATION_ORDER)
      policy(tqos.policy<dds::core::policy::DestinationOrder>());
    if (tqos.present() & DDSI_QP_HISTORY)
      policy(tqos.policy<dds::core::policy::History>());
    if (tqos.present() & DDSI_QP_RESOURCE_LIMITS)
      policy(tqos.policy<dds::core::policy::ResourceLimits>());
    if (tqos.present() & DDSI_QP_TRANSPORT_PRIORITY)
      policy(tqos.policy<dds::core::policy::TransportPriority>());
    if (tqos.present() & DDSI_QP_LIFESPAN)
      policy(tqos.policy<dds::core::policy::Lifespan>());
    if (tqos.present() & DDSI_QP_OWNERSHIP)
      policy(tqos.policy<dds::core::policy::Ownership>());
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    if (tqos.present() & DDSI_QP_DATA_REPRESENTATION)
      policy(tqos.policy<dds::core::policy::DataRepresentation>());
    if (tqos.present() & DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT)
      policy(tqos.policy<dds::core::policy::TypeConsistencyEnforcement>());
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    return *this;
}

template<> dds::core::policy::UserData&
DataWriterQosDelegate::policy<dds::core::policy::UserData>()
{
    present_ |= DDSI_QP_USER_DATA;
    return user_data_;
}

template<> dds::core::policy::Durability&
DataWriterQosDelegate::policy<dds::core::policy::Durability>()
{
    present_ |= DDSI_QP_DURABILITY;
    return durability_;
}

#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
template<> dds::core::policy::DurabilityService&
DataWriterQosDelegate::policy<dds::core::policy::DurabilityService>()
{
    present_ |= DDSI_QP_DURABILITY_SERVICE;
    return durability_service_;
}
#endif  // OMG_DDS_PERSISTENCE_SUPPORT

template<> dds::core::policy::Deadline&
DataWriterQosDelegate::policy<dds::core::policy::Deadline>()
{
    present_ |= DDSI_QP_DEADLINE;
    return deadline_;
}

template<> dds::core::policy::LatencyBudget&
DataWriterQosDelegate::policy<dds::core::policy::LatencyBudget>()
{
    present_ |= DDSI_QP_LATENCY_BUDGET;
    return budget_;
}

template<> dds::core::policy::Liveliness&
DataWriterQosDelegate::policy<dds::core::policy::Liveliness>()
{
    present_ |= DDSI_QP_LIVELINESS;
    return liveliness_;
}

template<> dds::core::policy::Reliability&
DataWriterQosDelegate::policy<dds::core::policy::Reliability>()
{
    present_ |= DDSI_QP_RELIABILITY;
    return reliability_;
}

template<> dds::core::policy::DestinationOrder&
DataWriterQosDelegate::policy<dds::core::policy::DestinationOrder>()
{
    present_ |= DDSI_QP_DESTINATION_ORDER;
    return order_;
}

template<> dds::core::policy::History&
DataWriterQosDelegate::policy<dds::core::policy::History>()
{
    present_ |= DDSI_QP_HISTORY;
    return history_;
}

template<> dds::core::policy::ResourceLimits&
DataWriterQosDelegate::policy<dds::core::policy::ResourceLimits>()
{
    present_ |= DDSI_QP_RESOURCE_LIMITS;
    return resources_;
}

template<> dds::core::policy::TransportPriority&
DataWriterQosDelegate::policy<dds::core::policy::TransportPriority>()
{
    present_ |= DDSI_QP_TRANSPORT_PRIORITY;
    return priority_;
}

template<> dds::core::policy::Lifespan&
DataWriterQosDelegate::policy<dds::core::policy::Lifespan>()
{
    present_ |= DDSI_QP_LIFESPAN;
    return lifespan_;
}

template<> dds::core::policy::Ownership&
DataWriterQosDelegate::policy<dds::core::policy::Ownership>()
{
    present_ |= DDSI_QP_OWNERSHIP;
    return ownership_;
}

#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
template<> dds::core::policy::OwnershipStrength&
DataWriterQosDelegate::policy<dds::core::policy::OwnershipStrength>()
{
    present_ |= DDSI_QP_OWNERSHIP_STRENGTH;
    return strength_;
}
#endif  // OMG_DDS_OWNERSHIP_SUPPORT

template<> dds::core::policy::WriterDataLifecycle&
DataWriterQosDelegate::policy<dds::core::policy::WriterDataLifecycle>()
{
    present_ |= DDSI_QP_ADLINK_WRITER_DATA_LIFECYCLE;
    return lifecycle_;
}

#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
template<> dds::core::policy::DataRepresentation&
DataWriterQosDelegate::policy<dds::core::policy::DataRepresentation>()
{
    present_ |= DDSI_QP_DATA_REPRESENTATION;
    return datarepresentation_;
}

template<> dds::core::policy::TypeConsistencyEnforcement&
DataWriterQosDelegate::policy<dds::core::policy::TypeConsistencyEnforcement>()
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
