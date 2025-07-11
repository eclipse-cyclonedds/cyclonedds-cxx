// Copyright(c) 2006 to 2021 ZettaScale Technology and others
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

#ifndef CYCLONEDDS_PUB_QOS_DATA_WRITER_QOS_DELEGATE_HPP_
#define CYCLONEDDS_PUB_QOS_DATA_WRITER_QOS_DELEGATE_HPP_

#include <dds/core/detail/conformance.hpp>
#include <org/eclipse/cyclonedds/topic/qos/TopicQosDelegate.hpp>

struct _DDS_NamedDataWriterQos;

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

class OMG_DDS_API DataWriterQosDelegate
{
public:
    DataWriterQosDelegate();
    DataWriterQosDelegate(const org::eclipse::cyclonedds::topic::qos::TopicQosDelegate& tqos);

    void policy(const dds::core::policy::UserData&          user_data);
    void policy(const dds::core::policy::Durability&        durability);
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    void policy(const dds::core::policy::DurabilityService& durability_service);
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
    void policy(const dds::core::policy::Deadline&          deadline);
    void policy(const dds::core::policy::LatencyBudget&     budget);
    void policy(const dds::core::policy::Liveliness&        liveliness);
    void policy(const dds::core::policy::Reliability&       reliability);
    void policy(const dds::core::policy::DestinationOrder&  order);
    void policy(const dds::core::policy::History&           history);
    void policy(const dds::core::policy::ResourceLimits&    resources);
    void policy(const dds::core::policy::TransportPriority& priority);
    void policy(const dds::core::policy::Lifespan&          lifespan);
    void policy(const dds::core::policy::Ownership&         ownership);
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
    void policy(const dds::core::policy::OwnershipStrength& strength);
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
    void policy(const dds::core::policy::WriterDataLifecycle&   lifecycle);
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    void policy(const dds::core::policy::DataRepresentation& datarepresentation);
    void policy(const dds::core::policy::TypeConsistencyEnforcement& typeconsistencyenforcement);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    void policy(const dds::core::policy::WriterBatching&    writerbatching);
    void policy(const dds::core::policy::PSMXInstances&     psmxinstances);
    void policy(const dds::core::policy::IgnoreLocal& ignorelocal);

    template <typename POLICY> const POLICY& policy() const;
    template <typename POLICY> POLICY& policy();

    /* The returned ddsc QoS has to be freed. */
    dds_qos_t* ddsc_qos() const;
    void ddsc_qos(const dds_qos_t* qos, bool copy_flags);

    void named_qos(const struct _DDS_NamedDataWriterQos &qos);

    void check() const;

    bool operator ==(const DataWriterQosDelegate& other) const;
    DataWriterQosDelegate& operator =(const org::eclipse::cyclonedds::topic::qos::TopicQosDelegate& tqos);

    const uint64_t &present() const {return present_;}
    uint64_t &present() {return present_;}
private:
    uint64_t                                   present_ = 0;
    dds::core::policy::UserData                user_data_;
    dds::core::policy::Durability              durability_;
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    dds::core::policy::DurabilityService       durability_service_;
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
    dds::core::policy::Deadline                deadline_;
    dds::core::policy::LatencyBudget           budget_;
    dds::core::policy::Liveliness              liveliness_;
    dds::core::policy::Reliability             reliability_;
    dds::core::policy::DestinationOrder        order_;
    dds::core::policy::History                 history_;
    dds::core::policy::ResourceLimits          resources_;
    dds::core::policy::TransportPriority       priority_;
    dds::core::policy::Lifespan                lifespan_;
    dds::core::policy::Ownership               ownership_;
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
    dds::core::policy::OwnershipStrength       strength_;
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
    dds::core::policy::WriterDataLifecycle     lifecycle_;
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    dds::core::policy::DataRepresentation      datarepresentation_;
    dds::core::policy::TypeConsistencyEnforcement typeconsistencyenforcement_;
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    dds::core::policy::WriterBatching          writerbatching_;
    dds::core::policy::PSMXInstances           psmxinstances_;
    dds::core::policy::IgnoreLocal             ignorelocal_;
};



//==============================================================================

template<> inline const dds::core::policy::UserData&
DataWriterQosDelegate::policy<dds::core::policy::UserData>() const
{
    return user_data_;
}

template<> OMG_DDS_API dds::core::policy::UserData&
DataWriterQosDelegate::policy<dds::core::policy::UserData>();

template<> inline const dds::core::policy::Durability&
DataWriterQosDelegate::policy<dds::core::policy::Durability>() const
{
    return durability_;
}

template<> OMG_DDS_API dds::core::policy::Durability&
DataWriterQosDelegate::policy<dds::core::policy::Durability>();

#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
template<> inline const dds::core::policy::DurabilityService&
DataWriterQosDelegate::policy<dds::core::policy::DurabilityService>() const
{
    return durability_service_;
}

template<> OMG_DDS_API dds::core::policy::DurabilityService&
DataWriterQosDelegate::policy<dds::core::policy::DurabilityService>();
#endif  // OMG_DDS_PERSISTENCE_SUPPORT

template<> inline const dds::core::policy::Deadline&
DataWriterQosDelegate::policy<dds::core::policy::Deadline>() const
{
    return deadline_;
}

template<> OMG_DDS_API dds::core::policy::Deadline&
DataWriterQosDelegate::policy<dds::core::policy::Deadline>();

template<> inline const dds::core::policy::LatencyBudget&
DataWriterQosDelegate::policy<dds::core::policy::LatencyBudget>() const
{
    return budget_;
}

template<> OMG_DDS_API dds::core::policy::LatencyBudget&
DataWriterQosDelegate::policy<dds::core::policy::LatencyBudget>();


template<> inline const dds::core::policy::Liveliness&
DataWriterQosDelegate::policy<dds::core::policy::Liveliness>() const
{
    return liveliness_;
}

template<> OMG_DDS_API dds::core::policy::Liveliness&
DataWriterQosDelegate::policy<dds::core::policy::Liveliness>();

template<> inline const dds::core::policy::Reliability&
DataWriterQosDelegate::policy<dds::core::policy::Reliability>() const
{
    return reliability_;
}

template<> OMG_DDS_API dds::core::policy::Reliability&
DataWriterQosDelegate::policy<dds::core::policy::Reliability>();

template<> inline const dds::core::policy::DestinationOrder&
DataWriterQosDelegate::policy<dds::core::policy::DestinationOrder>() const
{
    return order_;
}

template<> OMG_DDS_API dds::core::policy::DestinationOrder&
DataWriterQosDelegate::policy<dds::core::policy::DestinationOrder>();

template<> inline const dds::core::policy::History&
DataWriterQosDelegate::policy<dds::core::policy::History>() const
{
    return history_;
}

template<> OMG_DDS_API dds::core::policy::History&
DataWriterQosDelegate::policy<dds::core::policy::History>();


template<> inline const dds::core::policy::ResourceLimits&
DataWriterQosDelegate::policy<dds::core::policy::ResourceLimits>() const
{
    return resources_;
}

template<> OMG_DDS_API dds::core::policy::ResourceLimits&
DataWriterQosDelegate::policy<dds::core::policy::ResourceLimits>();

template<> inline const dds::core::policy::TransportPriority&
DataWriterQosDelegate::policy<dds::core::policy::TransportPriority>() const
{
    return priority_;
}

template<> OMG_DDS_API dds::core::policy::TransportPriority&
DataWriterQosDelegate::policy<dds::core::policy::TransportPriority>();

template<> inline const dds::core::policy::Lifespan&
DataWriterQosDelegate::policy<dds::core::policy::Lifespan>() const
{
    return lifespan_;
}

template<> OMG_DDS_API dds::core::policy::Lifespan&
DataWriterQosDelegate::policy<dds::core::policy::Lifespan>();

template<> inline const dds::core::policy::Ownership&
DataWriterQosDelegate::policy<dds::core::policy::Ownership>() const
{
    return ownership_;
}

template<> OMG_DDS_API dds::core::policy::Ownership&
DataWriterQosDelegate::policy<dds::core::policy::Ownership>();

#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
template<> inline const dds::core::policy::OwnershipStrength&
DataWriterQosDelegate::policy<dds::core::policy::OwnershipStrength>() const
{
    return strength_;
}

template<> OMG_DDS_API dds::core::policy::OwnershipStrength&
DataWriterQosDelegate::policy<dds::core::policy::OwnershipStrength>();
#endif  // OMG_DDS_OWNERSHIP_SUPPORT

template<> inline const dds::core::policy::WriterDataLifecycle&
DataWriterQosDelegate::policy<dds::core::policy::WriterDataLifecycle>() const
{
    return lifecycle_;
}

template<> OMG_DDS_API dds::core::policy::WriterDataLifecycle&
DataWriterQosDelegate::policy<dds::core::policy::WriterDataLifecycle>();

#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

template<> inline const  dds::core::policy::DataRepresentation&
DataWriterQosDelegate::policy<dds::core::policy::DataRepresentation>() const
{
    return datarepresentation_;
}

template<> OMG_DDS_API dds::core::policy::DataRepresentation&
DataWriterQosDelegate::policy<dds::core::policy::DataRepresentation>();

template<> inline const  dds::core::policy::TypeConsistencyEnforcement&
DataWriterQosDelegate::policy<dds::core::policy::TypeConsistencyEnforcement>() const
{
    return typeconsistencyenforcement_;
}

template<> OMG_DDS_API dds::core::policy::TypeConsistencyEnforcement&
DataWriterQosDelegate::policy<dds::core::policy::TypeConsistencyEnforcement>();
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

template<> inline const  dds::core::policy::WriterBatching&
DataWriterQosDelegate::policy<dds::core::policy::WriterBatching>() const
{
    return writerbatching_;
}

template<> OMG_DDS_API dds::core::policy::WriterBatching&
DataWriterQosDelegate::policy<dds::core::policy::WriterBatching>();

template<>
OMG_DDS_API dds::core::policy::PSMXInstances&
DataWriterQosDelegate::policy<dds::core::policy::PSMXInstances>();

template<>
inline const dds::core::policy::PSMXInstances&
DataWriterQosDelegate::policy<dds::core::policy::PSMXInstances>() const
{
    return psmxinstances_;
}

template<> inline const dds::core::policy::IgnoreLocal&
DataWriterQosDelegate::policy<dds::core::policy::IgnoreLocal>() const
{
    return ignorelocal_;
}

template<> OMG_DDS_API dds::core::policy::IgnoreLocal&
DataWriterQosDelegate::policy<dds::core::policy::IgnoreLocal>();


}
}
}
}
}

#endif /* CYCLONEDDS_PUB_QOS_DATA_WRITER_QOS_DELEGATE_HPP_ */
