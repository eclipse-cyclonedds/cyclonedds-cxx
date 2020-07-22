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
#include <org/eclipse/cyclonedds/sub/qos/DataReaderQosDelegate.hpp>

#include <cassert>

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
}

DataReaderQosDelegate::DataReaderQosDelegate(
    const DataReaderQosDelegate& other)
    : user_data_(other.user_data_),
      durability_(other.durability_),
      deadline_(other.deadline_),
      budget_(other.budget_),
      liveliness_(other.liveliness_),
      reliability_(other.reliability_),
      order_(other.order_),
      history_(other.history_),
      resources_(other.resources_),
      ownership_(other.ownership_),
      tfilter_(other.tfilter_),
      lifecycle_(other.lifecycle_)
{
}

DataReaderQosDelegate::DataReaderQosDelegate(
    const org::eclipse::cyclonedds::topic::qos::TopicQosDelegate& tqos)
{
    *this = tqos;
}

DataReaderQosDelegate::~DataReaderQosDelegate()
{
}

void
DataReaderQosDelegate::policy(const dds::core::policy::UserData& user_data)
{
    user_data.delegate().check();
    user_data_ = user_data;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::Durability& durability)
{
    durability.delegate().check();
    durability_ = durability;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::Deadline& deadline)
{
    deadline.delegate().check();
    deadline_ = deadline;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::LatencyBudget&  budget)
{
    budget.delegate().check();
    budget_ = budget;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::Liveliness& liveliness)
{
    liveliness.delegate().check();
    liveliness_ = liveliness;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::Reliability& reliability)
{
    reliability.delegate().check();
    reliability_ = reliability;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::DestinationOrder& order)
{
    order.delegate().check();
    order_ = order;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::History& history)
{
    history.delegate().check();
    history_ = history;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::ResourceLimits& resources)
{
    resources.delegate().check();
    resources_ = resources;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::Ownership& ownership)
{
    ownership.delegate().check();
    ownership_ = ownership;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::TimeBasedFilter& tfilter)
{
    tfilter.delegate().check();
    tfilter_ = tfilter;
}

void
DataReaderQosDelegate::policy(const dds::core::policy::ReaderDataLifecycle& lifecycle)
{
    lifecycle.delegate().check();
    lifecycle_ = lifecycle;
}

dds_qos_t*
DataReaderQosDelegate::ddsc_qos() const
{
    dds_qos_t* qos = dds_create_qos();
    deadline_    .delegate().set_c_policy(qos);
    durability_  .delegate().set_c_policy(qos);
    history_     .delegate().set_c_policy(qos);
    budget_      .delegate().set_c_policy(qos);
    lifecycle_   .delegate().set_c_policy(qos);
    liveliness_  .delegate().set_c_policy(qos);
    order_       .delegate().set_c_policy(qos);
    ownership_   .delegate().set_c_policy(qos);
    tfilter_     .delegate().set_c_policy(qos);
    reliability_ .delegate().set_c_policy(qos);
    resources_   .delegate().set_c_policy(qos);
    user_data_   .delegate().set_c_policy(qos);
    return qos;
}

void
DataReaderQosDelegate::ddsc_qos(const dds_qos_t* qos)
{
    assert(qos);
    deadline_    .delegate().set_iso_policy(qos);
    durability_  .delegate().set_iso_policy(qos);
    history_     .delegate().set_iso_policy(qos);
    budget_      .delegate().set_iso_policy(qos);
    lifecycle_   .delegate().set_iso_policy(qos);
    liveliness_  .delegate().set_iso_policy(qos);
    order_       .delegate().set_iso_policy(qos);
    ownership_   .delegate().set_iso_policy(qos);
    tfilter_     .delegate().set_iso_policy(qos);
    reliability_ .delegate().set_iso_policy(qos);
    resources_   .delegate().set_iso_policy(qos);
    user_data_   .delegate().set_iso_policy(qos);
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
    return other.user_data_   == user_data_   &&
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
           other.lifecycle_   == lifecycle_;
}

DataReaderQosDelegate&
DataReaderQosDelegate::operator =(const DataReaderQosDelegate& other)
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
    ownership_   = other.ownership_;
    tfilter_     = other.tfilter_;
    lifecycle_   = other.lifecycle_;
    return *this;
}

DataReaderQosDelegate&
DataReaderQosDelegate::operator =(const org::eclipse::cyclonedds::topic::qos::TopicQosDelegate& tqos)
{
    durability_  = tqos.policy<dds::core::policy::Durability>();
    deadline_    = tqos.policy<dds::core::policy::Deadline>();
    budget_      = tqos.policy<dds::core::policy::LatencyBudget>();
    liveliness_  = tqos.policy<dds::core::policy::Liveliness>();
    reliability_ = tqos.policy<dds::core::policy::Reliability>();
    order_       = tqos.policy<dds::core::policy::DestinationOrder>();
    history_     = tqos.policy<dds::core::policy::History>();
    resources_   = tqos.policy<dds::core::policy::ResourceLimits>();
    ownership_   = tqos.policy<dds::core::policy::Ownership>();
    return *this;
}

}
}
}
}
}
