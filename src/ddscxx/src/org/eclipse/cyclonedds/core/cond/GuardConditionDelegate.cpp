// Copyright(c) 2006 to 2020 ZettaScale Technology and others
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

#include <org/eclipse/cyclonedds/core/cond/GuardConditionDelegate.hpp>
#include <org/eclipse/cyclonedds/core/cond/WaitSetDelegate.hpp>


org::eclipse::cyclonedds::core::cond::GuardConditionDelegate::GuardConditionDelegate() :
        org::eclipse::cyclonedds::core::cond::ConditionDelegate()
{
    dds_entity_t ddsc_guard_cond;

    ddsc_guard_cond = dds_create_guardcondition(DDS_CYCLONEDDS_HANDLE);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_guard_cond, "Could not create guard condition.");
    this->set_ddsc_entity(ddsc_guard_cond);
}

org::eclipse::cyclonedds::core::cond::GuardConditionDelegate::~GuardConditionDelegate()
{
}

void
org::eclipse::cyclonedds::core::cond::GuardConditionDelegate::close()
{
    this->check();
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    ConditionDelegate::detach_and_close(ddsc_entity);
}


bool
org::eclipse::cyclonedds::core::cond::GuardConditionDelegate::trigger_value() const
{
    bool triggered;
    dds_return_t ret;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    this->check();
    ret = dds_read_guardcondition(this->ddsc_entity, &triggered);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Failed to get guard condition trigger value");

    return triggered;
}

void
org::eclipse::cyclonedds::core::cond::GuardConditionDelegate::trigger_value(bool value)
{
    dds_return_t ret;
    bool triggered;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    if (value) {
        ret = dds_set_guardcondition(this->ddsc_entity, value);
        ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Failed to set guard condition trigger value");
    } else {
        ret = dds_take_guardcondition(this->ddsc_entity, &triggered);
        ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Failed to reset guard condition");
    }
}
