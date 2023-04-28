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


#include <dds/sub/discovery.hpp>

namespace dds
{
namespace sub
{

void ignore(
    const dds::domain::DomainParticipant& dp,
    const dds::core::InstanceHandle& handle)
{
    DDSCXX_UNUSED_ARG(dp);
    DDSCXX_UNUSED_ARG(handle);
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

}
}
