// Copyright(c) 2006 to 2021 ZettaScale Technology and others
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

#include <org/eclipse/cyclonedds/core/InstanceHandleDelegate.hpp>

#include "dds/dds.h"

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace core
{

InstanceHandleDelegate::InstanceHandleDelegate() : handle_(DDS_HANDLE_NIL)
{
    // empty
}

InstanceHandleDelegate::InstanceHandleDelegate(dds_instance_handle_t h) : handle_(h)
{
}

InstanceHandleDelegate::InstanceHandleDelegate(const dds::core::null_type& src)
    : handle_(DDS_HANDLE_NIL)
{
    (void)src;
}

dds_instance_handle_t
InstanceHandleDelegate::handle() const
{
    return handle_;
}


InstanceHandleDelegate&
InstanceHandleDelegate::operator=(const dds::core::null_type& src)
{
    (void)src;
    handle_ = DDS_HANDLE_NIL;
    return *this;
}

bool
InstanceHandleDelegate::is_nil() const
{
    return (handle_ == DDS_HANDLE_NIL);
}

bool
InstanceHandleDelegate::operator==(const InstanceHandleDelegate& that) const
{
    return (this->handle_ == that.handle_);
}

bool
InstanceHandleDelegate::operator<(const InstanceHandleDelegate& that) const
{
    return (this->handle_ < that.handle_);
}

bool
InstanceHandleDelegate::operator>(const InstanceHandleDelegate& that) const
{
    return (this->handle_ > that.handle_);
}

}
}
}
}
