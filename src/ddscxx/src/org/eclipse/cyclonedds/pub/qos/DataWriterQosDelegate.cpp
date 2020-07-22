/*
 * Copyright(c) 2006 to 2020 ADLINK Technology Limited and others
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

#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/pub/qos/DataWriterQosDelegate.hpp>

#include <cassert>

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
    reliability_.kind(dds::core::policy::ReliabilityKind::RELIABLE);
}

DataWriterQosDelegate::DataWriterQosDelegate(
    const DataWriterQosDelegate& other)
    : user_data_(other.user_data_),
      durability_(other.durability_),
      deadline_(other.deadline_),
      budget_(other.budget_),
      liveliness_(other.liveliness_),
      reliability_(other.reliability_),
      order_(other.order_),
      history_(other.history_),
      resources_(other.resources_),
      priority_(other.priority_),
      lifespan_(other.lifespan_),
      ownership_(other.ownership_),
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
      strength_(other.strength_),
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
      lifecycle_(other.lifecycle_)
{
}

DataWriterQosDelegate::DataWriterQosDelegate(
    const org::eclipse::cyclonedds::topic::qos::TopicQosDelegate& tqos)
{
    reliability_.kind(dds::core::policy::ReliabilityKind::RELIABLE);

    *this = tqos;
}

DataWriterQosDelegate::~DataWriterQosDelegate()
{
}

void
DataWriterQosDelegate::policy(const dds::core::policy::UserData& user_data)
{
    user_data.delegate().check();
    user_data_ = user_data;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Durability& durability)
{
    durability.delegate().check();
    durability_ = durability;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Deadline& deadline)
{
    deadline.delegate().check();
    deadline_ = deadline;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::LatencyBudget&  budget)
{
    budget.delegate().check();
    budget_ = budget;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Liveliness& liveliness)
{
    liveliness.delegate().check();
    liveliness_ = liveliness;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Reliability& reliability)
{
    reliability.delegate().check();
    reliability_ = reliability;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::DestinationOrder& order)
{
    order.delegate().check();
    order_ = order;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::History& history)
{
    history.delegate().check();
    history_ = history;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::ResourceLimits& resources)
{
    resources.delegate().check();
    resources_ = resources;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::TransportPriority& priority)
{
    priority.delegate().check();
    priority_ = priority;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Lifespan& lifespan)
{
    lifespan.delegate().check();
    lifespan_ = lifespan;
}

void
DataWriterQosDelegate::policy(const dds::core::policy::Ownership& ownership)
{
    ownership.delegate().check();
    ownership_ = ownership;
}

#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
void
DataWriterQosDelegate::policy(const dds::core::policy::OwnershipStrength& strength)
{
    strength.delegate().check();
    strength_ = strength;
}
#endif  // OMG_DDS_OWNERSHIP_SUPPORT

void
DataWriterQosDelegate::policy(const dds::core::policy::WriterDataLifecycle& lifecycle)
{
    lifecycle.delegate().check();
    lifecycle_ = lifecycle;
}

dds_qos_t*
DataWriterQosDelegate::ddsc_qos() const
{
    dds_qos_t* qos = dds_create_qos();
    user_data_   .delegate().set_c_policy(qos);
    durability_  .delegate().set_c_policy(qos);
    deadline_    .delegate().set_c_policy(qos);
    budget_      .delegate().set_c_policy(qos);
    liveliness_  .delegate().set_c_policy(qos);
    reliability_ .delegate().set_c_policy(qos);
    order_       .delegate().set_c_policy(qos);
    history_     .delegate().set_c_policy(qos);
    resources_   .delegate().set_c_policy(qos);
    priority_    .delegate().set_c_policy(qos);
    lifespan_    .delegate().set_c_policy(qos);
    ownership_   .delegate().set_c_policy(qos);
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
    strength_    .delegate().set_c_policy(qos);
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
    lifecycle_.delegate().set_c_policy(qos);
    return qos;
}

void
DataWriterQosDelegate::ddsc_qos(const dds_qos_t* qos)
{
    assert(qos);
    user_data_   .delegate().set_iso_policy(qos);
    durability_  .delegate().set_iso_policy(qos);
    deadline_    .delegate().set_iso_policy(qos);
    budget_      .delegate().set_iso_policy(qos);
    liveliness_  .delegate().set_iso_policy(qos);
    reliability_ .delegate().set_iso_policy(qos);
    order_       .delegate().set_iso_policy(qos);
    history_     .delegate().set_iso_policy(qos);
    resources_   .delegate().set_iso_policy(qos);
    priority_    .delegate().set_iso_policy(qos);
    lifespan_    .delegate().set_iso_policy(qos);
    ownership_   .delegate().set_iso_policy(qos);
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
    strength_    .delegate().set_iso_policy(qos);
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
    lifecycle_   .delegate().set_iso_policy(qos);
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
    return other.user_data_   == user_data_   &&
           other.durability_  == durability_  &&
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
           other.lifecycle_   == lifecycle_;
}

DataWriterQosDelegate&
DataWriterQosDelegate::operator =(const DataWriterQosDelegate& other)
{
    user_data_   = other.user_data_;
    durability_  = other.durability_;
    deadline_    = other.deadline_;
    budget_      = other.budget_;
    liveliness_  = other.liveliness_;
    reliability_ = other.reliability_;
    order_       = other.order_;
    history_     = other.history_;
    resources_   = other.resources_;
    priority_    = other.priority_;
    lifespan_    = other.lifespan_;
    ownership_   = other.ownership_;
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
    strength_    = other.strength_;
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
    lifecycle_   = other.lifecycle_;
    return *this;
}

DataWriterQosDelegate&
DataWriterQosDelegate::operator =(const org::eclipse::cyclonedds::topic::qos::TopicQosDelegate& tqos)
{
    durability_  = tqos.policy<dds::core::policy::Durability>();
    deadline_    = tqos.policy<dds::core::policy::Deadline>();
    budget_      = tqos.policy<dds::core::policy::LatencyBudget>();
    liveliness_  = tqos.policy<dds::core::policy::Liveliness>();
    reliability_ = tqos.policy<dds::core::policy::Reliability>();
    order_       = tqos.policy<dds::core::policy::DestinationOrder>();
    history_     = tqos.policy<dds::core::policy::History>();
    resources_   = tqos.policy<dds::core::policy::ResourceLimits>();
    priority_    = tqos.policy<dds::core::policy::TransportPriority>();
    lifespan_    = tqos.policy<dds::core::policy::Lifespan>();
    ownership_   = tqos.policy<dds::core::policy::Ownership>();
    return *this;
}

}
}
}
}
}
