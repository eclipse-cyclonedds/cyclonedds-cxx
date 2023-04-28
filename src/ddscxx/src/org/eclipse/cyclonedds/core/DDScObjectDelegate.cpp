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

#include <assert.h>

#include <org/eclipse/cyclonedds/core/DDScObjectDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include "org/eclipse/cyclonedds/core/Mutex.hpp"

org::eclipse::cyclonedds::core::DDScObjectDelegate::entity_map_type
org::eclipse::cyclonedds::core::DDScObjectDelegate::entity_map;

org::eclipse::cyclonedds::core::Mutex org::eclipse::cyclonedds::core::DDScObjectDelegate::entity_map_mutex;

org::eclipse::cyclonedds::core::DDScObjectDelegate::DDScObjectDelegate () :
    ddsc_entity(0)
{
}

org::eclipse::cyclonedds::core::DDScObjectDelegate::~DDScObjectDelegate ()
{
    delete_from_entity_map();

    if (this->ddsc_entity > 0) {
        dds_delete(this->ddsc_entity);
        this->ddsc_entity = 0;
    }
}

void
org::eclipse::cyclonedds::core::DDScObjectDelegate::close ()
{
    org::eclipse::cyclonedds::core::ObjectDelegate::close();
}

dds_entity_t
org::eclipse::cyclonedds::core::DDScObjectDelegate::get_ddsc_entity ()
{
    dds_entity_t handle;

    this->lock();
    handle = this->ddsc_entity;
    this->unlock();

    return handle;
}

void
org::eclipse::cyclonedds::core::DDScObjectDelegate::set_ddsc_entity (dds_entity_t e)
{
    this->lock();
    this->ddsc_entity = e;
    this->unlock();
}

void
org::eclipse::cyclonedds::core::DDScObjectDelegate::add_to_entity_map(org::eclipse::cyclonedds::core::ObjectDelegate::weak_ref_type weak_ref)
{
    // can be used without lock; only called from wrapper function constructor

    DDScObjectDelegate::entity_map_mutex.lock ();
    DDScObjectDelegate::entity_map[this->ddsc_entity] = weak_ref;

#ifndef NDEBUG
    DDScObjectDelegate::entity_map_type::iterator it = DDScObjectDelegate::entity_map.find(this->ddsc_entity);

    assert(it != DDScObjectDelegate::entity_map.end());
#endif

    DDScObjectDelegate::entity_map_mutex.unlock ();
}

void
org::eclipse::cyclonedds::core::DDScObjectDelegate::delete_from_entity_map()
{
    // can be used without lock; only called from wrapper function destructor

    if (this->ddsc_entity > 0) {
        DDScObjectDelegate::entity_map_mutex.lock ();
        DDScObjectDelegate::entity_map_type::iterator it = DDScObjectDelegate::entity_map.find(this->ddsc_entity);
        if (it != DDScObjectDelegate::entity_map.end()) {
            DDScObjectDelegate::entity_map.erase(it);
        }
        DDScObjectDelegate::entity_map_mutex.unlock ();
    }
}

org::eclipse::cyclonedds::core::ObjectDelegate::ref_type
org::eclipse::cyclonedds::core::DDScObjectDelegate::extract_strong_ref (dds_entity_t e)
{
    org::eclipse::cyclonedds::core::ObjectDelegate::weak_ref_type e_ptr;

    DDScObjectDelegate::entity_map_mutex.lock ();
    DDScObjectDelegate::entity_map_type::iterator it = DDScObjectDelegate::entity_map.find(e);

    assert (it != DDScObjectDelegate::entity_map.end());

    e_ptr = it->second;

    DDScObjectDelegate::entity_map_mutex.unlock ();

    // coverity[return_local_addr_alias:FALSE]
    return e_ptr.lock();
}

