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
#include <org/eclipse/cyclonedds/pub/qos/PublisherQosDelegate.hpp>

#include <cassert>

#include "dds/ddsi/ddsi_plist.h"

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace pub
{
namespace qos
{

PublisherQosDelegate::PublisherQosDelegate()
{
    ddsc_qos(&ddsi_default_qos_publisher_subscriber);
    check();
}

void
PublisherQosDelegate::policy(const dds::core::policy::Presentation& presentation)
{
    presentation.delegate().check();
    present_ |= DDSI_QP_PRESENTATION;
    presentation_ = presentation;
}

void
PublisherQosDelegate::policy(const dds::core::policy::Partition& partition)
{
    partition.delegate().check();
    present_ |= DDSI_QP_PARTITION;
    partition_ = partition;
}

void
PublisherQosDelegate::policy(const dds::core::policy::GroupData& gdata)
{
    gdata.delegate().check();
    present_ |= DDSI_QP_GROUP_DATA;
    gdata_ = gdata;
}

void
PublisherQosDelegate::policy(const dds::core::policy::EntityFactory& factory_policy)
{
    factory_policy.delegate().check();
    present_ |= DDSI_QP_ADLINK_ENTITY_FACTORY;
    factory_policy_ = factory_policy;
}

dds_qos_t*
PublisherQosDelegate::ddsc_qos() const
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
        gdata_          .delegate().set_c_policy(qos);
    if (present_ & DDSI_QP_ADLINK_ENTITY_FACTORY)
        factory_policy_ .delegate().set_c_policy(qos);
    return qos;
}

void
PublisherQosDelegate::ddsc_qos(const dds_qos_t* qos)
{
    assert(qos);
    present_ = qos->present;
    if (present_ & DDSI_QP_PRESENTATION)
        presentation_   .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_PARTITION)
        partition_      .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_GROUP_DATA)
        gdata_          .delegate().set_iso_policy(qos);
    if (present_ & DDSI_QP_ADLINK_ENTITY_FACTORY)
        factory_policy_ .delegate().set_iso_policy(qos);
}

void
PublisherQosDelegate::named_qos(const struct _DDS_NamedPublisherQos &qos)
{
    (void)qos;
#if 0
    /* We only need the QoS part of the named QoS. */
    const struct _DDS_PublisherQos *q = &qos.publisher_qos;
    /* The idl policies are aligned the same as the ddsc/builtin representation.
     * So, cast and use the ddsc policy translations (or builtin when available). */
    presentation_   .delegate().v_policy((v_presentationPolicy&)    (q->presentation)  );
    partition_      .delegate().v_policy((v_builtinPartitionPolicy&)(q->partition)     );
    gdata_          .delegate().v_policy((v_builtinGroupDataPolicy&)(q->group_data)    );
    factory_policy_ .delegate().v_policy((v_entityFactoryPolicy&)   (q->entity_factory));
#endif
}

void
PublisherQosDelegate::check() const
{
    /* Policies are checked when set.
     * No consistency check between policies needed. */
}

bool
PublisherQosDelegate::operator ==(const PublisherQosDelegate& other) const
{
    return other.present_        == present_ &&
           other.presentation_   == presentation_ &&
           other.partition_      == partition_    &&
           other.gdata_          == gdata_        &&
           other.factory_policy_ == factory_policy_;
}

template<>
dds::core::policy::Presentation&
PublisherQosDelegate::policy<dds::core::policy::Presentation>()
{
    present_ |= DDSI_QP_PRESENTATION;
    return presentation_;
}

template<>
dds::core::policy::Partition&
PublisherQosDelegate::policy<dds::core::policy::Partition>()
{
    present_ |= DDSI_QP_PARTITION;
    return partition_;
}

template<>
dds::core::policy::GroupData&
PublisherQosDelegate::policy<dds::core::policy::GroupData>()
{
    present_ |= DDSI_QP_GROUP_DATA;
    return gdata_;
}

template<>
dds::core::policy::EntityFactory&
PublisherQosDelegate::policy<dds::core::policy::EntityFactory>()
{
    present_ |= DDSI_QP_ADLINK_ENTITY_FACTORY;
    return factory_policy_;
}

}
}
}
}
}
