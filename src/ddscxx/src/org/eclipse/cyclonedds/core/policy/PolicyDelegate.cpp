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

#include <dds/core/policy/CorePolicy.hpp>

#include <org/eclipse/cyclonedds/core/policy/PolicyDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/core/MiscUtils.hpp>
#include "dds/dds.h"
#include "dds/ddsc/dds_public_qos.h"

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace core
{
namespace policy
{

//==============================================================================

DeadlineDelegate::DeadlineDelegate(const DeadlineDelegate& other)
    : period_(other.period_)
{
    this->check();
}

DeadlineDelegate::DeadlineDelegate(const dds::core::Duration& d)
    : period_(d)
{
}

void DeadlineDelegate::period(const dds::core::Duration& d)
{
    period_ = d;
}

dds::core::Duration DeadlineDelegate::period() const
{
    return period_;
}

bool DeadlineDelegate::operator ==(const DeadlineDelegate& other) const
{
    return other.period() == period_;
}

void DeadlineDelegate::check() const
{
    /* A Duration object is always valid:
     *      The period duration is always valid:
     *          Nothing to check. */
}

void DeadlineDelegate::check_against(const org::eclipse::cyclonedds::core::policy::TimeBasedFilterDelegate& filter) const
{
    filter.check_against(*this);
}

void DeadlineDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_duration_t deadline;
    if (dds_qget_deadline(qos, &deadline)) {
        period_ = org::eclipse::cyclonedds::core::convertDuration(deadline);
    }
}

void DeadlineDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_qset_deadline(qos, org::eclipse::cyclonedds::core::convertDuration(period_));
}


//==============================================================================

DestinationOrderDelegate::DestinationOrderDelegate(const DestinationOrderDelegate& other)
    : kind_(other.kind_)
{
}

DestinationOrderDelegate::DestinationOrderDelegate(dds::core::policy::DestinationOrderKind::Type kind)
    : kind_(kind)
{
    this->check();
}

void DestinationOrderDelegate::kind(dds::core::policy::DestinationOrderKind::Type kind)
{
    kind_ = kind;
}

dds::core::policy::DestinationOrderKind::Type DestinationOrderDelegate::kind() const
{
    return kind_;
}

bool DestinationOrderDelegate::operator ==(const DestinationOrderDelegate& other) const
{
    return other.kind() == kind_;
}

void DestinationOrderDelegate::check() const
{
    /* The kind correctness is enforced by the compiler: nothing to check. */
}

void DestinationOrderDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_destination_order_kind_t kind;
    if (dds_qget_destination_order(qos, &kind)) {
        switch (kind)
        {
            case DDS_DESTINATIONORDER_BY_RECEPTION_TIMESTAMP:
                kind_ = dds::core::policy::DestinationOrderKind::BY_RECEPTION_TIMESTAMP;
                break;
            case DDS_DESTINATIONORDER_BY_SOURCE_TIMESTAMP:
                kind_ = dds::core::policy::DestinationOrderKind::BY_SOURCE_TIMESTAMP;
                break;
            default:
                assert(0);
                break;
        }
    }
}

void DestinationOrderDelegate::set_c_policy(dds_qos_t* qos) const
{
    switch(kind_)
    {
    case dds::core::policy::DestinationOrderKind::BY_RECEPTION_TIMESTAMP:
        dds_qset_destination_order(qos, DDS_DESTINATIONORDER_BY_RECEPTION_TIMESTAMP);
        break;
    case dds::core::policy::DestinationOrderKind::BY_SOURCE_TIMESTAMP:
        dds_qset_destination_order(qos, DDS_DESTINATIONORDER_BY_SOURCE_TIMESTAMP);
        break;
    default:
        assert(0);
        break;
    }
}


//==============================================================================

DurabilityDelegate::DurabilityDelegate(const DurabilityDelegate& other)
    : kind_(other.kind_)
{
}

DurabilityDelegate::DurabilityDelegate(dds::core::policy::DurabilityKind::Type kind)
    : kind_(kind)
{
    this->check();
}

void DurabilityDelegate::kind(dds::core::policy::DurabilityKind::Type kind)
{
    kind_ = kind;
}

dds::core::policy::DurabilityKind::Type DurabilityDelegate::kind() const
{
    return kind_;
}

bool DurabilityDelegate::operator ==(const DurabilityDelegate& other) const
{
    return other.kind() == kind_;
}

void DurabilityDelegate::check() const
{
    /* The kind correctness is enforced by the compiler: nothing to check. */
}

void DurabilityDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_durability_kind_t kind;
    if (dds_qget_durability(qos, &kind)) {
        switch (kind) {
            case DDS_DURABILITY_VOLATILE:
                kind_ = dds::core::policy::DurabilityKind::VOLATILE;
                break;
            case DDS_DURABILITY_TRANSIENT:
                kind_ = dds::core::policy::DurabilityKind::TRANSIENT;
                break;
            case DDS_DURABILITY_TRANSIENT_LOCAL:
                kind_ = dds::core::policy::DurabilityKind::TRANSIENT_LOCAL;
                break;
            case DDS_DURABILITY_PERSISTENT:
                kind_ = dds::core::policy::DurabilityKind::PERSISTENT;
                break;
            default:
                assert(0);
                break;
        }
    }
}

void DurabilityDelegate::set_c_policy(dds_qos_t* qos) const
{
    switch(kind_)
    {
    case dds::core::policy::DurabilityKind::VOLATILE:
        dds_qset_durability(qos, DDS_DURABILITY_VOLATILE);
        break;
    case dds::core::policy::DurabilityKind::TRANSIENT_LOCAL:
        dds_qset_durability(qos, DDS_DURABILITY_TRANSIENT_LOCAL);
        break;
    case dds::core::policy::DurabilityKind::TRANSIENT:
        dds_qset_durability(qos, DDS_DURABILITY_TRANSIENT);
        break;
    case dds::core::policy::DurabilityKind::PERSISTENT:
        dds_qset_durability(qos, DDS_DURABILITY_PERSISTENT);
        break;
    default:
        assert(0);
        break;
    }
}


//==============================================================================

#ifdef  OMG_DDS_PERSISTENCE_SUPPORT

DurabilityServiceDelegate::DurabilityServiceDelegate(const DurabilityServiceDelegate& other)
    : cleanup_delay_(other.cleanup_delay_),
      history_kind_(other.history_kind_),
      history_depth_(other.history_depth_),
      max_samples_(other.max_samples_),
      max_instances_(other.max_instances_),
      max_samples_per_instance_(other.max_samples_per_instance_)
{
}

DurabilityServiceDelegate::DurabilityServiceDelegate(const dds::core::Duration& service_cleanup_delay,
                                                     dds::core::policy::HistoryKind::Type history_kind,
                                                     int32_t history_depth,
                                                     int32_t max_samples,
                                                     int32_t max_instances,
                                                     int32_t max_samples_per_instance)
    : cleanup_delay_(service_cleanup_delay),
      history_kind_(history_kind),
      history_depth_(history_depth),
      max_samples_(max_samples),
      max_instances_(max_instances),
      max_samples_per_instance_(max_samples_per_instance)
{
    this->check();
}

void DurabilityServiceDelegate::service_cleanup_delay(const dds::core::Duration& d)
{
    cleanup_delay_ = d;
}

const dds::core::Duration DurabilityServiceDelegate::service_cleanup_delay() const
{
    return cleanup_delay_;
}

void DurabilityServiceDelegate::history_kind(dds::core::policy::HistoryKind::Type kind)
{
    history_kind_ = kind;
}

dds::core::policy::HistoryKind::Type DurabilityServiceDelegate::history_kind() const
{
    return history_kind_;
}

void DurabilityServiceDelegate::history_depth(int32_t depth)
{
    history_depth_ = depth;
}

int32_t DurabilityServiceDelegate::history_depth() const
{
    return history_depth_;
}

void DurabilityServiceDelegate::max_samples(int32_t max_samples)
{
    max_samples_ = max_samples;
}

int32_t DurabilityServiceDelegate::max_samples() const
{
    return max_samples_;
}

void DurabilityServiceDelegate::max_instances(int32_t max_instances)
{
    max_instances_ = max_instances;
}

int32_t DurabilityServiceDelegate::max_instances() const
{
    return max_instances_;
}

void DurabilityServiceDelegate::max_samples_per_instance(int32_t max_samples_per_instance)
{
    max_samples_per_instance_ = max_samples_per_instance;
}

int32_t DurabilityServiceDelegate::max_samples_per_instance() const
{
    return max_samples_per_instance_;
}

bool DurabilityServiceDelegate::operator ==(const DurabilityServiceDelegate& other) const
{
    return other.service_cleanup_delay() == cleanup_delay_ &&
           other.history_kind() == history_kind_ &&
           other.history_depth() == history_depth_ &&
           other.max_samples() == max_samples_ &&
           other.max_instances() == max_instances_ &&
           other.max_samples_per_instance() == max_samples_per_instance_;
}

void DurabilityServiceDelegate::check() const
{
    /* The kind correctness is enforced by the compiler. */
    /* A Duration object is always valid: the service_cleanup_delay is valid. */

    if ((max_samples_ < 0) && (max_samples_ != dds::core::LENGTH_UNLIMITED)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_INVALID_ARGUMENT_ERROR, "Invalid DurabilityService::max_samples (%ld) value.", max_samples_);
    }
    if ((max_instances_ < 0) && (max_instances_ != dds::core::LENGTH_UNLIMITED)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_INVALID_ARGUMENT_ERROR, "Invalid DurabilityService::max_instances (%ld) value.", max_instances_);
    }
    if ((max_samples_per_instance_ < 0) && (max_samples_per_instance_ != dds::core::LENGTH_UNLIMITED)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_INVALID_ARGUMENT_ERROR, "Invalid DurabilityService::max_samples_per_instance (%ld) value.", max_samples_per_instance_);
    }

    if ((history_kind_ == dds::core::policy::HistoryKind::KEEP_LAST) &&
        (history_depth_ <= 0)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_INCONSISTENT_POLICY_ERROR, "DurabilityService: history_depth (%ld) not consistent with KEEP_LAST", history_depth_);
    }

    if ((max_samples_per_instance_ != dds::core::LENGTH_UNLIMITED) &&
        (history_depth_ > max_samples_per_instance_)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_INCONSISTENT_POLICY_ERROR, "DurabilityService: history_depth (%ld) not consistent with max_samples_per_instance (%ld)", history_depth_, max_samples_per_instance_);
    }
}

void DurabilityServiceDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_duration_t service_cleanup_delay;
    dds_history_kind_t history_kind;

    if (dds_qget_durability_service(qos,
                                    &service_cleanup_delay,
                                    &history_kind,
                                    &history_depth_,
                                    &max_samples_,
                                    &max_instances_,
                                    &max_samples_per_instance_)) {
        cleanup_delay_ = org::eclipse::cyclonedds::core::convertDuration(service_cleanup_delay);
        switch (history_kind) {
            case DDS_HISTORY_KEEP_LAST:
                history_kind_ = dds::core::policy::HistoryKind::KEEP_LAST;
                break;
            case DDS_HISTORY_KEEP_ALL:
                history_kind_ = dds::core::policy::HistoryKind::KEEP_ALL;
                break;
            default:
                assert(0);
                break;
        }
    }
}

void DurabilityServiceDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_duration_t cleanup_delay = org::eclipse::cyclonedds::core::convertDuration(cleanup_delay_);
    switch(history_kind_)
    {
    case dds::core::policy::HistoryKind::KEEP_LAST:
        dds_qset_durability_service(
            qos, cleanup_delay, DDS_HISTORY_KEEP_LAST, history_depth_, max_samples_,
            max_instances_, max_samples_per_instance_);
        break;
    case dds::core::policy::HistoryKind::KEEP_ALL:
        dds_qset_durability_service(
            qos, cleanup_delay, DDS_HISTORY_KEEP_ALL, history_depth_, max_samples_,
            max_instances_, max_samples_per_instance_);
        break;
    default:
        assert(0);
        break;
    }
}

#endif  // OMG_DDS_PERSISTENCE_SUPPORT


//==============================================================================

EntityFactoryDelegate::EntityFactoryDelegate(const EntityFactoryDelegate& other)
    : auto_enable_(other.auto_enable_)
{
}

EntityFactoryDelegate::EntityFactoryDelegate(bool auto_enable)
    : auto_enable_(auto_enable)
{
    this->check();
}

void EntityFactoryDelegate::auto_enable(bool on)
{
    auto_enable_ = on;
}

bool EntityFactoryDelegate::auto_enable() const
{
    return auto_enable_;
}

bool EntityFactoryDelegate::operator ==(const EntityFactoryDelegate& other) const
{
    return other.auto_enable() == auto_enable_;
}

void EntityFactoryDelegate::check() const
{
    /* The auto_enable_ is just a boolean: nothing to check. */
}

void EntityFactoryDelegate::set_iso_policy(const dds_qos_t* qos)
{
    (void)qos;
    auto_enable_ = true;
}

void EntityFactoryDelegate::set_c_policy(dds_qos_t* qos) const
{
    (void)qos;
}


//==============================================================================

GroupDataDelegate::GroupDataDelegate()
    : value_()
{
}

GroupDataDelegate::GroupDataDelegate(const GroupDataDelegate& other)
    : value_(other.value_)
{
}

GroupDataDelegate::GroupDataDelegate(const dds::core::ByteSeq& seq)
    : value_(seq)
{
    this->check();
}

void GroupDataDelegate::value(const dds::core::ByteSeq& seq)
{
    value_ = seq;
}

const dds::core::ByteSeq& GroupDataDelegate::value() const
{
    return value_;
}

bool GroupDataDelegate::operator ==(const GroupDataDelegate& other) const
{
    return other.value() == value_;
}

void GroupDataDelegate::check() const
{
    /* The value_ is just a sequence: nothing to check. */
}

void GroupDataDelegate::set_iso_policy(const dds_qos_t* qos)
{
    void *value;
    size_t sz;
    if (dds_qget_groupdata(qos, &value, &sz)) {
        if (sz > 0) {
            org::eclipse::cyclonedds::core::convertByteSeq(value, static_cast<int32_t>(sz), value_);
        }
        dds_free(value);
    }
}

void GroupDataDelegate::set_c_policy(dds_qos_t* qos) const
{
    if(value_.size())
    {
        void* data = NULL;
        org::eclipse::cyclonedds::core::convertByteSeq(value_, data, static_cast<int32_t>(value_.size()));
        dds_qset_groupdata(qos, data, value_.size());
    }
}


//==============================================================================

HistoryDelegate::HistoryDelegate(const HistoryDelegate& other)
    :  kind_(other.kind_),
       depth_(other.depth_)
{
}

HistoryDelegate::HistoryDelegate(dds::core::policy::HistoryKind::Type kind, int32_t depth)
    :  kind_(kind),
       depth_(depth)
{
    this->check();
}

dds::core::policy::HistoryKind::Type HistoryDelegate::kind() const
{
    return kind_;
}

void HistoryDelegate::kind(dds::core::policy::HistoryKind::Type kind)
{
    kind_ = kind;
}

int32_t HistoryDelegate::depth() const
{
    return depth_;
}

void HistoryDelegate::depth(int32_t depth)
{
    depth_ = depth;
}

bool HistoryDelegate::operator ==(const HistoryDelegate& other) const
{
    return other.kind() == kind_ &&
           other.depth() == depth_;
}

void HistoryDelegate::check() const
{
    /* The kind correctness is enforced by the compiler. */

    if ((kind_ == dds::core::policy::HistoryKind::KEEP_LAST) &&
        (depth_ <= 0)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_INCONSISTENT_POLICY_ERROR, "History::depth (%ld) not consistent with KEEP_LAST", depth_);
    }
}

void HistoryDelegate::check_against(const org::eclipse::cyclonedds::core::policy::ResourceLimitsDelegate& limits) const
{
    limits.check_against(*this);
}

void HistoryDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_history_kind_t kind;

    if (dds_qget_history(qos, &kind, &depth_)) {
        switch (kind) {
            case DDS_HISTORY_KEEP_LAST:
                kind_= dds::core::policy::HistoryKind::KEEP_LAST;
                break;
            case DDS_HISTORY_KEEP_ALL:
                kind_ = dds::core::policy::HistoryKind::KEEP_ALL;
                break;
            default:
                assert(0);
                break;
        }
    }
}

void HistoryDelegate::set_c_policy(dds_qos_t* qos) const
{
    switch(kind_)
    {
    case dds::core::policy::HistoryKind::KEEP_LAST:
        dds_qset_history(qos, DDS_HISTORY_KEEP_LAST, depth_);
        break;
    case dds::core::policy::HistoryKind::KEEP_ALL:
        dds_qset_history(qos, DDS_HISTORY_KEEP_ALL, depth_);
        break;
    default:
        assert(0);
        break;
    }
}


//==============================================================================

LatencyBudgetDelegate::LatencyBudgetDelegate(const LatencyBudgetDelegate& other)
    : duration_(other.duration_)
{
}

LatencyBudgetDelegate::LatencyBudgetDelegate(const dds::core::Duration& d)
    : duration_(d)
{
    this->check();
}

void LatencyBudgetDelegate::duration(const dds::core::Duration& d)
{
    duration_ = d;
}

const dds::core::Duration LatencyBudgetDelegate::duration() const
{
    return duration_;
}

bool LatencyBudgetDelegate::operator ==(const LatencyBudgetDelegate& other) const
{
    return other.duration() == duration_;
}

void LatencyBudgetDelegate::check() const
{
    /* A Duration object is always valid:
     *      The duration_ is always valid:
     *          Nothing to check. */
}

void LatencyBudgetDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_duration_t duration;
    if (dds_qget_latency_budget(qos, &duration)) {
        duration_ = org::eclipse::cyclonedds::core::convertDuration(duration);
    }
}

void LatencyBudgetDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_qset_latency_budget(qos, org::eclipse::cyclonedds::core::convertDuration(duration_));
}


//==============================================================================

LifespanDelegate::LifespanDelegate(const LifespanDelegate& other)
    : duration_(other.duration_)
{
}

LifespanDelegate::LifespanDelegate(const dds::core::Duration& d)
    : duration_(d)
{
    this->check();
}

void LifespanDelegate::duration(const dds::core::Duration& d)
{
    duration_ = d;
}

const dds::core::Duration LifespanDelegate::duration() const
{
    return duration_;
}

bool LifespanDelegate::operator ==(const LifespanDelegate& other) const
{
    return other.duration() == duration_;
}

void LifespanDelegate::check() const
{
    /* A Duration object is always valid:
     *      The duration_ is always valid:
     *          Nothing to check. */
}

void LifespanDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_duration_t lifespan;
    if (dds_qget_lifespan(qos, &lifespan)) {
        duration_ = org::eclipse::cyclonedds::core::convertDuration(lifespan);
    }
}

void LifespanDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_qset_lifespan(qos, org::eclipse::cyclonedds::core::convertDuration(duration_));
}


//==============================================================================

LivelinessDelegate::LivelinessDelegate(const LivelinessDelegate& other)
    : kind_(other.kind_),
      lease_duration_(other.lease_duration_)
{
}

LivelinessDelegate::LivelinessDelegate(dds::core::policy::LivelinessKind::Type kind,
                                       dds::core::Duration lease_duration)
    : kind_(kind),
      lease_duration_(lease_duration)
{
    this->check();
}

void LivelinessDelegate::kind(dds::core::policy::LivelinessKind::Type kind)
{
    kind_ = kind;
}

dds::core::policy::LivelinessKind::Type LivelinessDelegate::kind() const
{
    return kind_;
}

void LivelinessDelegate::lease_duration(const dds::core::Duration& lease_duration)
{
    lease_duration_ = lease_duration;
}

const dds::core::Duration LivelinessDelegate::lease_duration() const
{
    return lease_duration_;
}

bool LivelinessDelegate::operator ==(const LivelinessDelegate& other) const
{
    return other.kind() == kind_ &&
           other.lease_duration() == lease_duration_;
}

void LivelinessDelegate::check() const
{
    /* The kind correctness is enforced by the compiler. */

    /* A Duration object is always valid:
     *      The lease_duration_ is always valid:
     *          Nothing to check. */
}

void LivelinessDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_liveliness_kind_t kind;
    dds_duration_t lease_duration;
    if (dds_qget_liveliness (qos, &kind, &lease_duration)) {
        switch (kind) {
            case DDS_LIVELINESS_AUTOMATIC:
                kind_= dds::core::policy::LivelinessKind::AUTOMATIC;
                break;
            case DDS_LIVELINESS_MANUAL_BY_PARTICIPANT:
                kind_= dds::core::policy::LivelinessKind::MANUAL_BY_PARTICIPANT;
                break;
            case DDS_LIVELINESS_MANUAL_BY_TOPIC:
                kind_ = dds::core::policy::LivelinessKind::MANUAL_BY_TOPIC;
                break;
            default:
                assert(0);
                break;
        }
        lease_duration_ = org::eclipse::cyclonedds::core::convertDuration(lease_duration);
    }
}

void LivelinessDelegate::set_c_policy(dds_qos_t* qos) const
{
    switch (kind_) {
    case dds::core::policy::LivelinessKind::AUTOMATIC:
        dds_qset_liveliness(qos, DDS_LIVELINESS_AUTOMATIC,
                            org::eclipse::cyclonedds::core::convertDuration(lease_duration_));
        break;
    case dds::core::policy::LivelinessKind::MANUAL_BY_PARTICIPANT:
        dds_qset_liveliness(qos, DDS_LIVELINESS_MANUAL_BY_PARTICIPANT,
                            org::eclipse::cyclonedds::core::convertDuration(lease_duration_));
        break;
    case dds::core::policy::LivelinessKind::MANUAL_BY_TOPIC:
        dds_qset_liveliness(qos, DDS_LIVELINESS_MANUAL_BY_TOPIC,
                            org::eclipse::cyclonedds::core::convertDuration(lease_duration_));
        break;
    default:
        assert(0);
        break;
    }
}


//==============================================================================

OwnershipDelegate::OwnershipDelegate(const OwnershipDelegate& other)
    : kind_(other.kind_)
{
}

OwnershipDelegate::OwnershipDelegate(dds::core::policy::OwnershipKind::Type kind)
    : kind_(kind)
{
    this->check();
}

void OwnershipDelegate::kind(dds::core::policy::OwnershipKind::Type kind)
{
    kind_ = kind;
}

dds::core::policy::OwnershipKind::Type OwnershipDelegate::kind() const
{
    return kind_;
}

bool OwnershipDelegate::operator ==(const OwnershipDelegate& other) const
{
    return other.kind() == kind_;
}

void OwnershipDelegate::check() const
{
    /* The kind correctness is enforced by the compiler: nothing to check. */
}

void OwnershipDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_ownership_kind_t kind;
    if (dds_qget_ownership(qos, &kind)) {
        switch (kind) {
            case DDS_OWNERSHIP_SHARED:
                kind_ = dds::core::policy::OwnershipKind::SHARED;
                break;
#ifdef OMG_DDS_OWNERSHIP_SUPPORT
            case DDS_OWNERSHIP_EXCLUSIVE:
                kind_ = dds::core::policy::OwnershipKind::EXCLUSIVE;
                break;
#endif // OMG_DDS_OWNERSHIP_SUPPORT
            default:
                break;
        }
    }
}

void OwnershipDelegate::set_c_policy(dds_qos_t* qos) const
{
    switch (kind_) {
    case dds::core::policy::OwnershipKind::SHARED:
        dds_qset_ownership(qos, DDS_OWNERSHIP_SHARED);
        break;
#ifdef OMG_DDS_OWNERSHIP_SUPPORT
    case dds::core::policy::OwnershipKind::EXCLUSIVE:
        dds_qset_ownership(qos, DDS_OWNERSHIP_EXCLUSIVE);
        break;
#endif // OMG_DDS_OWNERSHIP_SUPPORT
    default:
        assert(0);
        break;
    }
}


//==============================================================================

#ifdef  OMG_DDS_OWNERSHIP_SUPPORT

OwnershipStrengthDelegate::OwnershipStrengthDelegate(const OwnershipStrengthDelegate& other)
    : strength_(other.strength_)
{
}

OwnershipStrengthDelegate::OwnershipStrengthDelegate(int32_t s)
    : strength_(s)
{
    this->check();
}

int32_t OwnershipStrengthDelegate::strength() const
{
    return strength_;
}

void OwnershipStrengthDelegate::strength(int32_t s)
{
    strength_ = s;
}

bool OwnershipStrengthDelegate::operator ==(const OwnershipStrengthDelegate& other) const
{
    return other.strength() == strength_;
}

void OwnershipStrengthDelegate::check() const
{
    /* The strength is just a int32_t: nothing to check. */
}

void OwnershipStrengthDelegate::set_iso_policy(const dds_qos_t* qos)
{
    (void)dds_qget_ownership_strength(qos, &strength_);
}

void OwnershipStrengthDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_qset_ownership_strength(qos, strength_);
}

#endif  // OMG_DDS_OWNERSHIP_SUPPORT


//==============================================================================

PartitionDelegate::PartitionDelegate(const PartitionDelegate& other)
    : name_(other.name_)
{
}

PartitionDelegate::PartitionDelegate(const std::string& partition)
    : name_()
{
    name_.push_back(partition);
    this->check();
}

PartitionDelegate::PartitionDelegate(const dds::core::StringSeq& partitions)
    : name_(partitions)
{
    this->check();
}

void PartitionDelegate::name(const std::string& partition)
{
    name_.clear();
    name_.push_back(partition);
}

void PartitionDelegate::name(const dds::core::StringSeq& partitions)
{
    name_ = partitions;
}

const dds::core::StringSeq& PartitionDelegate::name() const
{
    return name_;
}

bool PartitionDelegate::operator ==(const PartitionDelegate& other) const
{
    return other.name() == name_;
}

void PartitionDelegate::check() const
{
    /* The name_ is just a sequence: nothing to check. */
}

void PartitionDelegate::set_iso_policy(const dds_qos_t* qos)
{
    uint32_t n;
    char **ps;
    if (dds_qget_partition (qos, &n, &ps)) {
        if (n) {
            org::eclipse::cyclonedds::core::convertStringSeq(ps, n, name_);
            while (n > 0) {
                n--;
                dds_free(ps[n]);
            }
        }
        dds_free(ps);
    }
}

void PartitionDelegate::set_c_policy(dds_qos_t* qos) const
{
    if(name_.size())
    {
        char** partitions = 0;
        org::eclipse::cyclonedds::core::convertStringSeq(name_, partitions);
        dds_qset_partition(qos, static_cast<uint32_t>(name_.size()), const_cast<const char**>(static_cast<char**>(partitions)));
        for(size_t i = 0; i < name_.size(); i++)
            delete[] partitions[i];
        delete[] partitions;
    }
}


//==============================================================================

PresentationDelegate::PresentationDelegate(const PresentationDelegate& other)
    : access_scope_(other.access_scope_),
      coherent_access_(other.coherent_access_),
      ordered_access_(other.ordered_access_)
{
}

PresentationDelegate::PresentationDelegate(dds::core::policy::PresentationAccessScopeKind::Type access_scope,
                                           bool coherent_access,
                                           bool ordered_access)
    : access_scope_(access_scope),
      coherent_access_(coherent_access),
      ordered_access_(ordered_access)
{
    this->check();
}

void PresentationDelegate::access_scope(dds::core::policy::PresentationAccessScopeKind::Type as)
{
    access_scope_ = as;
}

dds::core::policy::PresentationAccessScopeKind::Type PresentationDelegate::access_scope() const
{
    return access_scope_;
}

void PresentationDelegate::coherent_access(bool on)
{
    coherent_access_ = on;
}

bool PresentationDelegate::coherent_access() const
{
    return coherent_access_;
}

void PresentationDelegate::ordered_access(bool on)
{
    ordered_access_ = on;
}

bool PresentationDelegate::ordered_access() const
{
    return ordered_access_;
}

bool PresentationDelegate::operator ==(const PresentationDelegate& other) const
{
    return other.access_scope() == access_scope_ &&
           other.coherent_access() == coherent_access_ &&
           other.ordered_access() == ordered_access_;
}

void PresentationDelegate::check() const
{
    /* The enum and bool correctness is enforced by the compiler. */

    if (access_scope_ == dds::core::policy::PresentationAccessScopeKind::TOPIC) {
        if (ordered_access_) {
            ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Presentation::access_scope==TOPIC with ordered_access==true, not currently supported");
        }
    }

    if (access_scope_ == dds::core::policy::PresentationAccessScopeKind::GROUP) {
        if (ordered_access_) {
            ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Presentation::access_scope==GROUP with ordered_access==true, not currently supported");
        }
    }
}

void PresentationDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_presentation_access_scope_kind_t access_scope;
    if (dds_qget_presentation(qos, &access_scope, &coherent_access_, &ordered_access_)) {
        switch (access_scope) {
            case DDS_PRESENTATION_INSTANCE:
                access_scope_ = dds::core::policy::PresentationAccessScopeKind::INSTANCE;
                break;
            case DDS_PRESENTATION_TOPIC:
                access_scope_ = dds::core::policy::PresentationAccessScopeKind::TOPIC;
                break;
            case DDS_PRESENTATION_GROUP:
                access_scope_ = dds::core::policy::PresentationAccessScopeKind::GROUP;
                break;
            default:
                break;
        }
    }
}

void PresentationDelegate::set_c_policy(dds_qos_t* qos) const
{
    switch (access_scope_) {
    case dds::core::policy::PresentationAccessScopeKind::INSTANCE:
        dds_qset_presentation(qos, DDS_PRESENTATION_INSTANCE, coherent_access_, ordered_access_);
        break;
    case dds::core::policy::PresentationAccessScopeKind::TOPIC:
        dds_qset_presentation(qos, DDS_PRESENTATION_TOPIC, coherent_access_, ordered_access_);
        break;
#ifdef OMG_DDS_OBJECT_MODEL_SUPPORT
    case dds::core::policy::PresentationAccessScopeKind::GROUP:
        dds_qset_presentation(qos, DDS_PRESENTATION_GROUP, coherent_access_, ordered_access_);
        break;
#endif // OMG_DDS_OBJECT_MODEL_SUPPORT
    default:
        assert(0);
        break;
    }
}

//==============================================================================

ReaderDataLifecycleDelegate::ReaderDataLifecycleDelegate(const ReaderDataLifecycleDelegate& other)
    : autopurge_nowriter_samples_delay_(other.autopurge_nowriter_samples_delay_),
      autopurge_disposed_samples_delay_(other.autopurge_disposed_samples_delay_)
{
}

ReaderDataLifecycleDelegate::ReaderDataLifecycleDelegate(const dds::core::Duration& nowriter_delay,
                                                         const dds::core::Duration& disposed_samples_delay)
    : autopurge_nowriter_samples_delay_(nowriter_delay),
      autopurge_disposed_samples_delay_(disposed_samples_delay)
{
    this->check();
}

const dds::core::Duration ReaderDataLifecycleDelegate::autopurge_nowriter_samples_delay() const
{
    return autopurge_nowriter_samples_delay_;
}

void ReaderDataLifecycleDelegate::autopurge_nowriter_samples_delay(const dds::core::Duration& d)
{
    autopurge_nowriter_samples_delay_ = d;
}

const dds::core::Duration ReaderDataLifecycleDelegate::autopurge_disposed_samples_delay() const
{
    return autopurge_disposed_samples_delay_;
}

void ReaderDataLifecycleDelegate::autopurge_disposed_samples_delay(const dds::core::Duration& d)
{
    autopurge_disposed_samples_delay_ = d;
}

bool ReaderDataLifecycleDelegate::operator ==(const ReaderDataLifecycleDelegate& other) const
{
    return other.autopurge_nowriter_samples_delay() == autopurge_nowriter_samples_delay_ &&
           other.autopurge_disposed_samples_delay() == autopurge_disposed_samples_delay_;
}

void ReaderDataLifecycleDelegate::check() const
{
    /* A Duration object is always valid:
     *      The autopurge_nowriter_samples_delay_ duration is always valid
     *      The autopurge_disposed_samples_delay_ duration is always valid
     *          Nothing to check. */
}

void ReaderDataLifecycleDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_duration_t autopurge_nowriter_samples_delay;
    dds_duration_t autopurge_disposed_samples_delay;
    if (dds_qget_reader_data_lifecycle (qos, &autopurge_nowriter_samples_delay, &autopurge_disposed_samples_delay)) {
        autopurge_nowriter_samples_delay_ = org::eclipse::cyclonedds::core::convertDuration(autopurge_nowriter_samples_delay);
        autopurge_disposed_samples_delay_ = org::eclipse::cyclonedds::core::convertDuration(autopurge_disposed_samples_delay);
    }
}

void ReaderDataLifecycleDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_qset_reader_data_lifecycle(qos, org::eclipse::cyclonedds::core::convertDuration(autopurge_nowriter_samples_delay_),
                                   org::eclipse::cyclonedds::core::convertDuration(autopurge_disposed_samples_delay_));
}

//==============================================================================

ReliabilityDelegate::ReliabilityDelegate(const ReliabilityDelegate& other)
    :  kind_(other.kind_),
       max_blocking_time_(other.max_blocking_time_)
{
}

ReliabilityDelegate::ReliabilityDelegate(dds::core::policy::ReliabilityKind::Type kind,
                                         const dds::core::Duration& max_blocking_time)
    :  kind_(kind),
       max_blocking_time_(max_blocking_time)
{
    this->check();
}

void ReliabilityDelegate::kind(dds::core::policy::ReliabilityKind::Type kind)
{
    kind_ = kind;
}

dds::core::policy::ReliabilityKind::Type ReliabilityDelegate::kind() const
{
    return kind_;
}

void ReliabilityDelegate::max_blocking_time(const dds::core::Duration& d)
{
    max_blocking_time_ = d;
}

const dds::core::Duration ReliabilityDelegate::max_blocking_time() const
{
    return max_blocking_time_;
}

bool ReliabilityDelegate::operator ==(const ReliabilityDelegate& other) const
{
    return other.kind() == kind_ &&
           other.max_blocking_time() == max_blocking_time_;
}

void ReliabilityDelegate::check() const
{
    /* The kind correctness is enforced by the compiler. */
    /* A Duration object is always valid: the max_blocking_time_ is valid. */
}

void ReliabilityDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_reliability_kind_t kind;
    dds_duration_t max_blocking_time;
    if (dds_qget_reliability (qos, &kind, &max_blocking_time)) {
        switch (kind) {
            case DDS_RELIABILITY_RELIABLE:
                kind_ = dds::core::policy::ReliabilityKind::RELIABLE;
                break;
            case DDS_RELIABILITY_BEST_EFFORT:
                kind_ = dds::core::policy::ReliabilityKind::BEST_EFFORT;
                break;
            default:
                break;
        }
        max_blocking_time_ = org::eclipse::cyclonedds::core::convertDuration(max_blocking_time);
    }
}

void ReliabilityDelegate::set_c_policy(dds_qos_t* qos) const
{
    switch (kind_) {
    case dds::core::policy::ReliabilityKind::BEST_EFFORT:
        dds_qset_reliability(qos, DDS_RELIABILITY_BEST_EFFORT,
                             org::eclipse::cyclonedds::core::convertDuration(max_blocking_time_));
        break;
    case dds::core::policy::ReliabilityKind::RELIABLE:
        dds_qset_reliability(qos, DDS_RELIABILITY_RELIABLE,
                             org::eclipse::cyclonedds::core::convertDuration(max_blocking_time_));
        break;
    default:
        assert(0);
        break;
    }
}


//==============================================================================

ResourceLimitsDelegate::ResourceLimitsDelegate(const ResourceLimitsDelegate& other)
    : max_samples_(other.max_samples_),
      max_instances_(other.max_instances_),
      max_samples_per_instance_(other.max_samples_per_instance_)
{
}

ResourceLimitsDelegate::ResourceLimitsDelegate(int32_t max_samples,
                                               int32_t max_instances,
                                               int32_t max_samples_per_instance)
    : max_samples_(max_samples),
      max_instances_(max_instances),
      max_samples_per_instance_(max_samples_per_instance)
{
    this->check();
}

void ResourceLimitsDelegate::max_samples(int32_t samples)
{
    max_samples_ = samples;
}

int32_t ResourceLimitsDelegate::max_samples() const
{
    return max_samples_;
}

void ResourceLimitsDelegate::max_instances(int32_t max_instances)
{
    max_instances_ = max_instances;
}

int32_t ResourceLimitsDelegate::max_instances() const
{
    return max_instances_;
}

void ResourceLimitsDelegate::max_samples_per_instance(int32_t max_samples_per_instance)
{
    max_samples_per_instance_ = max_samples_per_instance;
}

int32_t ResourceLimitsDelegate::max_samples_per_instance() const
{
    return max_samples_per_instance_;
}

bool ResourceLimitsDelegate::operator ==(const ResourceLimitsDelegate& other) const
{
    return other.max_samples() == max_samples_ &&
           other.max_instances() == max_instances_ &&
           other.max_samples_per_instance() == max_samples_per_instance_;
}

void ResourceLimitsDelegate::check() const
{
    if ((max_samples_ <= 0) && (max_samples_ != dds::core::LENGTH_UNLIMITED)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_INVALID_ARGUMENT_ERROR, "Invalid ResourceLimits::max_samples (%ld) value.", max_samples_);
    }
    if ((max_instances_ <= 0) && (max_instances_ != dds::core::LENGTH_UNLIMITED)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_INVALID_ARGUMENT_ERROR, "Invalid ResourceLimits::max_instances (%ld) value.", max_instances_);
    }
    if ((max_samples_per_instance_ <= 0) && (max_samples_per_instance_ != dds::core::LENGTH_UNLIMITED)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_INVALID_ARGUMENT_ERROR, "Invalid ResourceLimits::max_samples_per_instance (%ld) value.", max_samples_per_instance_);
    }
}

void ResourceLimitsDelegate::check_against(const org::eclipse::cyclonedds::core::policy::HistoryDelegate& history) const
{
    if (history.kind() == dds::core::policy::HistoryKind::KEEP_LAST) {
        if (this->max_samples_per_instance_ != dds::core::LENGTH_UNLIMITED) {
            if (history.depth() > this->max_samples_per_instance_) {
                ISOCPP_THROW_EXCEPTION(ISOCPP_INCONSISTENT_POLICY_ERROR, "History::depth (%ld) > ResourceLimits::max_samples_per_instance (%ld) with KEEP_LAST", history.depth(), max_samples_per_instance_);
            }
        }
    }
}

void ResourceLimitsDelegate::set_iso_policy(const dds_qos_t* qos)
{
    (void)dds_qget_resource_limits(qos, &max_samples_, &max_instances_, &max_samples_per_instance_);
}

void ResourceLimitsDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_qset_resource_limits(qos, max_samples_, max_instances_, max_samples_per_instance_);
}

//==============================================================================

TimeBasedFilterDelegate::TimeBasedFilterDelegate(const TimeBasedFilterDelegate& other)
    : min_sep_(other.min_sep_)
{
}

TimeBasedFilterDelegate::TimeBasedFilterDelegate(const dds::core::Duration& min_separation)
    : min_sep_(min_separation)
{
    this->check();
}

void TimeBasedFilterDelegate::min_separation(const dds::core::Duration& ms)
{
    min_sep_ = ms;
}

const dds::core::Duration TimeBasedFilterDelegate::min_separation() const
{
    return min_sep_;
}

bool TimeBasedFilterDelegate::operator ==(const TimeBasedFilterDelegate& other) const
{
    return other.min_separation() == min_sep_;
}

void TimeBasedFilterDelegate::check() const
{
    /* A Duration object is always valid:
     *      The min_sep_ duration is always valid:
     *          Nothing to check. */
}

void TimeBasedFilterDelegate::check_against(const org::eclipse::cyclonedds::core::policy::DeadlineDelegate& deadline) const
{
    if (deadline.period() < this->min_sep_) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_INCONSISTENT_POLICY_ERROR, "Deadline: period < TimeBasedFilter::min_separation");
    }
}

void TimeBasedFilterDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_duration_t minimum_separation;
    if (dds_qget_time_based_filter(qos, &minimum_separation)) {
        min_sep_ = org::eclipse::cyclonedds::core::convertDuration(minimum_separation);
    }
}

void TimeBasedFilterDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_qset_time_based_filter(qos, org::eclipse::cyclonedds::core::convertDuration(min_sep_));
}


//==============================================================================

TopicDataDelegate::TopicDataDelegate()
    : value_()
{
}

TopicDataDelegate::TopicDataDelegate(const TopicDataDelegate& other)
    : value_(other.value_)
{
}

TopicDataDelegate::TopicDataDelegate(const dds::core::ByteSeq& seq)
    : value_(seq)
{
    this->check();
}

void TopicDataDelegate::value(const dds::core::ByteSeq& seq)
{
    value_ = seq;
}

const dds::core::ByteSeq& TopicDataDelegate::value() const
{
    return value_;
}

bool TopicDataDelegate::operator ==(const TopicDataDelegate& other) const
{
    return other.value() == value_;
}

void TopicDataDelegate::check() const
{
    /* The value_ is just a sequence: nothing to check. */
}

void TopicDataDelegate::set_iso_policy(const dds_qos_t* qos)
{
    void *value;
    size_t sz;
    if (dds_qget_topicdata(qos, &value, &sz)) {
        if (sz > 0) {
            org::eclipse::cyclonedds::core::convertByteSeq(value, static_cast<int32_t>(sz), value_);
        }
        dds_free(value);
    }
}

void TopicDataDelegate::set_c_policy(dds_qos_t* qos) const
{
    if(value_.size())
    {
        void* data = NULL;
        org::eclipse::cyclonedds::core::convertByteSeq(value_, data, static_cast<int32_t>(value_.size()));
        dds_qset_topicdata(qos, data, value_.size());
    }
}


//==============================================================================

TransportPriorityDelegate::TransportPriorityDelegate(const TransportPriorityDelegate& other)
    : value_(other.value_)
{
}

TransportPriorityDelegate::TransportPriorityDelegate(int32_t prio)
    : value_(prio)
{
    this->check();
}

void TransportPriorityDelegate::value(int32_t prio)
{
    value_ = prio;
}

int32_t TransportPriorityDelegate::value() const
{
    return value_;
}

bool TransportPriorityDelegate::operator ==(const TransportPriorityDelegate& other) const
{
    return other.value() == value_;
}

void TransportPriorityDelegate::check() const
{
    /* Any value is valid: nothing to check. */
}

void TransportPriorityDelegate::set_iso_policy(const dds_qos_t* qos)
{
    int32_t value;
    if (dds_qget_transport_priority(qos, &value)) {
        value_ = value;
    }
}

void TransportPriorityDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_qset_transport_priority(qos, value_);
}


//==============================================================================

UserDataDelegate::UserDataDelegate()
    : value_()
{
}

UserDataDelegate::UserDataDelegate(const UserDataDelegate& other)
    : value_(other.value_)
{
}

UserDataDelegate::UserDataDelegate(const dds::core::ByteSeq& seq)
    : value_(seq)
{
    this->check();
}

void UserDataDelegate::value(const dds::core::ByteSeq& seq)
{
    value_ = seq;
}

const dds::core::ByteSeq UserDataDelegate::value() const
{
    return value_;
}

bool UserDataDelegate::operator ==(const UserDataDelegate& other) const
{
    return other.value() == value_;
}

void UserDataDelegate::check() const
{
    /* The value_ is just a sequence: nothing to check. */
}

void UserDataDelegate::set_iso_policy(const dds_qos_t* qos)
{
    void *value;
    size_t sz;
    if (dds_qget_userdata(qos, &value, &sz)) {
        if(sz > 0)
        {
            org::eclipse::cyclonedds::core::convertByteSeq(value, static_cast<int32_t>(sz), value_);
        }
        dds_free(value);
    }
}

void UserDataDelegate::set_c_policy(dds_qos_t* qos) const
{
    if(value_.size())
    {
        void* data = NULL;
        org::eclipse::cyclonedds::core::convertByteSeq(value_, data, static_cast<int32_t>(value_.size()));
        dds_qset_userdata(qos, data, value_.size());
    }
}


//==============================================================================

WriterDataLifecycleDelegate::WriterDataLifecycleDelegate(const WriterDataLifecycleDelegate& other)
    : autodispose_(other.autodispose_)
{
}

WriterDataLifecycleDelegate::WriterDataLifecycleDelegate(bool autodispose)
    : autodispose_(autodispose)
{
    this->check();
}

bool WriterDataLifecycleDelegate::autodispose() const
{
    return autodispose_;
}

void WriterDataLifecycleDelegate::autodispose(bool b)
{
    autodispose_ = b;
}

bool WriterDataLifecycleDelegate::operator ==(const WriterDataLifecycleDelegate& other) const
{
    return other.autodispose() == autodispose_;
}

void WriterDataLifecycleDelegate::check() const
{
    /* The autodispose is just a boolean: nothing to check. */
}

void WriterDataLifecycleDelegate::set_iso_policy(const dds_qos_t* qos)
{
    (void)dds_qget_writer_data_lifecycle(qos, &autodispose_);
}

void WriterDataLifecycleDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_qset_writer_data_lifecycle(qos, autodispose_);
}


//==============================================================================

#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

DataRepresentationDelegate::DataRepresentationDelegate(const DataRepresentationDelegate& other)
    : value_(other.value_)
{
}

DataRepresentationDelegate::DataRepresentationDelegate(const dds::core::policy::DataRepresentationIdSeq& value)
    : value_(value)
{
    this->check();
}

void DataRepresentationDelegate::value(const dds::core::policy::DataRepresentationIdSeq &value)
{
    value_ = value;
}

const dds::core::policy::DataRepresentationIdSeq& DataRepresentationDelegate::value() const
{
    return value_;
}

bool DataRepresentationDelegate::operator ==(const DataRepresentationDelegate& other) const
{
    return other.value() == value_;
}

void DataRepresentationDelegate::check() const
{
    /* The kind correctness is enforced by the compiler: nothing to check. */
}

void DataRepresentationDelegate::set_iso_policy(const dds_qos_t* qos)
{
    uint32_t n = 0;
    dds_data_representation_id_t *value = NULL;
    decltype(value_) tmp;
    if (dds_qget_data_representation(qos, &n, &value) && n > 0 && value != NULL) {
        for (uint32_t i = 0; i < n; i++) {
            switch (value[i]) {
                case DDS_DATA_REPRESENTATION_XCDR1:
                    tmp.push_back(dds::core::policy::DataRepresentationId::XCDR1);
                    break;
                case DDS_DATA_REPRESENTATION_XML:
                    tmp.push_back(dds::core::policy::DataRepresentationId::XML);
                    break;
                case DDS_DATA_REPRESENTATION_XCDR2:
                    tmp.push_back(dds::core::policy::DataRepresentationId::XCDR2);
                    break;
                default:
                    break;
            }
        }
    }
    value_ = tmp;

    if (value)
      dds_free(value);
}

void DataRepresentationDelegate::set_c_policy(dds_qos_t* qos) const
{
    if(value_.size())
    {
        const auto value = value_;
        std::vector<dds_data_representation_id_t> reps;
        for (const auto & v:value) {
            switch (v.underlying()) {
            case dds::core::policy::DataRepresentationId::XCDR1:
                reps.push_back(static_cast<dds_data_representation_id_t>(DDS_DATA_REPRESENTATION_XCDR1));
                break;
            case dds::core::policy::DataRepresentationId::XML:
                reps.push_back(static_cast<dds_data_representation_id_t>(DDS_DATA_REPRESENTATION_XML));
                break;
            case dds::core::policy::DataRepresentationId::XCDR2:
                reps.push_back(static_cast<dds_data_representation_id_t>(DDS_DATA_REPRESENTATION_XCDR2));
                break;
            }
        }
        dds_qset_data_representation(qos, static_cast<uint32_t>(reps.size()), reps.data());
    }
}

#endif  // OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

//==============================================================================

#ifdef  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

TypeConsistencyEnforcementDelegate::TypeConsistencyEnforcementDelegate(const TypeConsistencyEnforcementDelegate& other):
    kind_(other.kind_),
    ignore_sequence_bounds_(other.ignore_sequence_bounds_),
    ignore_string_bounds_(other.ignore_string_bounds_),
    ignore_member_names_(other.ignore_member_names_),
    prevent_type_widening_(other.prevent_type_widening_),
    force_type_validation_(other.force_type_validation_)
{

}

TypeConsistencyEnforcementDelegate::TypeConsistencyEnforcementDelegate(
    dds::core::policy::TypeConsistencyKind kind,
    bool ignore_sequence_bounds,
    bool ignore_string_bounds,
    bool ignore_member_names,
    bool prevent_type_widening,
    bool force_type_validation):
        kind_(kind),
        ignore_sequence_bounds_(ignore_sequence_bounds),
        ignore_string_bounds_(ignore_string_bounds),
        ignore_member_names_(ignore_member_names),
        prevent_type_widening_(prevent_type_widening),
        force_type_validation_(force_type_validation)
{
    this->check();
}

void TypeConsistencyEnforcementDelegate::kind(dds::core::policy::TypeConsistencyKind kind)
{
    kind_ = kind;
}

dds::core::policy::TypeConsistencyKind TypeConsistencyEnforcementDelegate::kind() const
{
    return kind_;
}

void TypeConsistencyEnforcementDelegate::ignore_sequence_bounds(bool ignore_sequence_bounds)
{
    ignore_sequence_bounds_ = ignore_sequence_bounds;
}

bool TypeConsistencyEnforcementDelegate::ignore_sequence_bounds() const
{
    return ignore_sequence_bounds_;
}

void TypeConsistencyEnforcementDelegate::ignore_string_bounds(bool ignore_string_bounds)
{
    ignore_string_bounds_ = ignore_string_bounds;
}

bool TypeConsistencyEnforcementDelegate::ignore_string_bounds() const
{
    return ignore_string_bounds_;
}

void TypeConsistencyEnforcementDelegate::ignore_member_names(bool ignore_member_names)
{
    ignore_member_names_ = ignore_member_names;
}

bool TypeConsistencyEnforcementDelegate::ignore_member_names() const
{
    return ignore_member_names_;
}

void TypeConsistencyEnforcementDelegate::prevent_type_widening(bool prevent_type_widening)
{
    prevent_type_widening_ = prevent_type_widening;
}

bool TypeConsistencyEnforcementDelegate::prevent_type_widening() const
{
    return prevent_type_widening_;
}

void TypeConsistencyEnforcementDelegate::force_type_validation(bool force_type_validation)
{
    force_type_validation_ = force_type_validation;
}

bool TypeConsistencyEnforcementDelegate::force_type_validation() const
{
    return force_type_validation_;
}

bool TypeConsistencyEnforcementDelegate::operator ==(const TypeConsistencyEnforcementDelegate& other) const
{
    return other.kind() == kind_ &&
           other.ignore_sequence_bounds() == ignore_sequence_bounds_ &&
           other.ignore_string_bounds() == ignore_string_bounds_ &&
           other.ignore_member_names() == ignore_member_names_ &&
           other.prevent_type_widening() == prevent_type_widening_ &&
           other.force_type_validation() == force_type_validation_;
}

void TypeConsistencyEnforcementDelegate::check() const
{
    /* The kind correctness is enforced by the compiler: nothing to check. */
}

void TypeConsistencyEnforcementDelegate::set_iso_policy(const dds_qos_t* qos)
{
    dds_type_consistency_kind_t kind = DDS_TYPE_CONSISTENCY_DISALLOW_TYPE_COERCION;
    bool ignore_sequence_bounds = false;
    bool ignore_string_bounds = false;
    bool ignore_member_names = false;
    bool prevent_type_widening = false;
    bool force_type_validation = false;

    if (dds_qget_type_consistency(qos,
                                  &kind,
                                  &ignore_sequence_bounds,
                                  &ignore_string_bounds,
                                  &ignore_member_names,
                                  &prevent_type_widening,
                                  &force_type_validation)) {
        switch (kind) {
            case DDS_TYPE_CONSISTENCY_DISALLOW_TYPE_COERCION:
                kind_ = dds::core::policy::TypeConsistencyKind::DISALLOW_TYPE_COERCION;
                break;
            case DDS_TYPE_CONSISTENCY_ALLOW_TYPE_COERCION:
                kind_ = dds::core::policy::TypeConsistencyKind::ALLOW_TYPE_COERCION;
                break;
            default:
                return;
        }
        ignore_sequence_bounds_ = ignore_sequence_bounds;
        ignore_string_bounds_ = ignore_string_bounds;
        ignore_member_names_ = ignore_member_names;
        prevent_type_widening_ = prevent_type_widening;
        force_type_validation_ = force_type_validation;
    }
}

void TypeConsistencyEnforcementDelegate::set_c_policy(dds_qos_t* qos) const
{
    dds_type_consistency_kind_t kind = DDS_TYPE_CONSISTENCY_DISALLOW_TYPE_COERCION;
    switch (kind_.underlying()) {
        case dds::core::policy::TypeConsistencyKind::DISALLOW_TYPE_COERCION:
            kind = DDS_TYPE_CONSISTENCY_DISALLOW_TYPE_COERCION;
            break;
        case dds::core::policy::TypeConsistencyKind::ALLOW_TYPE_COERCION:
            kind = DDS_TYPE_CONSISTENCY_ALLOW_TYPE_COERCION;
            break;
        default:
            return;
    }
    dds_qset_type_consistency (
        qos,
        kind,
        ignore_sequence_bounds_,
        ignore_string_bounds_,
        ignore_member_names_,
        prevent_type_widening_,
        force_type_validation_);
}


#endif  // OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

}
}
}
}
}
