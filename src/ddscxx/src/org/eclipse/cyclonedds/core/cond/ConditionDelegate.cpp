// Copyright(c) 2006 to 2020 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <org/eclipse/cyclonedds/core/cond/ConditionDelegate.hpp>
#include <org/eclipse/cyclonedds/core/cond/WaitSetDelegate.hpp>
#include <dds/core/cond/WaitSet.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>

#include <dds/core/cond/Condition.hpp>

#include "dds/dds.h"

org::eclipse::cyclonedds::core::cond::ConditionDelegate::ConditionDelegate() :
        myFunctor(NULL)
{
}

org::eclipse::cyclonedds::core::cond::ConditionDelegate::~ConditionDelegate()
{
    if (!this->closed) {
        try {
            DDScObjectDelegate::close();
        } catch (...) {
           /* Empty: the exception throw should have already traced an error. */
        }
    }

    // Delete functor ('this' may already be closed at this point, but make
    // sure that functor is delete here
    if (this->myFunctor) {
        delete this->myFunctor;
        this->myFunctor = nullptr;
    }
}

void
org::eclipse::cyclonedds::core::cond::ConditionDelegate::detach_from_waitset(
    const dds_entity_t entity_handle) {
    std::vector<WaitSetDelegate *> waitset_list_tmp;
    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLockForCopy(this->waitSetListUpdateMutex);
    waitset_list_tmp.assign(this->waitSetList.begin(), this->waitSetList.end());
    scopedLockForCopy.unlock();

    for (auto waitset : waitset_list_tmp) {
        org::eclipse::cyclonedds::core::ScopedObjectLock scopedWaisetLock(*waitset);
        {
            org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(this->waitSetListUpdateMutex);
            // remove the waitset from the list and detach the condition
            if (this->waitSetList.erase(waitset)) {
                waitset->remove_condition_locked(this, entity_handle);
            }
        }
    }
}

void
org::eclipse::cyclonedds::core::cond::ConditionDelegate::detach_and_close(
    const dds_entity_t entity_handle)
{
    detach_from_waitset(entity_handle);
    org::eclipse::cyclonedds::core::cond::ConditionDelegate::close();
}

void
org::eclipse::cyclonedds::core::cond::ConditionDelegate::close()
{
    // close the condition
    DDScObjectDelegate::close();

    if (this->myFunctor) {
        delete this->myFunctor;
        this->myFunctor = nullptr;
    }
}


void
org::eclipse::cyclonedds::core::cond::ConditionDelegate::init(
                                      ObjectDelegate::weak_ref_type weak_ref)
{
    /* Set weak_ref before passing ourselves to other isocpp objects. */
    this->set_weak_ref(weak_ref);
    /* Add weak_ref to the map of entities */
    this->add_to_entity_map(weak_ref);
}

void
org::eclipse::cyclonedds::core::cond::ConditionDelegate::reset_handler()
{
    if (this->myFunctor) {
        delete this->myFunctor;
        this->myFunctor = NULL;
    }
}


void
org::eclipse::cyclonedds::core::cond::ConditionDelegate::dispatch()
{
    if (this->trigger_value() && this->myFunctor)
    {
       dds::core::cond::TCondition<
                     org::eclipse::cyclonedds::core::cond::ConditionDelegate>
                                                       cond = this->wrapper();
        this->myFunctor->dispatch(cond);
    }
}

void
org::eclipse::cyclonedds::core::cond::ConditionDelegate::add_waitset(
    const dds::core::cond::TCondition<ConditionDelegate> & cond,
    org::eclipse::cyclonedds::core::cond::WaitSetDelegate *waitset)
{
    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(this->waitSetListUpdateMutex);

    // Insert waitset to the list and attach the condition
    if (this->waitSetList.insert(waitset).second) {
        waitset->add_condition_locked(cond);
    }
}

bool
org::eclipse::cyclonedds::core::cond::ConditionDelegate::remove_waitset(
    org::eclipse::cyclonedds::core::cond::WaitSetDelegate *waitset)
{
    bool ret = false;
    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(this->waitSetListUpdateMutex);

    // remove the waitset from the list and detach the condition
    if (this->waitSetList.erase(waitset)) {
        waitset->remove_condition_locked(this);
        ret = true;
    }
    // since the API expects to return false even in cases when condition was not attached to
    // waitset
  return ret;
}

dds::core::cond::TCondition<org::eclipse::cyclonedds::core::cond::ConditionDelegate>
org::eclipse::cyclonedds::core::cond::ConditionDelegate::wrapper()
{
    org::eclipse::cyclonedds::core::cond::ConditionDelegate::ref_type ref =
          ::std::dynamic_pointer_cast<ConditionDelegate>
                                                      (this->get_strong_ref());

    dds::core::cond::TCondition<org::eclipse::cyclonedds::core::cond::ConditionDelegate>
                                                                condition(ref);

    return condition;
}
