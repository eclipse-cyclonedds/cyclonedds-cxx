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

#include <org/eclipse/cyclonedds/core/EntitySet.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>

void
org::eclipse::cyclonedds::core::EntitySet::insert(org::eclipse::cyclonedds::core::EntityDelegate& entity)
{
    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(this->mutex);
    this->entities.insert(entity.get_weak_ref());
}

void
org::eclipse::cyclonedds::core::EntitySet::erase(org::eclipse::cyclonedds::core::EntityDelegate& entity)
{
    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(this->mutex);
    this->entities.erase(entity.get_weak_ref());
}

bool
org::eclipse::cyclonedds::core::EntitySet::contains(const dds::core::InstanceHandle& handle)
{
    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(this->mutex);
    bool contains = false;

    WeakReferenceSet<ObjectDelegate::weak_ref_type>::iterator it;

    for (it = this->entities.begin(); !contains && (it != this->entities.end()); it++) {
        org::eclipse::cyclonedds::core::ObjectDelegate::ref_type ref = it->lock();
        if (ref) {
            org::eclipse::cyclonedds::core::EntityDelegate::ref_type entity =
                    ::std::dynamic_pointer_cast<EntityDelegate>(ref);

            /* Check if current entity is the one that is searched for. */
            contains = (entity->instance_handle() == handle);
            if (!contains) {
                /* Check if current entity contains the one searched for. */
                contains = entity->contains_entity(handle);
            }
        }
    }

    return contains;
}

void
org::eclipse::cyclonedds::core::EntitySet::all_close()
{
    /* Copy the entities to use them outside the lock. */
    vector vctr = this->copy();
    /* Call close() of all Entities. */
    for (vectorIterator it = vctr.begin(); it != vctr.end(); ++it) {
        org::eclipse::cyclonedds::core::ObjectDelegate::ref_type ref = it->lock();
        if (ref) {
            ::std::dynamic_pointer_cast<EntityDelegate>(ref).get()->close();
        }
    }
}

void
org::eclipse::cyclonedds::core::EntitySet::all_retain()
{
    /* Copy the entities to use them outside the lock. */
    vector vctr = this->copy();
    /* Call retain() of all Entities. */
    for (vectorIterator it = vctr.begin(); it != vctr.end(); ++it) {
        org::eclipse::cyclonedds::core::ObjectDelegate::ref_type ref = it->lock();
         if (ref) {
             ::std::dynamic_pointer_cast<EntityDelegate>(ref).get()->retain();
         }
    }
}

void
org::eclipse::cyclonedds::core::EntitySet::all_enable()
{
    /* Copy the entities to use them outside the lock. */
    vector vctr = this->copy();
    /* Call enable() of all Entities. */
    for (vectorIterator it = vctr.begin(); it != vctr.end(); ++it) {
        org::eclipse::cyclonedds::core::ObjectDelegate::ref_type ref = it->lock();
         if (ref) {
             ::std::dynamic_pointer_cast<EntityDelegate>(ref).get()->close();
         }
    }
}

org::eclipse::cyclonedds::core::EntitySet::vector
org::eclipse::cyclonedds::core::EntitySet::copy()
{
    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(this->mutex);
    vector vctr(this->entities.size());
    std::copy(this->entities.begin(), this->entities.end(), vctr.begin());
    return vctr;
}
