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

#include <org/eclipse/cyclonedds/core/Mutex.hpp>
#include "dds/dds.h"
#include "dds/ddsrt/sync.h"

org::eclipse::cyclonedds::core::Mutex::Mutex()
{
  mtx = dds_alloc (sizeof (ddsrt_mutex_t));
  ddsrt_mutex_init ((ddsrt_mutex_t*)this->mtx);
}

org::eclipse::cyclonedds::core::Mutex::~Mutex()
{
  ddsrt_mutex_destroy ((ddsrt_mutex_t*)this->mtx);
  dds_free (mtx);
}

void org::eclipse::cyclonedds::core::Mutex::lock() const
{
  ddsrt_mutex_lock ((ddsrt_mutex_t*) this->mtx);
}

bool org::eclipse::cyclonedds::core::Mutex::try_lock () const
{
  return (ddsrt_mutex_trylock ((ddsrt_mutex_t*) this->mtx));
}

void org::eclipse::cyclonedds::core::Mutex::unlock() const
{
  ddsrt_mutex_unlock ((ddsrt_mutex_t*) this->mtx);
}
