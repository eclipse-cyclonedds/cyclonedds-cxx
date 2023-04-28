// Copyright(c) 2006 to 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "dds/dds.hpp"
#include <gtest/gtest.h>

using namespace dds::pub::qos;
using namespace dds::sub::qos;
using namespace dds::topic::qos;
using namespace dds::domain::qos;
using namespace dds::core::policy;
using namespace org::eclipse::cyclonedds::core::policy;



/*
 * Just some fantasy policy values that are different than default.
 */
dds::core::ByteSeq nonDefaultByteSeq( 4, 0xAA );

UserData               nonDefaultUserData(nonDefaultByteSeq);
EntityFactory          nonDefaultEntityFactory(false);
TopicData              nonDefaultTopicData(nonDefaultByteSeq);
Durability             nonDefaultDurability(dds::core::policy::DurabilityKind::TRANSIENT_LOCAL);
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
DurabilityService nonDefaultDurabilityService(dds::core::Duration(10, 10),
                                              dds::core::policy::HistoryKind::KEEP_ALL,
                                              10,  /* depth */
                                              100, /* max_samples */
                                              5,   /* max_instances */
                                              25   /* max_samples_per_instance */);
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
Deadline               nonDefaultDeadline(dds::core::Duration(100, 100));
LatencyBudget          nonDefaultBudget(dds::core::Duration(5));
Liveliness             nonDefaultLiveliness(dds::core::policy::LivelinessKind::MANUAL_BY_PARTICIPANT,
                                            dds::core::Duration(0, 8));
Reliability            nonDefaultReliability(dds::core::policy::ReliabilityKind::RELIABLE,
                                             dds::core::Duration(8, 8));
DestinationOrder       nonDefaultOrder(dds::core::policy::DestinationOrderKind::BY_SOURCE_TIMESTAMP);
History                nonDefaultHistory(dds::core::policy::HistoryKind::KEEP_ALL,
                                         17 /* depth */);
ResourceLimits         nonDefaultResources(200, /* max_samples */
                                           10,  /* max_instances */
                                           50   /* max_samples_per_instance */);
TransportPriority      nonDefaultPriority(19 /* priority */);
Lifespan               nonDefaultLifespan(dds::core::Duration::zero());
Ownership              nonDefaultOwnership(dds::core::policy::OwnershipKind::EXCLUSIVE);
Presentation           nonDefaultPresentation(dds::core::policy::PresentationAccessScopeKind::TOPIC,
                                       true, /* coherent access */
                                       false  /* ordered access */);
Partition              nonDefaultPartition("ExamplePartition");
GroupData              nonDefaultGdata(nonDefaultByteSeq);
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
OwnershipStrength      nonDefaultStrength(15);
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
WriterDataLifecycle    nonDefaultWdLifecycle(true);
TimeBasedFilter        nonDefaultTbFilter(dds::core::Duration(14, 14));
ReaderDataLifecycle    nonDefaultRdLifecycle(dds::core::Duration(1, 1),
                                             dds::core::Duration(2, 2));
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
DataRepresentation     nonDefaultRepresentation({dds::core::policy::DataRepresentationId::XCDR1,
                                                 dds::core::policy::DataRepresentationId::XML,
                                                 dds::core::policy::DataRepresentationId::XCDR2});
TypeConsistencyEnforcement nonDefaultTypeConsistencyEnforcement(dds::core::policy::TypeConsistencyKind::ALLOW_TYPE_COERCION, true, true, true, true, true);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT



/*
 * Temporary policy holders.
 */
UserData            tmpUserData;
EntityFactory       tmpEntityFactory;
TopicData           tmpTopicData;
Durability          tmpDurability;
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
DurabilityService   tmpDurabilityService;
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
Deadline            tmpDeadline;
LatencyBudget       tmpBudget;
Liveliness          tmpLiveliness;
Reliability         tmpReliability;
DestinationOrder    tmpOrder;
History             tmpHistory;
ResourceLimits      tmpResources;
TransportPriority   tmpPriority;
Lifespan            tmpLifespan;
Ownership           tmpOwnership;
Presentation        tmpPresentation;
Partition           tmpPartition;
GroupData           tmpGdata;
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
OwnershipStrength   tmpStrength;
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
WriterDataLifecycle tmpWdLifecycle;
TimeBasedFilter     tmpTbFilter;
ReaderDataLifecycle tmpRdLifecycle;
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
DataRepresentation  tmpRepresentation;
TypeConsistencyEnforcement  tmpEnforcement;
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

TEST(Qos, DomainParticipant)
{
    DomainParticipantQos dpQosDefault;

    /* Get non-default QoSses in different ways. */
    DomainParticipantQos dpQosShifted;
    dpQosShifted << nonDefaultEntityFactory
                 << nonDefaultUserData;
    DomainParticipantQos dpQosConstructed(dpQosShifted);
    DomainParticipantQos dpQosAssigned;
    dpQosAssigned = dpQosShifted;

    /* Compare the QoSses. */
    ASSERT_NE(dpQosDefault, dpQosShifted);
    ASSERT_NE(dpQosDefault, dpQosAssigned);
    ASSERT_NE(dpQosDefault, dpQosConstructed);
    ASSERT_EQ(dpQosShifted, dpQosConstructed);
    ASSERT_EQ(dpQosShifted, dpQosAssigned);

    /* Compare the Policies (getting them in different ways). */
    dpQosShifted >> tmpUserData;
    dpQosShifted >> tmpEntityFactory;
    ASSERT_EQ(nonDefaultUserData,      tmpUserData);
    ASSERT_EQ(nonDefaultEntityFactory, tmpEntityFactory);

    ASSERT_EQ(nonDefaultUserData,      dpQosConstructed.policy<UserData>());
    ASSERT_EQ(nonDefaultEntityFactory, dpQosConstructed.policy<EntityFactory>());
}

TEST(Qos, Topic)
{
    TopicQos tQosDefault;

    /* Get non-default QoSses in different ways. */
    TopicQos tQosShifted;
    tQosShifted << nonDefaultTopicData
                << nonDefaultDurability
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
                << nonDefaultDurabilityService
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
                << nonDefaultDeadline
                << nonDefaultBudget
                << nonDefaultLiveliness
                << nonDefaultReliability
                << nonDefaultOrder
                << nonDefaultHistory
                << nonDefaultResources
                << nonDefaultPriority
                << nonDefaultLifespan
                << nonDefaultOwnership
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
                << nonDefaultRepresentation
                << nonDefaultTypeConsistencyEnforcement
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
                ;
    TopicQos tQosConstructed(tQosShifted);
    TopicQos tQosAssigned;
    tQosAssigned = tQosShifted;

    /* Compare the QoSses. */
    ASSERT_NE(tQosDefault, tQosShifted);
    ASSERT_NE(tQosDefault, tQosAssigned);
    ASSERT_NE(tQosDefault, tQosConstructed);
    ASSERT_EQ(tQosShifted, tQosConstructed);
    ASSERT_EQ(tQosShifted, tQosAssigned);

    /* Compare the Policies (getting them in different ways). */
    tQosShifted >> tmpTopicData;
    tQosShifted >> tmpDurability;
    tQosShifted >> tmpDeadline;
    tQosShifted >> tmpBudget;
    tQosShifted >> tmpLiveliness;
    tQosShifted >> tmpReliability;
    tQosShifted >> tmpOrder;
    tQosShifted >> tmpHistory;
    tQosShifted >> tmpResources;
    tQosShifted >> tmpPriority;
    tQosShifted >> tmpLifespan;
    tQosShifted >> tmpOwnership;
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    tQosShifted >> tmpRepresentation;
    tQosShifted >> tmpEnforcement;
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    ASSERT_EQ(nonDefaultTopicData,   tmpTopicData);
    ASSERT_EQ(nonDefaultDurability,  tmpDurability);
    ASSERT_EQ(nonDefaultDeadline,    tmpDeadline);
    ASSERT_EQ(nonDefaultBudget,      tmpBudget);
    ASSERT_EQ(nonDefaultLiveliness,  tmpLiveliness);
    ASSERT_EQ(nonDefaultReliability, tmpReliability);
    ASSERT_EQ(nonDefaultOrder,       tmpOrder);
    ASSERT_EQ(nonDefaultHistory,     tmpHistory);
    ASSERT_EQ(nonDefaultResources,   tmpResources);
    ASSERT_EQ(nonDefaultPriority,    tmpPriority);
    ASSERT_EQ(nonDefaultLifespan,    tmpLifespan);
    ASSERT_EQ(nonDefaultOwnership,   tmpOwnership);
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    ASSERT_EQ(nonDefaultRepresentation, tmpRepresentation);
    ASSERT_EQ(nonDefaultTypeConsistencyEnforcement, tmpEnforcement);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

    ASSERT_EQ(nonDefaultTopicData,   tQosConstructed.policy<TopicData>());
    ASSERT_EQ(nonDefaultDurability,  tQosConstructed.policy<Durability>());
    ASSERT_EQ(nonDefaultDeadline,    tQosConstructed.policy<Deadline>());
    ASSERT_EQ(nonDefaultBudget,      tQosConstructed.policy<LatencyBudget>());
    ASSERT_EQ(nonDefaultLiveliness,  tQosConstructed.policy<Liveliness>());
    ASSERT_EQ(nonDefaultReliability, tQosConstructed.policy<Reliability>());
    ASSERT_EQ(nonDefaultOrder,       tQosConstructed.policy<DestinationOrder>());
    ASSERT_EQ(nonDefaultHistory,     tQosConstructed.policy<History>());
    ASSERT_EQ(nonDefaultResources,   tQosConstructed.policy<ResourceLimits>());
    ASSERT_EQ(nonDefaultPriority,    tQosConstructed.policy<TransportPriority>());
    ASSERT_EQ(nonDefaultLifespan,    tQosConstructed.policy<Lifespan>());
    ASSERT_EQ(nonDefaultOwnership,   tQosConstructed.policy<Ownership>());
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    ASSERT_EQ(nonDefaultRepresentation, tQosConstructed.policy<DataRepresentation>());
    ASSERT_EQ(nonDefaultTypeConsistencyEnforcement, tQosConstructed.policy<TypeConsistencyEnforcement>());
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    tQosShifted >> tmpDurabilityService;
    ASSERT_EQ(nonDefaultDurabilityService, tmpDurabilityService);
    ASSERT_EQ(nonDefaultDurabilityService, tQosConstructed.policy<dds::core::policy::DurabilityService>());
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
}

TEST(Qos, Publisher)
{
    PublisherQos pQosDefault;

    /* Get non-default QoSses in different ways. */
    PublisherQos pQosShifted;
    pQosShifted << nonDefaultEntityFactory
                << nonDefaultPresentation
                << nonDefaultPartition
                << nonDefaultGdata;
    PublisherQos pQosConstructed(pQosShifted);
    PublisherQos pQosAssigned;
    pQosAssigned = pQosShifted;

    /* Compare the QoSses. */
    ASSERT_NE(pQosDefault, pQosShifted);
    ASSERT_NE(pQosDefault, pQosAssigned);
    ASSERT_NE(pQosDefault, pQosConstructed);
    ASSERT_EQ(pQosShifted, pQosConstructed);
    ASSERT_EQ(pQosShifted, pQosAssigned);

    /* Compare the Policies (getting them in different ways). */
    pQosShifted >> tmpEntityFactory;
    pQosShifted >> tmpPresentation;
    pQosShifted >> tmpPartition;
    pQosShifted >> tmpGdata;
    ASSERT_EQ(nonDefaultEntityFactory, tmpEntityFactory);
    ASSERT_EQ(nonDefaultPresentation,  tmpPresentation);
    ASSERT_EQ(nonDefaultPartition,     tmpPartition);
    ASSERT_EQ(nonDefaultGdata,         tmpGdata);

    ASSERT_EQ(nonDefaultEntityFactory, pQosConstructed.policy<EntityFactory>());
    ASSERT_EQ(nonDefaultPresentation,  pQosConstructed.policy<Presentation>());
    ASSERT_EQ(nonDefaultPartition,     pQosConstructed.policy<Partition>());
    ASSERT_EQ(nonDefaultGdata,         pQosConstructed.policy<GroupData>());
}

TEST(Qos, Subscriber)
{
    SubscriberQos sQosDefault;

    /* Get non-default QoSses in different ways. */
    SubscriberQos sQosShifted;
    sQosShifted << nonDefaultEntityFactory
                << nonDefaultPresentation
                << nonDefaultPartition
                << nonDefaultGdata;
    SubscriberQos sQosConstructed(sQosShifted);
    SubscriberQos sQosAssigned;
    sQosAssigned = sQosShifted;

    /* Compare the QoSses. */
    ASSERT_NE(sQosDefault, sQosShifted);
    ASSERT_NE(sQosDefault, sQosAssigned);
    ASSERT_NE(sQosDefault, sQosConstructed);
    ASSERT_EQ(sQosShifted, sQosConstructed);
    ASSERT_EQ(sQosShifted, sQosAssigned);

    /* Compare the Policies (getting them in different ways). */
    sQosShifted >> tmpEntityFactory;
    sQosShifted >> tmpPresentation;
    sQosShifted >> tmpPartition;
    sQosShifted >> tmpGdata;
    ASSERT_EQ(nonDefaultEntityFactory, tmpEntityFactory);
    ASSERT_EQ(nonDefaultPresentation,  tmpPresentation);
    ASSERT_EQ(nonDefaultPartition,     tmpPartition);
    ASSERT_EQ(nonDefaultGdata,         tmpGdata);

    ASSERT_EQ(nonDefaultEntityFactory, sQosConstructed.policy<EntityFactory>());
    ASSERT_EQ(nonDefaultPresentation,  sQosConstructed.policy<Presentation>());
    ASSERT_EQ(nonDefaultPartition,     sQosConstructed.policy<Partition>());
    ASSERT_EQ(nonDefaultGdata,         sQosConstructed.policy<GroupData>());
}

TEST(Qos, DataWriter)
{
    DataWriterQos dwQosDefault;

    TopicQos tQosShifted;
    tQosShifted << nonDefaultTopicData
                << nonDefaultDurability
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
                << nonDefaultDurabilityService
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
                << nonDefaultDeadline
                << nonDefaultBudget
                << nonDefaultLiveliness
                << nonDefaultReliability
                << nonDefaultOrder
                << nonDefaultHistory
                << nonDefaultResources
                << nonDefaultPriority
                << nonDefaultLifespan
                << nonDefaultOwnership
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
                << nonDefaultRepresentation
                << nonDefaultTypeConsistencyEnforcement
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
                  ;

    /* Get non-default QoSses in different ways. */
    DataWriterQos dwQosShifted;
    dwQosShifted << nonDefaultUserData
                 << nonDefaultDurability
                 << nonDefaultDeadline
                 << nonDefaultBudget
                 << nonDefaultLiveliness
                 << nonDefaultReliability
                 << nonDefaultOrder
                 << nonDefaultHistory
                 << nonDefaultResources
                 << nonDefaultPriority
                 << nonDefaultLifespan
                 << nonDefaultOwnership
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
                 << nonDefaultStrength
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
                 << nonDefaultWdLifecycle
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
                 << nonDefaultRepresentation
                 << nonDefaultTypeConsistencyEnforcement
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
                  ;
    DataWriterQos dwQosWConstructed(dwQosShifted);
    DataWriterQos dwQosWAssigned1 = dwQosShifted; /* Actually calls copy constructor. */
    DataWriterQos dwQosWAssigned2;
    DataWriterQos dwQosTConstructed(tQosShifted);
    DataWriterQos dwQosTAssigned1 = tQosShifted; /* Actually calls copy constructor. */
    DataWriterQos dwQosTAssigned2;
    dwQosWAssigned2 = dwQosShifted;
    dwQosTAssigned2 = tQosShifted;

    /* Compare the QoSses. */
    ASSERT_NE(dwQosDefault,      dwQosWConstructed);
    ASSERT_NE(dwQosDefault,      dwQosWAssigned1);
    ASSERT_NE(dwQosDefault,      dwQosWAssigned2);
    ASSERT_NE(dwQosDefault,      dwQosTConstructed);
    ASSERT_NE(dwQosDefault,      dwQosTAssigned1);
    ASSERT_NE(dwQosDefault,      dwQosTAssigned2);
    ASSERT_NE(dwQosWConstructed, dwQosTConstructed);
    ASSERT_EQ(dwQosWConstructed, dwQosWAssigned2);
    ASSERT_EQ(dwQosWAssigned1,   dwQosWAssigned2);
    ASSERT_EQ(dwQosTConstructed, dwQosTAssigned2);
    ASSERT_EQ(dwQosTAssigned1,   dwQosTAssigned2);

    /* Compare the Policies (getting them in different ways). */
    dwQosShifted >> tmpUserData;
    dwQosShifted >> tmpDurability;
    dwQosShifted >> tmpDeadline;
    dwQosShifted >> tmpBudget;
    dwQosShifted >> tmpLiveliness;
    dwQosShifted >> tmpReliability;
    dwQosShifted >> tmpOrder;
    dwQosShifted >> tmpHistory;
    dwQosShifted >> tmpResources;
    dwQosShifted >> tmpPriority;
    dwQosShifted >> tmpLifespan;
    dwQosShifted >> tmpOwnership;
    dwQosShifted >> tmpWdLifecycle;
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    dwQosShifted >> tmpRepresentation;
    dwQosShifted >> tmpEnforcement;
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    ASSERT_EQ(nonDefaultUserData,    tmpUserData);
    ASSERT_EQ(nonDefaultDurability,  tmpDurability);
    ASSERT_EQ(nonDefaultDeadline,    tmpDeadline);
    ASSERT_EQ(nonDefaultBudget,      tmpBudget);
    ASSERT_EQ(nonDefaultLiveliness,  tmpLiveliness);
    ASSERT_EQ(nonDefaultReliability, tmpReliability);
    ASSERT_EQ(nonDefaultOrder,       tmpOrder);
    ASSERT_EQ(nonDefaultHistory,     tmpHistory);
    ASSERT_EQ(nonDefaultResources,   tmpResources);
    ASSERT_EQ(nonDefaultPriority,    tmpPriority);
    ASSERT_EQ(nonDefaultLifespan,    tmpLifespan);
    ASSERT_EQ(nonDefaultOwnership,   tmpOwnership);
    ASSERT_EQ(nonDefaultWdLifecycle, tmpWdLifecycle);
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    ASSERT_EQ(nonDefaultRepresentation, tmpRepresentation);
    ASSERT_EQ(nonDefaultTypeConsistencyEnforcement, tmpEnforcement);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

    ASSERT_EQ(nonDefaultUserData,    dwQosWConstructed.policy<UserData>());
    ASSERT_EQ(nonDefaultDurability,  dwQosWConstructed.policy<Durability>());
    ASSERT_EQ(nonDefaultDeadline,    dwQosWConstructed.policy<Deadline>());
    ASSERT_EQ(nonDefaultBudget,      dwQosWConstructed.policy<LatencyBudget>());
    ASSERT_EQ(nonDefaultLiveliness,  dwQosWConstructed.policy<Liveliness>());
    ASSERT_EQ(nonDefaultReliability, dwQosWConstructed.policy<Reliability>());
    ASSERT_EQ(nonDefaultOrder,       dwQosWConstructed.policy<DestinationOrder>());
    ASSERT_EQ(nonDefaultHistory,     dwQosWConstructed.policy<History>());
    ASSERT_EQ(nonDefaultResources,   dwQosWConstructed.policy<ResourceLimits>());
    ASSERT_EQ(nonDefaultPriority,    dwQosWConstructed.policy<TransportPriority>());
    ASSERT_EQ(nonDefaultLifespan,    dwQosWConstructed.policy<Lifespan>());
    ASSERT_EQ(nonDefaultOwnership,   dwQosWConstructed.policy<Ownership>());
    ASSERT_EQ(nonDefaultWdLifecycle, dwQosWConstructed.policy<WriterDataLifecycle>());
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    ASSERT_EQ(nonDefaultRepresentation, dwQosWConstructed.policy<DataRepresentation>());
    ASSERT_EQ(nonDefaultTypeConsistencyEnforcement, dwQosWConstructed.policy<TypeConsistencyEnforcement>());
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
    dwQosShifted >> tmpStrength;
    ASSERT_EQ(nonDefaultStrength,    tmpStrength);
    ASSERT_EQ(nonDefaultStrength,    dwQosWConstructed.policy<dds::core::policy::OwnershipStrength>());
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
}

TEST(Qos, DataReader)
{
    DataReaderQos drQosDefault;

    TopicQos tQosShifted;
    tQosShifted << nonDefaultTopicData
                << nonDefaultDurability
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
                << nonDefaultDurabilityService
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
                << nonDefaultDeadline
                << nonDefaultBudget
                << nonDefaultLiveliness
                << nonDefaultReliability
                << nonDefaultOrder
                << nonDefaultHistory
                << nonDefaultResources
                << nonDefaultPriority
                << nonDefaultLifespan
                << nonDefaultOwnership
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
                << nonDefaultRepresentation
                << nonDefaultTypeConsistencyEnforcement
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
                ;

    /* Get non-default QoSses in different ways. */
    DataReaderQos drQosShifted;
    drQosShifted << nonDefaultUserData
                 << nonDefaultDurability
                 << nonDefaultDeadline
                 << nonDefaultBudget
                 << nonDefaultLiveliness
                 << nonDefaultReliability
                 << nonDefaultOrder
                 << nonDefaultHistory
                 << nonDefaultResources
                 << nonDefaultOwnership
                 << nonDefaultTbFilter
                 << nonDefaultRdLifecycle
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
                 << nonDefaultRepresentation
                 << nonDefaultTypeConsistencyEnforcement
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
                  ;
    DataReaderQos drQosRConstructed(drQosShifted);
    DataReaderQos drQosRAssigned1 = drQosShifted; /* Actually calls copy constructor. */
    DataReaderQos drQosRAssigned2;
    DataReaderQos drQosTConstructed(tQosShifted);
    DataReaderQos drQosTAssigned1 = tQosShifted; /* Actually calls copy constructor. */
    DataReaderQos drQosTAssigned2;
    drQosRAssigned2 = drQosShifted;
    drQosTAssigned2 = tQosShifted;

    /* Compare the QoSses. */
    ASSERT_NE(drQosDefault,      drQosRConstructed);
    ASSERT_NE(drQosDefault,      drQosRAssigned1);
    ASSERT_NE(drQosDefault,      drQosRAssigned2);
    ASSERT_NE(drQosDefault,      drQosTConstructed);
    ASSERT_NE(drQosDefault,      drQosTAssigned1);
    ASSERT_NE(drQosDefault,      drQosTAssigned2);
    ASSERT_NE(drQosRConstructed, drQosTConstructed);
    ASSERT_EQ(drQosRConstructed, drQosRAssigned2);
    ASSERT_EQ(drQosRAssigned1,   drQosRAssigned2);
    ASSERT_EQ(drQosTConstructed, drQosTAssigned2);
    ASSERT_EQ(drQosTAssigned1,   drQosTAssigned2);

    /* Compare the Policies (getting them in different ways). */
    drQosShifted >> tmpUserData;
    drQosShifted >> tmpDurability;
    drQosShifted >> tmpDeadline;
    drQosShifted >> tmpBudget;
    drQosShifted >> tmpLiveliness;
    drQosShifted >> tmpReliability;
    drQosShifted >> tmpOrder;
    drQosShifted >> tmpHistory;
    drQosShifted >> tmpResources;
    drQosShifted >> tmpOwnership;
    drQosShifted >> tmpTbFilter;
    drQosShifted >> tmpRdLifecycle;
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    drQosShifted >> tmpRepresentation;
    drQosShifted >> tmpEnforcement;
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    ASSERT_EQ(nonDefaultUserData,    tmpUserData);
    ASSERT_EQ(nonDefaultDurability,  tmpDurability);
    ASSERT_EQ(nonDefaultDeadline,    tmpDeadline);
    ASSERT_EQ(nonDefaultBudget,      tmpBudget);
    ASSERT_EQ(nonDefaultLiveliness,  tmpLiveliness);
    ASSERT_EQ(nonDefaultReliability, tmpReliability);
    ASSERT_EQ(nonDefaultOrder,       tmpOrder);
    ASSERT_EQ(nonDefaultHistory,     tmpHistory);
    ASSERT_EQ(nonDefaultResources,   tmpResources);
    ASSERT_EQ(nonDefaultOwnership,   tmpOwnership);
    ASSERT_EQ(nonDefaultTbFilter,    tmpTbFilter);
    ASSERT_EQ(nonDefaultRdLifecycle, tmpRdLifecycle);
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    ASSERT_EQ(nonDefaultRepresentation, tmpRepresentation);
    ASSERT_EQ(nonDefaultTypeConsistencyEnforcement, tmpEnforcement);
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT

    ASSERT_EQ(nonDefaultUserData,    drQosRConstructed.policy<UserData>());
    ASSERT_EQ(nonDefaultDurability,  drQosRConstructed.policy<Durability>());
    ASSERT_EQ(nonDefaultDeadline,    drQosRConstructed.policy<Deadline>());
    ASSERT_EQ(nonDefaultBudget,      drQosRConstructed.policy<LatencyBudget>());
    ASSERT_EQ(nonDefaultLiveliness,  drQosRConstructed.policy<Liveliness>());
    ASSERT_EQ(nonDefaultReliability, drQosRConstructed.policy<Reliability>());
    ASSERT_EQ(nonDefaultOrder,       drQosRConstructed.policy<DestinationOrder>());
    ASSERT_EQ(nonDefaultHistory,     drQosRConstructed.policy<History>());
    ASSERT_EQ(nonDefaultResources,   drQosRConstructed.policy<ResourceLimits>());
    ASSERT_EQ(nonDefaultOwnership,   drQosRConstructed.policy<Ownership>());
    ASSERT_EQ(nonDefaultTbFilter,    drQosRConstructed.policy<TimeBasedFilter>());
    ASSERT_EQ(nonDefaultRdLifecycle, drQosRConstructed.policy<ReaderDataLifecycle>());
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    ASSERT_EQ(nonDefaultRepresentation, drQosRConstructed.policy<DataRepresentation>());
    ASSERT_EQ(nonDefaultTypeConsistencyEnforcement, drQosRConstructed.policy<TypeConsistencyEnforcement>());
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
}

TEST(Qos, invalid_values)
{
    History        invalidHistory;
    ResourceLimits invalidResources;

    ASSERT_THROW({
        invalidHistory = History(dds::core::policy::HistoryKind::KEEP_LAST,
                                 0 /* depth */);
    }, dds::core::InconsistentPolicyError);

    ASSERT_THROW({
        invalidResources = ResourceLimits(0, /* max_samples */
                                          0,  /* max_instances */
                                          0   /* max_samples_per_instance */);
    }, dds::core::InvalidArgumentError);
}

TEST(Qos, invalid_policies)
{
    TopicQos             tQos;
    DataWriterQos        dwQos;
    DataReaderQos        drQos;

    History        invalidHistory;
    invalidHistory.kind(dds::core::policy::HistoryKind::KEEP_LAST);
    invalidHistory.depth(0);

    ASSERT_THROW({
        tQos << invalidHistory;
    }, dds::core::InconsistentPolicyError);

    ASSERT_THROW({
        dwQos << invalidHistory;
    }, dds::core::InconsistentPolicyError);

    ASSERT_THROW({
        drQos << invalidHistory;
    }, dds::core::InconsistentPolicyError);

    ResourceLimits invalidResources;
    invalidResources.max_samples(0);
    invalidResources.max_instances(0);
    invalidResources.max_samples_per_instance(0);

    ASSERT_THROW({
        tQos << invalidResources;
    }, dds::core::InvalidArgumentError);

    ASSERT_THROW({
        dwQos << invalidResources;
    }, dds::core::InvalidArgumentError);

    ASSERT_THROW({
        drQos << invalidResources;
    }, dds::core::InvalidArgumentError);
}

TEST(Qos, policy_name)
{
    ASSERT_EQ(dds::core::policy::policy_name<UserData>::name(),            "UserData");
    ASSERT_EQ(dds::core::policy::policy_name<Durability>::name(),          "Durability");
    ASSERT_EQ(dds::core::policy::policy_name<Presentation>::name(),        "Presentation");
    ASSERT_EQ(dds::core::policy::policy_name<Deadline>::name(),            "Deadline");
    ASSERT_EQ(dds::core::policy::policy_name<LatencyBudget>::name(),       "LatencyBudget");
    ASSERT_EQ(dds::core::policy::policy_name<TimeBasedFilter>::name(),     "TimeBasedFilter");
    ASSERT_EQ(dds::core::policy::policy_name<Ownership>::name(),           "Ownership");
#ifdef  OMG_DDS_OWNERSHIP_SUPPORT
    ASSERT_EQ(dds::core::policy::policy_name<OwnershipStrength>::name(),   "OwnershipStrength");
#endif  // OMG_DDS_OWNERSHIP_SUPPORT
    ASSERT_EQ(dds::core::policy::policy_name<Liveliness>::name(),          "Liveliness");
    ASSERT_EQ(dds::core::policy::policy_name<Partition>::name(),           "Partition");
    ASSERT_EQ(dds::core::policy::policy_name<Reliability>::name(),         "Reliability");
    ASSERT_EQ(dds::core::policy::policy_name<DestinationOrder>::name(),    "DestinationOrder");
    ASSERT_EQ(dds::core::policy::policy_name<History>::name(),             "History");
    ASSERT_EQ(dds::core::policy::policy_name<ResourceLimits>::name(),      "ResourceLimits");
    ASSERT_EQ(dds::core::policy::policy_name<EntityFactory>::name(),       "EntityFactory");
    ASSERT_EQ(dds::core::policy::policy_name<WriterDataLifecycle>::name(), "WriterDataLifecycle");
    ASSERT_EQ(dds::core::policy::policy_name<ReaderDataLifecycle>::name(), "ReaderDataLifecycle");
    ASSERT_EQ(dds::core::policy::policy_name<TopicData>::name(),           "TopicData");
    ASSERT_EQ(dds::core::policy::policy_name<GroupData>::name(),           "GroupData");
    ASSERT_EQ(dds::core::policy::policy_name<TransportPriority>::name(),   "TransportPriority");
    ASSERT_EQ(dds::core::policy::policy_name<Lifespan>::name(),            "Lifespan");
#ifdef  OMG_DDS_PERSISTENCE_SUPPORT
    ASSERT_EQ(dds::core::policy::policy_name<DurabilityService>::name(),   "DurabilityService");
#endif  // OMG_DDS_PERSISTENCE_SUPPORT
#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    ASSERT_EQ(dds::core::policy::policy_name<DataRepresentation>::name(),  "DataRepresentation");
    ASSERT_EQ(dds::core::policy::policy_name<TypeConsistencyEnforcement>::name(),  "TypeConsistencyEnforcement");
#endif //  OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
}
