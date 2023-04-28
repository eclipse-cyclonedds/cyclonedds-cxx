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

#include <dds/core/detail/conformance.hpp>
#include <dds/core/policy/CorePolicy.hpp>


OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::UserData,            UserData)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::Durability,          Durability)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::Presentation,        Presentation)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::Deadline,            Deadline)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::LatencyBudget,       LatencyBudget)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::TimeBasedFilter,     TimeBasedFilter)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::Ownership,           Ownership)
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::OwnershipStrength,   OwnershipStrength)
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::Liveliness,          Liveliness)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::Partition,           Partition)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::Reliability,         Reliability)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::DestinationOrder,    DestinationOrder)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::History,             History)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::ResourceLimits,      ResourceLimits)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::EntityFactory,       EntityFactory)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::WriterDataLifecycle, WriterDataLifecycle)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::ReaderDataLifecycle, ReaderDataLifecycle)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::TopicData,           TopicData)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::GroupData,           GroupData)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::TransportPriority,   TransportPriority)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::Lifespan,            Lifespan)
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::DataRepresentation,  DataRepresentation)
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::TypeConsistencyEnforcement, TypeConsistencyEnforcement)
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
OMG_DDS_DEFINE_POLICY_TRAITS(dds::core::policy::DurabilityService,   DurabilityService)
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
