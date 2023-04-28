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

#include <org/eclipse/cyclonedds/topic/TopicDescriptionDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace topic
{

TopicDescriptionDelegate::TopicDescriptionDelegate(
        const dds::domain::DomainParticipant& dp,
        const std::string& name,
        const std::string& type_name)
    : myParticipant(dp),
      myTopicName(name),
      myTypeName(type_name),
      nrDependents(0),
      ser_type_(NULL)
{
}

TopicDescriptionDelegate::~TopicDescriptionDelegate()
{
}

const std::string&
TopicDescriptionDelegate::name() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return myTopicName;
}

const std::string&
TopicDescriptionDelegate::type_name() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return myTypeName;
}

const dds::domain::DomainParticipant&
TopicDescriptionDelegate::domain_participant() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return myParticipant;
}

void
TopicDescriptionDelegate::incrNrDependents()
{
    nrDependents++;
}

void
TopicDescriptionDelegate::decrNrDependents()
{
    nrDependents--;
}

bool
TopicDescriptionDelegate::hasDependents() const
{
    return (nrDependents > 0);
}

ddsi_sertype *
TopicDescriptionDelegate::get_ser_type() const
{
    return ser_type_;
}


}
}
}
}
