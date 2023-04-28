// Copyright(c) 2006 to 2020 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <org/eclipse/cyclonedds/sub/cond/ReadConditionDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>



org::eclipse::cyclonedds::sub::cond::ReadConditionDelegate::ReadConditionDelegate(
    const dds::sub::AnyDataReader& dr,
    const dds::sub::status::DataState& state_filter) :
        QueryDelegate(dr, state_filter)
{
    dds_entity_t ddsc_dr;
    dds_entity_t ddsc_read_cond;
    uint32_t ddsc_mask;

    // Get ddsc entity for reader
    ddsc_dr = dr.delegate()->get_ddsc_entity();
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_dr, "Could not get reader entity");

    // Create the mask corresponding to the sample, view and instance state
    ddsc_mask = dr.delegate()->get_ddsc_state_mask(state_filter);

    // Create ddsc read condition
    ddsc_read_cond = dds_create_readcondition(ddsc_dr, ddsc_mask);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_read_cond, "Could not create read condition.");
    this->set_ddsc_entity(ddsc_read_cond);
}

/* The close() operation of Condition will try to remove this Condition from
 * its WaitSets. However, since the WaitSets hold a reference to their Conditions,
 * the destructor can never be invoked for Conditions that are still attached
 * to WaitSets.
 * For that reason we know that if the destructor is invoked, the Condition
 * can no longer be attached to a WaitSet, so we can skip the local close()
 * and immediately proceed the the close() of the DDScObjectDelegate parent.
 * If we would try to invoke Condition::close() here, then we would run
 * into a deadlock when we claim the WaitSet lock in case this destructor
 * is invoked by the destructor of the WaitSet, which has the WaitSet already
 * locked before.
 */
org::eclipse::cyclonedds::sub::cond::ReadConditionDelegate::~ReadConditionDelegate()
{
    if (!this->closed) {
        try {
            QueryDelegate::deinit();
        } catch (...) {
            /* Empty: the exception throw should have already traced an error. */
        }
    }
}

void
org::eclipse::cyclonedds::sub::cond::ReadConditionDelegate::init(
    ObjectDelegate::weak_ref_type weak_ref)
{
    QueryDelegate::init(weak_ref);
}

void
org::eclipse::cyclonedds::sub::cond::ReadConditionDelegate::close()
{
    this->check();
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    QueryDelegate::deinit();
    ConditionDelegate::detach_and_close(ddsc_entity);
}

bool
org::eclipse::cyclonedds::sub::cond::ReadConditionDelegate::trigger_value() const
{
    this->check();

    return dds_triggered(ddsc_entity);
}
