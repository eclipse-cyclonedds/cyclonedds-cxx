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
#include <org/eclipse/cyclonedds/sub/qos/DataReaderQosDelegate.hpp>

#include <cassert>

#include "dds/ddsi/ddsi_plist.h"

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace sub
{
namespace qos
{

DataReaderQosDelegate::DataReaderQosDelegate()
{
    ddsc_qos(&ddsi_default_qos_reader);
    present() &= ~DDSI_QP_DATA_REPRESENTATION;
    check();
}

DataReaderQosDelegate::DataReaderQosDelegate(
    const org::eclipse::cyclonedds::topic::qos::TopicQosDelegate& tqos): DataReaderQosDelegate()
{
    *this = tqos;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::UserData& user_data)
{
    user_data.delegate().check();
    present_ |= DDSI_QP_USER_DATA;
    user_data_ = user_data;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::Durability& durability)
{
    durability.delegate().check();
    present_ |= DDSI_QP_DURABILITY;
    durability_ = durability;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::Deadline& deadline)
{
    deadline.delegate().check();
    present_ |= DDSI_QP_DEADLINE;
    deadline_ = deadline;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::LatencyBudget&  budget)
{
    budget.delegate().check();
    present_ |= DDSI_QP_LATENCY_BUDGET;
    budget_ = budget;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::Liveliness& liveliness)
{
    liveliness.delegate().check();
    present_ |= DDSI_QP_LIVELINESS;
    liveliness_ = liveliness;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::Reliability& reliability)
{
    reliability.delegate().check();
    present_ |= DDSI_QP_RELIABILITY;
    reliability_ = reliability;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::DestinationOrder& order)
{
    order.delegate().check();
    present_ |= DDSI_QP_DESTINATION_ORDER;
    order_ = order;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::History& history)
{
    history.delegate().check();
    present_ |= DDSI_QP_HISTORY;
    history_ = history;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::ResourceLimits& resources)
{
    resources.delegate().check();
    present_ |= DDSI_QP_RESOURCE_LIMITS;
    resources_ = resources;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::Ownership& ownership)
{
    ownership.delegate().check();
    present_ |= DDSI_QP_OWNERSHIP;
    ownership_ = ownership;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::TimeBasedFilter& tfilter)
{
    tfilter.delegate().check();
    present_ |= DDSI_QP_TIME_BASED_FILTER;
    tfilter_ = tfilter;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::ReaderDataLifecycle& lifecycle)
{
    lifecycle.delegate().check();
    present_ |= DDSI_QP_ADLINK_READER_DATA_LIFECYCLE;
    lifecycle_ = lifecycle;
}

#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
void
DataReaderQosDelegate::policy(const dds::core::policy::DataRepresentation& datarepresentation)
{
    datarepresentation.delegate().check();
    present_ |= DDSI_QP_DATA_REPRESENTATION;
    datarepresentation_ = datarepresentation;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::TypeConsistencyEnforcement& typeconsistencyenforcement)
{
    typeconsistencyenforcement.delegate().check();
    present_ |= DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT;
    typeconsistencyenforcement_ = typeconsistencyenforcement;
}
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

dds_qos_t*
DataReaderQosDelegate::ddsc_qos() const
{
    dds_qos_t* qos = dds_create_qos();
    if (!qos) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_OUT_OF_RESOURCES_ERROR, "Could not create internal QoS.");
    }
    if (present_ & DDSI_QP_DEADLINE)
        deadline_    .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_DURABILITY)
        durability_  .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_HISTORY)
        history_     .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_LATENCY_BUDGET)
        budget_      .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_ADLINK_READER_DATA_LIFECYCLE)
        lifecycle_   .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_LIVELINESS)
        liveliness_  .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_DESTINATION_ORDER)
        order_       .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_OWNERSHIP)
        ownership_   .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_TIME_BASED_FILTER)
        tfilter_     .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_RELIABILITY)
        reliability_ .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_RESOURCE_LIMITS)
        resources_   .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_USER_DATA)
        user_data_   .delegate().set_c_policy(qos);
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    if (present_ & DDSI_QP_DATA_REPRESENTATION)
        datarepresentation_.delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT)
        typeconsistencyenforcement_.delegate().set_c_policy(qos);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    return qos;
}

void
DataReaderQosDelegate::ddsc_qos(const dds_qos_t* qos)
{
    assert(qos);
    present_ = qos->present;
    if (present_ & DDSI_QP_DEADLINE)
        deadline_    .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_DURABILITY)
        durability_  .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_HISTORY)
        history_     .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_LATENCY_BUDGET)
        budget_      .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_ADLINK_READER_DATA_LIFECYCLE)
        lifecycle_   .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_LIVELINESS)
        liveliness_  .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_DESTINATION_ORDER)
        order_       .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_OWNERSHIP)
        ownership_   .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_TIME_BASED_FILTER)
        tfilter_     .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_RELIABILITY)
        reliability_ .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_RESOURCE_LIMITS)
        resources_   .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_USER_DATA)
        user_data_   .delegate().set_iso_policy(qos);
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    if (present_ & DDSI_QP_DATA_REPRESENTATION)
        datarepresentation_.delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_TYPE_CONSISTENCY_ENFORCEMENT)
        typeconsistencyenforcement_.delegate().set_iso_policy(qos);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
}

void
DataReaderQosDelegate::named_qos(const struct _DDS_NamedDataReaderQos &qos)
{
    (void)qos;
#if 0
    /* We only need the QoS part of the named QoS. */
    const struct _DDS_DataReaderQos *q = &qos.datareader_qos;
    /* The idl policies are aligned the same as the ddsc/builtin representation.
     * So, cast and use the ddsc policy translations (or builtin when available).
     * Exception is the keys policy... */
    deadline_    .delegate().v_policy((v_deadlinePolicy&)       (q->deadline)             );
    durability_  .delegate().v_policy((v_durabilityPolicy&)     (q->durability)           );
    history_     .delegate().v_policy((v_historyPolicy&)        (q->history)              );
    budget_      .delegate().v_policy((v_latencyPolicy&)        (q->latency_budget)       );
    lifecycle_   .delegate().v_policy((v_readerLifecyclePolicy&)(q->reader_data_lifecycle));
    liveliness_  .delegate().v_policy((v_livelinessPolicy&)     (q->liveliness)           );
    order_       .delegate().v_policy((v_orderbyPolicy&)        (q->destination_order)    );
    ownership_   .delegate().v_policy((v_ownershipPolicy&)      (q->ownership)            );
    tfilter_     .delegate().v_policy((v_pacingPolicy&)         (q->time_based_filter)    );
    reliability_ .delegate().v_policy((v_reliabilityPolicy&)    (q->reliability)          );
    resources_   .delegate().v_policy((v_resourcePolicy&)       (q->resource_limits)      );
    user_data_   .delegate().v_policy((v_builtinUserDataPolicy&)(q->user_data)            );
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    datarepresentation_.delegate().v_policy((v_dataRepresentationPolicy&)(q->datarepresentation));
    typeconsistencyenforcement_.delegate().v_policy((v_typeConsistencyEnforcementPolicy&)(q->typeconsistencyenforcement));
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
#endif
}

void
DataReaderQosDelegate::check() const
{
    /* Policies are checked when set.
     * But consistency between policies needs to be checked. */
    history_.delegate().check_against(resources_.delegate());
    deadline_.delegate().check_against(tfilter_.delegate());
}

bool
DataReaderQosDelegate::operator==(const DataReaderQosDelegate& other) const
{
    return other.present_     == present_     &&
           other.user_data_   == user_data_   &&
           other.durability_  == durability_  &&
           other.deadline_    == deadline_    &&
           other.budget_      == budget_      &&
           other.liveliness_  == liveliness_  &&
           other.reliability_ == reliability_ &&
           other.order_       == order_       &&
           other.history_     == history_     &&
           other.resources_   == resources_   &&
           other.ownership_   == ownership_   &&
           other.tfilter_     == tfilter_     &&
           other.lifecycle_   == lifecycle_
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
        && other.datarepresentation_ == datarepresentation_
        && other.typeconsistencyenforcement_ == typeconsistencyenforcement_
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
           ;
}

DataReaderQosDelegate&
DataReaderQosDelegate::operator =(const org::eclipse::cyclonedds::topic::qos::TopicQosDelegate& tqos)
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

template<>
dds::core::policy::Durability&
DataReaderQosDelegate::policy<dds::core::policy::Durability>()
{
    present_ |= DDSI_QP_DURABILITY;
    return durability_;
}

template<>
dds::core::policy::UserData&
DataReaderQosDelegate::policy<dds::core::policy::UserData>()
{
    present_ |= DDSI_QP_USER_DATA;
    return user_data_;
}

template<>
dds::core::policy::Deadline&
DataReaderQosDelegate::policy<dds::core::policy::Deadline>()
{
    present_ |= DDSI_QP_DEADLINE;
    return deadline_;
}

template<>
dds::core::policy::LatencyBudget&
DataReaderQosDelegate::policy<dds::core::policy::LatencyBudget>()
{
    present_ |= DDSI_QP_LATENCY_BUDGET;
    return budget_;
}

template<>
dds::core::policy::Liveliness&
DataReaderQosDelegate::policy<dds::core::policy::Liveliness>()
{
    present_ |= DDSI_QP_LIVELINESS;
    return liveliness_;
}

template<>
dds::core::policy::Reliability&
DataReaderQosDelegate::policy<dds::core::policy::Reliability>()
{
    present_ |= DDSI_QP_RELIABILITY;
    return reliability_;
}

template<>
dds::core::policy::DestinationOrder&
DataReaderQosDelegate::policy<dds::core::policy::DestinationOrder>()
{
    present_ |= DDSI_QP_DESTINATION_ORDER;
    return order_;
}

template<>
dds::core::policy::History&
DataReaderQosDelegate::policy<dds::core::policy::History>()
{
    present_ |= DDSI_QP_HISTORY;
    return history_;
}

template<>
dds::core::policy::ResourceLimits&
DataReaderQosDelegate::policy<dds::core::policy::ResourceLimits>()
{
    present_ |= DDSI_QP_RESOURCE_LIMITS;
    return resources_;
}

template<>
dds::core::policy::Ownership&
DataReaderQosDelegate::policy<dds::core::policy::Ownership>()
{
    present_ |= DDSI_QP_OWNERSHIP;
    return ownership_;
}

template<>
dds::core::policy::TimeBasedFilter&
DataReaderQosDelegate::policy<dds::core::policy::TimeBasedFilter>()
{
    present_ |= DDSI_QP_TIME_BASED_FILTER;
    return tfilter_;
}

template<>
dds::core::policy::ReaderDataLifecycle&
DataReaderQosDelegate::policy<dds::core::policy::ReaderDataLifecycle>()
{
    present_ |= DDSI_QP_ADLINK_READER_DATA_LIFECYCLE;
    return lifecycle_;
}

#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
template<> dds::core::policy::DataRepresentation&
DataReaderQosDelegate::policy<dds::core::policy::DataRepresentation>()
{
    present_ |= DDSI_QP_DATA_REPRESENTATION;
    return datarepresentation_;
}

template<> dds::core::policy::TypeConsistencyEnforcement&
DataReaderQosDelegate::policy<dds::core::policy::TypeConsistencyEnforcement>()
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
