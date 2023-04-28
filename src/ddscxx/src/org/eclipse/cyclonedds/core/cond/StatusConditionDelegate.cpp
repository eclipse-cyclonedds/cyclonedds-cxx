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

#include <org/eclipse/cyclonedds/core/cond/StatusConditionDelegate.hpp>
#include <org/eclipse/cyclonedds/core/EntityDelegate.hpp>
#include <org/eclipse/cyclonedds/core/MiscUtils.hpp>

#include <dds/domain/DomainParticipant.hpp>
#include <dds/core/cond/StatusCondition.hpp>
#include <dds/core/status/State.hpp>

#include "dds/dds.h"


org::eclipse::cyclonedds::core::cond::StatusConditionDelegate::StatusConditionDelegate(
        const org::eclipse::cyclonedds::core::EntityDelegate *entity,
        dds_entity_t e) :
                org::eclipse::cyclonedds::core::cond::ConditionDelegate(),
                myEntity(::std::dynamic_pointer_cast
                                 <org::eclipse::cyclonedds::core::EntityDelegate>
                          (entity->get_strong_ref()))
{
  this->ddsc_entity = e;
}

org::eclipse::cyclonedds::core::cond::StatusConditionDelegate::~StatusConditionDelegate()
{
    if (!this->closed) {
        try {
            this->close();
        } catch (...) {
            /* Empty: the exception throw should have already traced an error. */
        }
    }
}

void
org::eclipse::cyclonedds::core::cond::StatusConditionDelegate::close()
{
    /* todo: cannot close this->ddsc_entity because this is the parent EntityDelegate;
        find better solution to get correct feedback when closed StatusCondition is used */

    // detach the condition from any waitset
    this->check();
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    ConditionDelegate::detach_from_waitset(ddsc_entity);
    // set the entity to 0
    this->ddsc_entity = DDS_HANDLE_NIL;
    // but don't close the entity as it is the parent entity (fix this above as per above todo)
    // ConditionDelegate::detach_and_close(ddsc_entity);
}

void
org::eclipse::cyclonedds::core::cond::StatusConditionDelegate::init(
        org::eclipse::cyclonedds::core::ObjectDelegate::weak_ref_type weak_ref)
{
    /* Set weak_ref before passing ourselves to other isocpp objects. */
    this->set_weak_ref(weak_ref);
}

void
org::eclipse::cyclonedds::core::cond::StatusConditionDelegate::enabled_statuses(
        const dds::core::status::StatusMask& status)
{
    uint32_t ddsc_status_mask = convertStatusMask(status);

    dds_return_t res = dds_set_status_mask(ddsc_entity, ddsc_status_mask);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(res, "dds_status_set_enabled failed");
}

dds::core::status::StatusMask
org::eclipse::cyclonedds::core::cond::StatusConditionDelegate::enabled_statuses() const
{
    uint32_t ddsc_status_mask;
    dds::core::status::StatusMask status_mask;

    dds_return_t res = dds_get_status_mask(ddsc_entity, &ddsc_status_mask);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(res, "dds_status_set_enabled failed");

    status_mask = convertStatusMask(ddsc_status_mask);

    return status_mask;
}

dds::core::Entity&
org::eclipse::cyclonedds::core::cond::StatusConditionDelegate::entity()
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds::core::Entity& resultEntity(this->myEntity);

    return resultEntity;
}

bool
org::eclipse::cyclonedds::core::cond::StatusConditionDelegate::trigger_value() const
{
    this->check();

    return dds_triggered(ddsc_entity);
}

dds::core::cond::TStatusCondition
                        <org::eclipse::cyclonedds::core::cond::StatusConditionDelegate>
org::eclipse::cyclonedds::core::cond::StatusConditionDelegate::wrapper()
{
    org::eclipse::cyclonedds::core::cond::StatusConditionDelegate::ref_type ref =
        ::std::dynamic_pointer_cast<StatusConditionDelegate>
                                                    (this->get_strong_ref());

    dds::core::cond::TStatusCondition<StatusConditionDelegate> statusCondition(ref);

    return statusCondition;
}
