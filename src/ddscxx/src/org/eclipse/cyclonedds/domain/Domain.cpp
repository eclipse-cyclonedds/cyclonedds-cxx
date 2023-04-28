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

#include "org/eclipse/cyclonedds/domain/Domain.hpp"

#include "dds/dds.h"

uint32_t org::eclipse::cyclonedds::domain::any_id()
{
    /* The any_id is basically the same as cyclonedds default domain. */
    return static_cast<uint32_t>(DDS_DOMAIN_DEFAULT);
}

uint32_t org::eclipse::cyclonedds::domain::default_id()
{
    return static_cast<uint32_t>(DDS_DOMAIN_DEFAULT);
}
