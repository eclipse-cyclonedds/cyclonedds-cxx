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

#include <dds/domain/find.hpp>

dds::domain::DomainParticipant
dds::domain::find(uint32_t id)
{
    dds::domain::DomainParticipant participant = dds::core::null;

    participant = dds::domain::DomainParticipant(
            org::eclipse::cyclonedds::domain::DomainParticipantDelegate::lookup_participant(id));

    return participant;
}
