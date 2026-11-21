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
#include "org/eclipse/cyclonedds/core/ObjectDelegate.hpp"
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>

org::eclipse::cyclonedds::core::ObjectDelegate::ObjectDelegate ()
{
}

org::eclipse::cyclonedds::core::ObjectDelegate::~ObjectDelegate ()
{
}

void org::eclipse::cyclonedds::core::ObjectDelegate::check () const
{
  if (closed.load()) {
    ISOCPP_THROW_EXCEPTION (ISOCPP_ALREADY_CLOSED_ERROR, "Trying to invoke an oparation on an object that was already closed");
  }
}

void org::eclipse::cyclonedds::core::ObjectDelegate::lock () const
{
  check();
  this->mutex.lock ();
}

void org::eclipse::cyclonedds::core::ObjectDelegate::unlock () const
{
  this->mutex.unlock ();
}

bool org::eclipse::cyclonedds::core::ObjectDelegate::is_valid() const
{
  bool is_closed = this->closed.load(std::memory_order_acquire);
  return !is_closed;
}

void org::eclipse::cyclonedds::core::ObjectDelegate::close ()
{
  this->closed.store(true);
}

void org::eclipse::cyclonedds::core::ObjectDelegate::set_weak_ref (const ObjectDelegate::weak_ref_type &weak_ref)
{
  this->myself = weak_ref;
}

org::eclipse::cyclonedds::core::ObjectDelegate::weak_ref_type
org::eclipse::cyclonedds::core::ObjectDelegate::get_weak_ref () const
{
  return this->myself;
}

org::eclipse::cyclonedds::core::ObjectDelegate::ref_type
org::eclipse::cyclonedds::core::ObjectDelegate::get_strong_ref () const
{
  return this->myself.lock ();
}
