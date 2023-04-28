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

#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/sub/qos/SubscriberQosDelegate.hpp>

#include <cassert>

#include "dds/ddsi/ddsi_plist.h"

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace sub
{
namespace qos
{

SubscriberQosDelegate::SubscriberQosDelegate()
{
    ddsc_qos(&ddsi_default_qos_publisher_subscriber);
    check();
}

void
SubscriberQosDelegate::policy(const dds::core::policy::Presentation& presentation)
{
    presentation.delegate().check();
    present_ |= DDSI_QP_PRESENTATION;
    presentation_ = presentation;
}

void
SubscriberQosDelegate::policy(const dds::core::policy::Partition& partition)
{
    partition.delegate().check();
    present_ |= DDSI_QP_PARTITION;
    partition_ = partition;
}

void
SubscriberQosDelegate::policy(const dds::core::policy::GroupData& group_data)
{
    group_data.delegate().check();
    present_ |= DDSI_QP_GROUP_DATA;
    group_data_ = group_data;
}

void
SubscriberQosDelegate::policy(const dds::core::policy::EntityFactory& entity_factory)
{
    entity_factory.delegate().check();
    present_ |= DDSI_QP_ADLINK_ENTITY_FACTORY;
    entity_factory_ = entity_factory;
}

dds_qos_t*
SubscriberQosDelegate::ddsc_qos() const
{
    dds_qos_t* qos = dds_create_qos();
    if (!qos) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_OUT_OF_RESOURCES_ERROR, "Could not create internal QoS.");
    }
    if (present_ & DDSI_QP_PRESENTATION)
        presentation_   .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_PARTITION)
        partition_      .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_GROUP_DATA)
        group_data_     .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_ADLINK_ENTITY_FACTORY)
        entity_factory_ .delegate().set_c_policy(qos);
    return qos;
}

void
SubscriberQosDelegate::ddsc_qos(const dds_qos_t* qos)
{
    assert(qos);
    present_ = qos->present;
    if (present_ & DDSI_QP_PRESENTATION)
        presentation_   .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_PARTITION)
        partition_      .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_GROUP_DATA)
        group_data_     .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_ADLINK_ENTITY_FACTORY)
        entity_factory_ .delegate().set_iso_policy(qos);
}

void
SubscriberQosDelegate::named_qos(const struct _DDS_NamedSubscriberQos &qos)
{
    (void)qos;
#if 0
    /* We only need the QoS part of the named QoS. */
    const struct _DDS_SubscriberQos *q = &qos.subscriber_qos;
    /* The idl policies are aligned the same as the ddsc/builtin representation.
     * So, cast and use the ddsc policy translations (or builtin when available). */
    presentation_   .delegate().v_policy((v_presentationPolicy&)    (q->presentation)  );
    partition_      .delegate().v_policy((v_builtinPartitionPolicy&)(q->partition)     );
    group_data_     .delegate().v_policy((v_builtinGroupDataPolicy&)(q->group_data)    );
    entity_factory_ .delegate().v_policy((v_entityFactoryPolicy&)   (q->entity_factory));
#endif
}

void
SubscriberQosDelegate::check() const
{
    /* Policies are checked when set.
     * No consistency check between policies needed. */
}

bool
SubscriberQosDelegate::operator ==(const SubscriberQosDelegate& other) const
{
    return other.present_        == present_        &&
           other.presentation_   == presentation_   &&
           other.partition_      == partition_      &&
           other.group_data_     == group_data_     &&
           other.entity_factory_ == entity_factory_;
}

template<>
dds::core::policy::Presentation&
SubscriberQosDelegate::policy<dds::core::policy::Presentation>()
{
    present_ |= DDSI_QP_PRESENTATION;
    return presentation_;
}

template<>
dds::core::policy::Partition&
SubscriberQosDelegate::policy<dds::core::policy::Partition>()
{
    present_ |= DDSI_QP_PARTITION;
    return partition_;
}

template<>
dds::core::policy::GroupData&
SubscriberQosDelegate::policy<dds::core::policy::GroupData>()
{
    present_ |= DDSI_QP_GROUP_DATA;
    return group_data_;
}

template<>
dds::core::policy::EntityFactory&
SubscriberQosDelegate::policy<dds::core::policy::EntityFactory>()
{
    present_ |= DDSI_QP_ADLINK_ENTITY_FACTORY;
    return entity_factory_;
}

}
}
}
}
}
