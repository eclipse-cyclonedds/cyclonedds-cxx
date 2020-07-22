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

#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/domain/qos/DomainParticipantQosDelegate.hpp>

#include <cassert>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace domain
{
namespace qos
{

DomainParticipantQosDelegate::DomainParticipantQosDelegate()
{
}

DomainParticipantQosDelegate::DomainParticipantQosDelegate(
    const DomainParticipantQosDelegate& other)
    :  user_data_(other.user_data_),
       entity_factory_(other.entity_factory_)
{
}

DomainParticipantQosDelegate::~DomainParticipantQosDelegate()
{
}

void
DomainParticipantQosDelegate::policy(const dds::core::policy::UserData& user_data)
{
    user_data.delegate().check();
    user_data_ = user_data;
}

void
DomainParticipantQosDelegate::policy(const dds::core::policy::EntityFactory& entity_factory)
{
    entity_factory.delegate().check();
    entity_factory_ = entity_factory;
}

dds_qos_t*
DomainParticipantQosDelegate::ddsc_qos() const
{
    dds_qos_t* qos = dds_create_qos();
    user_data_.delegate().set_c_policy(qos);
    entity_factory_.delegate().set_c_policy(qos);
    return qos;
}

void
DomainParticipantQosDelegate::ddsc_qos(const dds_qos_t* qos)
{
    assert(qos);
    user_data_.delegate().set_iso_policy(qos);
    entity_factory_.delegate().set_iso_policy(qos);
}

void
DomainParticipantQosDelegate::named_qos(const struct _DDS_NamedDomainParticipantQos &qos)
{
    (void)qos;
#if 0
    /* We only need the QoS part of the named QoS. */
    const struct _DDS_DomainParticipantQos *q = &qos.domainparticipant_qos;
    /* The idl policies are aligned the same as the ddsc/builtin representation.
     * So, cast and use the ddsc policy translations (or builtin when available). */
    user_data_          .delegate().v_policy((v_builtinUserDataPolicy&)(q->user_data          ));
    entity_factory_     .delegate().v_policy((v_entityFactoryPolicy&)  (q->entity_factory     ));
    watchdog_scheduling_.delegate().v_policy((v_schedulePolicy&)       (q->watchdog_scheduling));
    listener_scheduling_.delegate().v_policy((v_schedulePolicy&)       (q->listener_scheduling));
#endif
}

void
DomainParticipantQosDelegate::check() const
{
    /* Policies are checked when set.
     * No consistency check between policies needed. */
}

bool
DomainParticipantQosDelegate::operator ==(const DomainParticipantQosDelegate& other) const
{
    return other.user_data_           == user_data_ &&
           other.entity_factory_      == entity_factory_;

}

DomainParticipantQosDelegate&
DomainParticipantQosDelegate::operator = (const DomainParticipantQosDelegate& other)
{
    user_data_           = other.user_data_;
    entity_factory_      = other.entity_factory_;
    return *this;
}

}
}
}
}
}
