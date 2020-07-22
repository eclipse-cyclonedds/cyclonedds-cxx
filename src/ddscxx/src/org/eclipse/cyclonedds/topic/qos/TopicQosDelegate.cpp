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
#include <org/eclipse/cyclonedds/topic/qos/TopicQosDelegate.hpp>

#include <cassert>

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
}

TopicQosDelegate::TopicQosDelegate(
    const TopicQosDelegate& other)
    : topic_data_(other.topic_data_),
      durability_(other.durability_),
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
      durability_service_(other.durability_service_),
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
      deadline_(other.deadline_),
      budget_(other.budget_),
      liveliness_(other.liveliness_),
      reliability_(other.reliability_),
      order_(other.order_),
      history_(other.history_),
      resources_(other.resources_),
      priority_(other.priority_),
      lifespan_(other.lifespan_),
      ownership_(other.ownership_)
{
}

TopicQosDelegate::~TopicQosDelegate()
{
}

void
TopicQosDelegate::policy(const dds::core::policy::TopicData& topic_data)
{
    topic_data.delegate().check();
    topic_data_ = topic_data;
}

void
TopicQosDelegate::policy(const dds::core::policy::Durability& durability)
{
    durability.delegate().check();
    durability_ = durability;
}

#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
void
TopicQosDelegate::policy(const dds::core::policy::DurabilityService& durability_service)
{
    durability_service.delegate().check();
    durability_service_ = durability_service;
}
#endif  // OMG_DDS_PERSISTENCE_SUPPORT

void
TopicQosDelegate::policy(const dds::core::policy::Deadline& deadline)
{
    deadline.delegate().check();
    deadline_ = deadline;
}

void
TopicQosDelegate::policy(const dds::core::policy::LatencyBudget&  budget)
{
    budget.delegate().check();
    budget_ = budget;
}

void
TopicQosDelegate::policy(const dds::core::policy::Liveliness& liveliness)
{
    liveliness.delegate().check();
    liveliness_ = liveliness;
}

void
TopicQosDelegate::policy(const dds::core::policy::Reliability& reliability)
{
    reliability.delegate().check();
    reliability_ = reliability;
}

void
TopicQosDelegate::policy(const dds::core::policy::DestinationOrder& order)
{
    order.delegate().check();
    order_ = order;
}

void
TopicQosDelegate::policy(const dds::core::policy::History& history)
{
    history.delegate().check();
    history_ = history;
}

void
TopicQosDelegate::policy(const dds::core::policy::ResourceLimits& resources)
{
    resources.delegate().check();
    resources_ = resources;
}

void
TopicQosDelegate::policy(const dds::core::policy::TransportPriority& priority)
{
    priority.delegate().check();
    priority_ = priority;
}

void
TopicQosDelegate::policy(const dds::core::policy::Lifespan& lifespan)
{
    lifespan.delegate().check();
    lifespan_ = lifespan;
}

void
TopicQosDelegate::policy(const dds::core::policy::Ownership& ownership)
{
    ownership.delegate().check();
    ownership_ = ownership;
}

dds_qos_t*
TopicQosDelegate::ddsc_qos() const
{
    dds_qos_t* qos = dds_create_qos();
    if (!qos) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_OUT_OF_RESOURCES_ERROR, "Could not create internal QoS.");
    }
    topic_data_  .delegate().set_c_policy(qos);
    durability_  .delegate().set_c_policy(qos);
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    durability_service_.delegate().set_c_policy(qos);
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
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
    return qos;
}

void
TopicQosDelegate::ddsc_qos(const dds_qos_t* qos)
{
    assert(qos);
    topic_data_  .delegate().set_iso_policy(qos);
    durability_  .delegate().set_iso_policy(qos);
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    durability_service_.delegate().set_iso_policy(qos);
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
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
    return other.topic_data_  == topic_data_  &&
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
           other.ownership_   == ownership_;
}

TopicQosDelegate&
TopicQosDelegate::operator =(const TopicQosDelegate& other)
{
    topic_data_  = other.topic_data_;
    durability_  = other.durability_;
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    durability_service_ = other.durability_service_;
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
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
    return *this;
}

}
}
}
}
}
