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


#include <dds/sub/find.hpp>

namespace dds
{
namespace sub
{

const Subscriber
builtin_subscriber(const dds::domain::DomainParticipant& dp)
{
    org::eclipse::cyclonedds::sub::SubscriberDelegate::ref_type ref =
            org::eclipse::cyclonedds::sub::BuiltinSubscriberDelegate::get_builtin_subscriber(dp);
    return Subscriber(ref);
}

}
}
