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

#include <dds/domain/DomainParticipant.hpp>
#include <org/eclipse/cyclonedds/core/MiscUtils.hpp>
#include <org/eclipse/cyclonedds/core/cond/WaitSetDelegate.hpp>

#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>
#include <org/eclipse/cyclonedds/core/Mutex.hpp>


org::eclipse::cyclonedds::core::cond::WaitSetDelegate::WaitSetDelegate()
{
    dds_entity_t ddsc_waitset;

    ddsc_waitset = dds_create_waitset(DDS_CYCLONEDDS_HANDLE);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_waitset, "Could not create waitset.");
    this->set_ddsc_entity(ddsc_waitset);
}

org::eclipse::cyclonedds::core::cond::WaitSetDelegate::~WaitSetDelegate()
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
org::eclipse::cyclonedds::core::cond::WaitSetDelegate::init(ObjectDelegate::weak_ref_type weak_ref)
{
    this->set_weak_ref(weak_ref);
}

void
org::eclipse::cyclonedds::core::cond::WaitSetDelegate::close()
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);


    ConditionSeq cond_seq;
    for(const auto & cond : conditions_) {
      cond_seq.push_back(cond.second);
    }
    for(const auto & cond : cond_seq) {
      cond.delegate()->remove_waitset(this);
    }

    org::eclipse::cyclonedds::core::DDScObjectDelegate::close();
}

org::eclipse::cyclonedds::core::cond::WaitSetDelegate::ConditionSeq&
org::eclipse::cyclonedds::core::cond::WaitSetDelegate::wait(
    ConditionSeq& triggered,
    const dds::core::Duration& timeout)
{
    dds_duration_t c_timeout = org::eclipse::cyclonedds::core::convertDuration(timeout);
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    const size_t sz = conditions_.size();
    scopedLock.unlock();
    dds_attach_t * attach = new dds_attach_t[sz];
    memset (attach, 0, sizeof(dds_attach_t) * sz);

    dds_return_t n_triggered = dds_waitset_wait(this->get_ddsc_entity(), attach, sz, c_timeout);

    if (n_triggered == 0) {
        delete[] attach;
        ISOCPP_THROW_EXCEPTION(ISOCPP_TIMEOUT_ERROR, "dds::core::cond::WaitSet::wait() timed out.");
    } else if (n_triggered > 0) {
        const size_t nt = size_t(n_triggered);
        triggered.reserve(nt);

        for (size_t i = 0; i < nt && i < sz; i++) {
            org::eclipse::cyclonedds::core::cond::ConditionDelegate *cd =
                reinterpret_cast <org::eclipse::cyclonedds::core::cond::ConditionDelegate *>(attach[i]);
            assert(cd);
            cd->dispatch();
            triggered.push_back(cd->wrapper());
        }
        delete[] attach;
    } else {
        delete[] attach;
        ISOCPP_DDSC_RESULT_CHECK_AND_THROW(n_triggered, "dds_waitset_wait failed");
    }

    return triggered;
}

void
org::eclipse::cyclonedds::core::cond::WaitSetDelegate::dispatch(
    const dds::core::Duration& timeout)
{
    ConditionSeq triggered;
    try {
        wait(triggered, timeout);
    }
    catch(dds::core::TimeoutError &) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_TIMEOUT_ERROR,
            "dds::core::cond::WaitSet::dispatch() timed out.");
    }
    catch(...) {
        throw;
    }
    for (ConditionSeq::iterator it = triggered.begin(); it != triggered.end(); ++it) {
        it->dispatch();
    }
}

void
org::eclipse::cyclonedds::core::cond::WaitSetDelegate::attach_condition(
        const dds::core::cond::Condition & cond)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    cond.delegate()->add_waitset(cond, this);
}

bool
org::eclipse::cyclonedds::core::cond::WaitSetDelegate::detach_condition(
        org::eclipse::cyclonedds::core::cond::ConditionDelegate * cond)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    return cond->remove_waitset(this);
}

// this will be called when holding the lock
void
org::eclipse::cyclonedds::core::cond::WaitSetDelegate::add_condition_locked(
    const dds::core::cond::Condition& cond)
{
  dds_return_t ret;
  org::eclipse::cyclonedds::core::cond::ConditionDelegate *cond_delegate;
  ConstConditionIterator cond_it;

  cond_delegate = cond.delegate().get();

  // Check if condition not already added (in accordance with specification
  // adding a Condition that is already attached to the WaitSet has no effect)
  cond_it = conditions_.find(cond_delegate);
  if (cond_it == conditions_.end()) {
    ret = dds_waitset_attach(this->ddsc_entity,
                             cond.delegate()->get_ddsc_entity(),
                             reinterpret_cast<dds_attach_t>(cond_delegate));
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Failed to attach condition");

    conditions_.insert(ConditionEntry(cond_delegate, cond));
  }
}

// this will be called when holding the lock
void
org::eclipse::cyclonedds::core::cond::WaitSetDelegate::remove_condition_locked(
    org::eclipse::cyclonedds::core::cond::ConditionDelegate *cond,
    const dds_entity_t entity_handle)
{
  dds_return_t ret;
  ConstConditionIterator cond_it;
  // Check if condition was added (in accordance with specification
  // this function returns false if condition was not attached)
  cond_it = conditions_.find(cond);
  if (cond_it != conditions_.end()) {
    ret = dds_waitset_detach(
        this->ddsc_entity,
        (entity_handle == DDS_HANDLE_NIL) ? cond->get_ddsc_entity() : entity_handle);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Failed to detach condition");
    conditions_.erase(cond);
  }
}

org::eclipse::cyclonedds::core::cond::WaitSetDelegate::ConditionSeq&
org::eclipse::cyclonedds::core::cond::WaitSetDelegate::conditions(
    ConditionSeq& conds) const
{
    ConstConditionIterator it;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    // TODO: Get ddsc conditions for this waitset to check if entries in
    // this->conditions_ list are still valid. e.g. in case of a closed reader
    // the ddsc condition entities on the reader should be removed from the ddsc
    // waitset. Currently this does not work on ddsc, see the test case in the
    // WaitSet test-set.
    //
    // size_t size = this->conditions_.size();
    // dds_entity_t * entities = new dds_entity_t[size];
    // dds_return_t ret = dds_waitset_get_entities(this->ddsc_entity, entities, size);
    // todo: check this->conditions_ entries with entities
    // delete [] entities;

    conds.clear();
    for (it = this->conditions_.begin(); it != this->conditions_.end(); ++it) {
        conds.push_back(it->second);
    }

    return conds;
}

