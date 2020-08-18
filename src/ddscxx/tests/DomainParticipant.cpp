/*
 * Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include "dds/dds.hpp"
#include "dds/ddscxx/test.h"
#include "dds/ddsrt/environ.h"
#include "dds/version.h"
#include "HelloWorldData_DCPS.hpp"
#include "Space_DCPS.hpp"

#include "config_env.hpp"

namespace ddscxx { namespace tests { namespace DomainParticipant {

class TestDomainParticipantListener : public virtual dds::domain::NoOpDomainParticipantListener
{ };

} } }

ddscxx::tests::DomainParticipant::TestDomainParticipantListener participantListener;

/* Check various null initializations. */
DDSCXX_TEST(ddscxx_DomainParticipant, null)
{
    dds::domain::DomainParticipant participant1(dds::core::null);
    dds::domain::DomainParticipant participant2 = dds::core::null;
    ASSERT_EQ(participant1, dds::core::null);
    ASSERT_EQ(participant2, dds::core::null);
}

/* Try creating a domain participant with an configuration. */
DDSCXX_TEST(ddscxx_DomainParticipant, create_with_conf)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;
    uint32_t valid_domain = 3;
    uint32_t domain_id;
    dds_return_t ret;

    ret = ddsrt_setenv(DDS_PROJECT_NAME_NOSPACE_CAPS"_URI", CONFIG_ENV_SIMPLE);
    ASSERT_EQ(ret, DDS_RETCODE_OK);

    /* Valid specific domain value. */
    participant1 = dds::domain::DomainParticipant(valid_domain);
    ASSERT_NE(participant1, dds::core::null);
    domain_id = participant1.domain_id();
    ASSERT_EQ(domain_id, valid_domain);

    /* Default domain value. */
    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);
    domain_id = participant2.domain_id();
    ASSERT_EQ(domain_id, valid_domain);
}

/* Try creating a domain participant with an configuration. */
DDSCXX_TEST(ddscxx_DomainParticipant, create_with_str_conffile)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;
    uint32_t valid_domain = 3;
    uint32_t domain_id;

    /* Valid specific domain value. */
    participant1 = dds::domain::DomainParticipant(valid_domain,
                                                  org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_participant_qos(),
                                                  NULL,
                                                  dds::core::status::StatusMask::none(),
                                                  CONFIG_ENV_SIMPLE);
    ASSERT_NE(participant1, dds::core::null);
    domain_id = participant1.domain_id();
    ASSERT_EQ(domain_id, valid_domain);

    /* Default domain value. */
    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);
    domain_id = participant2.domain_id();
    ASSERT_EQ(domain_id, valid_domain);
}

DDSCXX_TEST(ddscxx_DomainParticipant, create_with_str_conf)
{
    dds::domain::DomainParticipant participant7 = dds::core::null;
    dds::domain::DomainParticipant participant8 = dds::core::null;
    dds::domain::DomainParticipant participantD1 = dds::core::null;
    dds::domain::DomainParticipant participantD2 = dds::core::null;
    uint32_t domain7 = 7;
    uint32_t domain8 = 8;
    uint32_t domain_id;

    participant7 = dds::domain::DomainParticipant(domain7,
                                                  org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_participant_qos(),
                                                  NULL,
                                                  dds::core::status::StatusMask::none(),
                                                  "<" DDS_PROJECT_NAME "><Domain><Id>any</Id></Domain></" DDS_PROJECT_NAME ">");
    ASSERT_NE(participant7, dds::core::null);
    domain_id = participant7.domain_id();
    ASSERT_EQ(domain_id, domain7);

    participantD1 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participantD1, dds::core::null);
    domain_id = participantD1.domain_id();
    ASSERT_EQ(domain_id, domain7);

    participant8 = dds::domain::DomainParticipant(domain8,
                                                  org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_participant_qos(),
                                                  NULL,
                                                  dds::core::status::StatusMask::none(),
                                                  "<" DDS_PROJECT_NAME "><Domain><Id>any</Id></Domain></" DDS_PROJECT_NAME ">");
    ASSERT_NE(participant8, dds::core::null);
    domain_id = participant8.domain_id();
    ASSERT_EQ(domain_id, domain8);

    participantD2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participantD2, dds::core::null);
    domain_id = participantD2.domain_id();
    ASSERT_EQ(domain_id, domain7);
}

DDSCXX_TEST(ddscxx_DomainParticipant, recreate_with_str_conf)
{
    dds::domain::DomainParticipant participant = dds::core::null;
    uint32_t domain_req = 3;
    uint32_t domain_set;

    participant = dds::domain::DomainParticipant(domain_req,
                                                 org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_participant_qos(),
                                                 NULL,
                                                 dds::core::status::StatusMask::none(),
                                                 "<" DDS_PROJECT_NAME "><Domain><Id>any</Id></Domain></" DDS_PROJECT_NAME ">");
    ASSERT_NE(participant, dds::core::null);
    domain_set = participant.domain_id();
    ASSERT_EQ(domain_set, domain_req);

    participant = dds::core::null;


    participant = dds::domain::DomainParticipant(domain_req,
                                                 org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_participant_qos(),
                                                 NULL,
                                                 dds::core::status::StatusMask::none(),
                                                 "<" DDS_PROJECT_NAME "><Domain><Id>any</Id></Domain></" DDS_PROJECT_NAME ">");
    ASSERT_NE(participant, dds::core::null);
    domain_set = participant.domain_id();
    ASSERT_EQ(domain_set, domain_req);
}

DDSCXX_TEST(ddscxx_DomainParticipant, create_with_str_conf_default)
{
    dds::domain::DomainParticipant participant = dds::core::null;

    ASSERT_THROW({
        participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id(),
                                                     org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_participant_qos(),
                                                     NULL,
                                                     dds::core::status::StatusMask::none(),
                                                     "<" DDS_PROJECT_NAME "><Domain><Id>any</Id></Domain></" DDS_PROJECT_NAME ">");
    }, dds::core::InvalidArgumentError);
}

DDSCXX_TEST(ddscxx_DomainParticipant, create_with_str_conf_invalid)
{
    dds::domain::DomainParticipant participant = dds::core::null;

    ASSERT_THROW({
        participant = dds::domain::DomainParticipant(1,
                                                     org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_participant_qos(),
                                                     NULL,
                                                     dds::core::status::StatusMask::none(),
                                                     "<CycloneDDS incorrect XML");
    }, dds::core::Error);
}

/* Try creating a domain participant without an configuration. */
DDSCXX_TEST(ddscxx_DomainParticipant, create_without_conf)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;
    uint32_t valid_domain = 3;
    uint32_t domain_id;
    dds_return_t ret;

    ret = ddsrt_unsetenv(DDS_PROJECT_NAME_NOSPACE_CAPS"_URI");
    ASSERT_EQ(ret, DDS_RETCODE_OK);

    /* When giving a domain, that specific domain should be used. */
    participant1 = dds::domain::DomainParticipant(valid_domain);
    ASSERT_NE(participant1, dds::core::null);
    domain_id = participant1.domain_id();
    ASSERT_EQ(domain_id, valid_domain);

    /* The default should slave the set domain. */
    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);
    domain_id = participant2.domain_id();
    ASSERT_EQ(domain_id, valid_domain);
}

/* Try creating multiple domain participants at the same time. */
DDSCXX_TEST(ddscxx_DomainParticipant, create_multiple)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;

    participant1 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::any_id());
    ASSERT_NE(participant1, dds::core::null);

    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::any_id());
    ASSERT_NE(participant2, dds::core::null);
}

/* Try creating multiple domain participants at the same time. */
DDSCXX_TEST(ddscxx_DomainParticipant, create_invalid)
{
    dds::domain::DomainParticipant participant = dds::core::null;

    /* Maximum id is 230. */
    ASSERT_THROW({
        participant = dds::domain::DomainParticipant(231);
    }, dds::core::InvalidArgumentError);
}

/* Try re-creating domain participant after last one was deleted. */
DDSCXX_TEST(ddscxx_DomainParticipant, recreate_after_delete)
{
    {
        dds::domain::DomainParticipant participant = dds::core::null;
        participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(participant, dds::core::null);
        /* Out of scope means deletion. */
    }

    {
        /* Re-create. */
        dds::domain::DomainParticipant participant = dds::core::null;
        participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
        ASSERT_NE(participant, dds::core::null);
    }
}

/* Try re-creating domain participant after last one was closed. */
DDSCXX_TEST(ddscxx_DomainParticipant, recreate_after_close)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;

    participant1 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant1, dds::core::null);

    participant1.close();

    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);
}

/* Check exception thrown by usage after close. */
DDSCXX_TEST(ddscxx_DomainParticipant, use_after_close)
{
    dds::domain::DomainParticipant participant = dds::core::null;

    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant, dds::core::null);
    participant.close();

    ASSERT_THROW({
        participant.close();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::core::status::StatusMask mask;
        participant.listener(&participantListener, mask);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        participant.listener();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        participant.qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::domain::qos::DomainParticipantQos qos;
        participant.qos(qos);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        participant.domain_id();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        participant.assert_liveliness();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::core::InstanceHandle handle = dds::core::null;
        participant.contains_entity(handle);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        participant.current_time();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        participant.default_publisher_qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::pub::qos::PublisherQos qos;
        participant.default_publisher_qos(qos);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        participant.default_subscriber_qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::sub::qos::SubscriberQos qos;
        participant.default_subscriber_qos(qos);
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        participant.default_topic_qos();
    }, dds::core::AlreadyClosedError);

    ASSERT_THROW({
        dds::topic::qos::TopicQos qos;
        participant.default_topic_qos(qos);
    }, dds::core::AlreadyClosedError);
}

/* Check exception thrown by usage after deletion. */
DDSCXX_TEST(ddscxx_DomainParticipant, use_after_deletion)
{
    dds::domain::DomainParticipant participant = dds::core::null;

    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant, dds::core::null);
    participant = dds::core::null;

    ASSERT_THROW({
        participant.close();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::core::status::StatusMask mask;
        participant.listener(&participantListener, mask);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        participant.listener();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        participant.qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::domain::qos::DomainParticipantQos qos;
        participant.qos(qos);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        participant.domain_id();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        participant.assert_liveliness();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::core::InstanceHandle handle = dds::core::null;
        participant.contains_entity(handle);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        participant.current_time();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        participant.default_publisher_qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::pub::qos::PublisherQos qos;
        participant.default_publisher_qos(qos);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        participant.default_subscriber_qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::sub::qos::SubscriberQos qos;
        participant.default_subscriber_qos(qos);
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        participant.default_topic_qos();
    }, dds::core::NullReferenceError);

    ASSERT_THROW({
        dds::topic::qos::TopicQos qos;
        participant.default_topic_qos(qos);
    }, dds::core::NullReferenceError);
}

/* Find non-created DomainParticipant. */
DDSCXX_TEST(ddscxx_DomainParticipant, find_none)
{
    dds::domain::DomainParticipant found = dds::core::null;
    found = dds::domain::find(0);
    ASSERT_EQ(found, dds::core::null);
}

/* Find unknown DomainParticipant. */
DDSCXX_TEST(ddscxx_DomainParticipant, find_wrong)
{
    dds::domain::DomainParticipant participant = dds::core::null;
    dds::domain::DomainParticipant found = dds::core::null;
    uint32_t domain_id;

    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant, dds::core::null);
    domain_id = participant.domain_id();

    found = dds::domain::find(domain_id + 1);
    ASSERT_EQ(found, dds::core::null);
}

/* Find deleted DomainParticipant. */
DDSCXX_TEST(ddscxx_DomainParticipant, find_deleted)
{
    dds::domain::DomainParticipant participant = dds::core::null;
    dds::domain::DomainParticipant found = dds::core::null;
    uint32_t domain_id;

    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant, dds::core::null);
    domain_id = participant.domain_id();

    /* This forces a deletion. */
    participant = dds::core::null;

    found = dds::domain::find(domain_id);
    ASSERT_EQ(found, dds::core::null);
}

/* Find created DomainParticipant. */
DDSCXX_TEST(ddscxx_DomainParticipant, find_one)
{
    dds::domain::DomainParticipant participant = dds::core::null;
    dds::domain::DomainParticipant found = dds::core::null;
    uint32_t domain_id;

    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant, dds::core::null);
    domain_id = participant.domain_id();

    found = dds::domain::find(domain_id);
    ASSERT_NE(found, dds::core::null);
    ASSERT_EQ(found, participant);
}

/* Find one of created DomainParticipants. */
DDSCXX_TEST(ddscxx_DomainParticipant, find_multiple)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;
    dds::domain::DomainParticipant found = dds::core::null;
    uint32_t domain_id1;
    uint32_t domain_id2;

    participant1 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant1, dds::core::null);
    domain_id1 = participant1.domain_id();

    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);
    domain_id2 = participant2.domain_id();

    ASSERT_EQ(domain_id1, domain_id2);

    found = dds::domain::find(domain_id1);
    ASSERT_NE(found, dds::core::null);
    ASSERT_TRUE((found == participant1) || (found == participant2));
}

/* Find the DomainParticipant that isn't deleted. */
DDSCXX_TEST(ddscxx_DomainParticipant, find_first)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;
    dds::domain::DomainParticipant found = dds::core::null;
    uint32_t domain_id1;
    uint32_t domain_id2;

    participant1 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant1, dds::core::null);
    domain_id1 = participant1.domain_id();

    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);
    domain_id2 = participant2.domain_id();

    ASSERT_EQ(domain_id1, domain_id2);

    /* Delete participant2 to force participant1 to be found. */
    participant2 = dds::core::null;

    found = dds::domain::find(domain_id1);
    ASSERT_NE(found, dds::core::null);
    ASSERT_EQ(found, participant1);
}

/* Find the DomainParticipant that isn't deleted. */
DDSCXX_TEST(ddscxx_DomainParticipant, find_second)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;
    dds::domain::DomainParticipant found = dds::core::null;
    uint32_t domain_id1;
    uint32_t domain_id2;

    participant1 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant1, dds::core::null);
    domain_id1 = participant1.domain_id();

    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);
    domain_id2 = participant2.domain_id();

    ASSERT_EQ(domain_id1, domain_id2);

    /* Delete participant1 to force participant2 to be found. */
    participant1 = dds::core::null;

    found = dds::domain::find(domain_id2);
    ASSERT_NE(found, dds::core::null);
    ASSERT_EQ(found, participant2);
}

/* Try to set null DomainParticipant to be ignored during discovery. */
DDSCXX_TEST(ddscxx_DomainParticipant, ignore_none)
{
    dds::domain::DomainParticipant participant = dds::core::null;
    dds::core::InstanceHandle hdl = dds::core::null;

    ASSERT_THROW({
        dds::domain::ignore(participant, hdl);
    }, dds::core::NullReferenceError);
}

/* Set a DomainParticipant to be ignored during discovery. */
DDSCXX_TEST(ddscxx_DomainParticipant, ignore_one)
{
    dds::domain::DomainParticipant participant = dds::core::null;
    dds::core::InstanceHandle hdl = dds::core::null;

    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant, dds::core::null);

    /* TODO: This has to be implemented. */
    ASSERT_THROW({
        dds::domain::ignore(participant, hdl);
    }, dds::core::UnsupportedError);
}

/* Check if a DomainParticipant can be created with a non default constructor. */
DDSCXX_TEST(ddscxx_DomainParticipant, non_default_constructor)
{
    dds::domain::DomainParticipant participant = dds::core::null;
    dds::core::status::StatusMask statusMask;
    dds::domain::qos::DomainParticipantQos dpQos = participant.default_participant_qos();

    participant = dds::domain::DomainParticipant(
                            org::eclipse::cyclonedds::domain::default_id(),
                            dpQos,
                            0,
                            statusMask);
    ASSERT_NE(participant, dds::core::null);
}

/* Check if a DomainParticipant returns current time. */
DDSCXX_TEST(ddscxx_DomainParticipant, current_time)
{
    dds::domain::DomainParticipant participant = dds::core::null;
    dds::core::Time par_time1;
    dds::core::Time par_time2;

    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant, dds::core::null);

    par_time1 = participant.current_time();
    dds_sleepfor(DDS_MSECS(2));
    par_time2 = participant.current_time();
    ASSERT_LT(par_time1, par_time2);
}


/* Check setting a default QoS does not trigger an exception. */
DDSCXX_TEST(ddscxx_DomainParticipant, default__qos)
{
    dds::domain::DomainParticipant participant = dds::core::null;

    participant = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant, dds::core::null);

    dds::domain::qos::DomainParticipantQos dpQos;
    participant.default_participant_qos(dpQos);

    dds::pub::qos::PublisherQos pubQos;
    participant.default_publisher_qos(pubQos);

    dds::sub::qos::SubscriberQos subQos;
    participant.default_subscriber_qos(subQos);

    dds::topic::qos::TopicQos topQos;
    participant.default_topic_qos(topQos);
}

#if 0

/*
dds::domain::DomainParticipant::contains_entity() can not be tested properly
yet because ddsc does not support instance handles for publisher, subscriber or
topic. This has to be implemented before contains_entity() can be tested.
*/

/* Check if a DomainParticipant contains and entity that was created on it. */
DDSCXX_XFAIL(ddscxx_DomainParticipant, contains_subscriber)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;
    dds::sub::Subscriber sub1 = dds::core::null;
    dds::sub::Subscriber sub2 = dds::core::null;
    bool found;

    participant1 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant1, dds::core::null);
    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);

    sub1 = dds::sub::Subscriber(participant1);
    ASSERT_NE(sub1, dds::core::null);
    sub2 = dds::sub::Subscriber(participant1);
    ASSERT_NE(sub2, dds::core::null);

    found = participant1.contains_entity(sub1.instance_handle());
    ASSERT_TRUE(found);
    found = participant1.contains_entity(sub2.instance_handle());
    ASSERT_FALSE(found);
    found = participant2.contains_entity(sub1.instance_handle());
    ASSERT_FALSE(found);
    found = participant2.contains_entity(sub2.instance_handle());
    ASSERT_TRUE(found);
}

/* Check if a DomainParticipant contains and entity that was created on it. */
DDSCXX_XFAIL(ddscxx_DomainParticipant, contains_publisher)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;
    dds::pub::Publisher pub1 = dds::core::null;
    dds::pub::Publisher pub2 = dds::core::null;
    bool found;

    participant1 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant1, dds::core::null);
    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);

    pub1 = dds::pub::Publisher(participant1);
    ASSERT_NE(pub1, dds::core::null);
    pub2 = dds::pub::Publisher(participant1);
    ASSERT_NE(pub2, dds::core::null);

    found = participant1.contains_entity(pub1.instance_handle());
    ASSERT_TRUE(found);
    found = participant1.contains_entity(pub2.instance_handle());
    ASSERT_FALSE(found);
    found = participant2.contains_entity(pub1.instance_handle());
    ASSERT_FALSE(found);
    found = participant2.contains_entity(pub2.instance_handle());
    ASSERT_TRUE(found);
}

/* Check if a DomainParticipant contains and entity that was created on it. */
DDSCXX_XFAIL(ddscxx_DomainParticipant, contains_topic)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;
    dds::topic::Topic<HelloWorldData::Msg> topic1 = dds::core::null;
    dds::topic::Topic<HelloWorldData::Msg> topic2 = dds::core::null;
    bool found;

    participant1 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant1, dds::core::null);
    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);

    topic1 = dds::topic::Topic<HelloWorldData::Msg>(participant1, "topic1");
    ASSERT_NE(topic1, dds::core::null);
    topic2 = dds::topic::Topic<HelloWorldData::Msg>(participant2, "topic2");
    ASSERT_NE(topic2, dds::core::null);

    found = participant1.contains_entity(topic1.instance_handle());
    ASSERT_TRUE(found);
    found = participant1.contains_entity(topic2.instance_handle());
    ASSERT_FALSE(found);
    found = participant2.contains_entity(topic1.instance_handle());
    ASSERT_FALSE(found);
    found = participant2.contains_entity(topic2.instance_handle());
    ASSERT_TRUE(found);
}

/* Check if a DomainParticipant contains and entity that was created on it. */
DDSCXX_XFAIL(ddscxx_DomainParticipant, contains_writer)
{
    dds::domain::DomainParticipant participant1 = dds::core::null;
    dds::domain::DomainParticipant participant2 = dds::core::null;
    dds::pub::Publisher pub1_A = dds::core::null;
    dds::pub::Publisher pub1_B = dds::core::null;
    dds::pub::Publisher pub2_A = dds::core::null;
    dds::pub::Publisher pub2_B = dds::core::null;
    dds::topic::Topic<Space::Type1> topic1 = dds::core::null;
    dds::topic::Topic<Space::Type1> topic2 = dds::core::null;
    dds::pub::DataWriter<Space::Type1> writer1_A  = dds::core::null;
    dds::pub::DataWriter<Space::Type1> writer1_B  = dds::core::null;
    dds::pub::DataWriter<Space::Type1> writer2_A1 = dds::core::null;
    dds::pub::DataWriter<Space::Type1> writer2_A2 = dds::core::null;

    bool found;

    participant1 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant1, dds::core::null);
    participant2 = dds::domain::DomainParticipant(org::eclipse::cyclonedds::domain::default_id());
    ASSERT_NE(participant2, dds::core::null);

    pub1_A = dds::pub::Publisher(participant1);
    ASSERT_NE(pub1_A, dds::core::null);
    pub1_B = dds::pub::Publisher(participant1);
    ASSERT_NE(pub1_B, dds::core::null);
    pub2_A = dds::pub::Publisher(participant2);
    ASSERT_NE(pub2_A, dds::core::null);
    pub2_B = dds::pub::Publisher(participant2);
    ASSERT_NE(pub2_B, dds::core::null);


    topic1 = dds::topic::Topic<Space::Type1>(participant1, "contains_writer_top1");
    ASSERT_NE(topic1, dds::core::null);
    topic2 = dds::topic::Topic<Space::Type1>(participant2, "contains_writer_top2");
    ASSERT_NE(topic2, dds::core::null);

    writer1_A = dds::pub::DataWriter<Space::Type1>(pub1_A, topic1);
    ASSERT_NE(writer1_A, dds::core::null);
    writer1_B = dds::pub::DataWriter<Space::Type1>(pub1_B, topic1);
    ASSERT_NE(writer1_B, dds::core::null);
    writer2_A1 = dds::pub::DataWriter<Space::Type1>(pub2_A, topic2);
    ASSERT_NE(writer2_A1, dds::core::null);
    writer2_A2 = dds::pub::DataWriter<Space::Type1>(pub2_A, topic2);
    ASSERT_NE(writer2_A2, dds::core::null);

    found = participant1.contains_entity(writer1_A.instance_handle());
    ASSERT_TRUE(found);
    found = participant1.contains_entity(writer1_B.instance_handle());
    ASSERT_TRUE(found);
    found = participant1.contains_entity(writer2_A1.instance_handle());
    ASSERT_FALSE(found);
    found = participant1.contains_entity(writer2_A2.instance_handle());
    ASSERT_FALSE(found);

    found = participant2.contains_entity(writer1_A.instance_handle());
    ASSERT_FALSE(found);
    found = participant2.contains_entity(writer1_B.instance_handle());
    ASSERT_FALSE(found);
    found = participant2.contains_entity(writer2_A1.instance_handle());
    ASSERT_TRUE(found);
    found = participant2.contains_entity(writer2_A2.instance_handle());
    ASSERT_TRUE(found);
}

#endif
