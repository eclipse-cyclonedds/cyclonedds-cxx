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


#include <dds/sub/find.hpp>

namespace dds
{
namespace sub
{

const Subscriber
builtin_subscriber(const dds::domain::DomainParticipant& dp)
{
    (void)dp;
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
#if 0
    org::eclipse::cyclonedds::sub::SubscriberDelegate::ref_type ref =
            org::eclipse::cyclonedds::sub::BuiltinSubscriberDelegate::get_builtin_subscriber(dp);
    return Subscriber(ref);
#else
    return Subscriber(dds::core::null);
#endif
}

}
}
