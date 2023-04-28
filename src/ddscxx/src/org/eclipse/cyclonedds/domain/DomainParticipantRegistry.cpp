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

#include <org/eclipse/cyclonedds/domain/DomainParticipantRegistry.hpp>


org::eclipse::cyclonedds::core::EntityRegistry
  <org::eclipse::cyclonedds::domain::DomainParticipantDelegate *,
   dds::domain::TDomainParticipant<org::eclipse::cyclonedds::domain::DomainParticipantDelegate> > org::eclipse::cyclonedds::domain::DomainParticipantRegistry::registry;


void
org::eclipse::cyclonedds::domain::DomainParticipantRegistry::insert(
    dds::domain::TDomainParticipant<org::eclipse::cyclonedds::domain::DomainParticipantDelegate>& participant)
{
    registry.insert(participant.delegate().get(), participant);
}

void
org::eclipse::cyclonedds::domain::DomainParticipantRegistry::remove(
    org::eclipse::cyclonedds::domain::DomainParticipantDelegate *delegate)
{
    registry.remove(delegate);
}
