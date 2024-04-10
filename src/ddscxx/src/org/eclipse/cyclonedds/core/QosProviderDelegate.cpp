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

#include <org/eclipse/cyclonedds/core/QosProviderDelegate.hpp>

#include <dds/ddsc/dds_public_qos_provider.h>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace core
{

QosProviderDelegate::QosProviderDelegate(const std::string& uri, const std::string& id) : qosProvider(nullptr)
{
    dds_return_t ret;

    ret = dds_create_qos_provider_scope(uri.c_str(), &qosProvider, id.c_str());
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Unable to create QosProvider.");
}

QosProviderDelegate::~QosProviderDelegate()
{
    dds_delete_qos_provider(qosProvider);
}

dds::domain::qos::DomainParticipantQos
QosProviderDelegate::participant_qos(const std::string &id)
{
    dds::domain::qos::DomainParticipantQos dpq;
    const dds_qos_t *c_dpq = NULL;
    dds_return_t ret;

    ret = dds_qos_provider_get_qos(qosProvider, DDS_PARTICIPANT_QOS, id.c_str(), &c_dpq);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Unable to obtain requested participant QoS.");

    dpq.delegate().ddsc_qos(c_dpq, false);

    return dpq;
}

dds::topic::qos::TopicQos
QosProviderDelegate::topic_qos(const std::string &id)
{
    dds::topic::qos::TopicQos tq;
    const dds_qos_t *c_tq = NULL;
    dds_return_t ret;

    ret = dds_qos_provider_get_qos(qosProvider, DDS_TOPIC_QOS, id.c_str(), &c_tq);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Unable to obtain requested topic QoS.");

    tq.delegate().ddsc_qos(c_tq, false);

    return tq;
}


dds::sub::qos::SubscriberQos
QosProviderDelegate::subscriber_qos(const std::string &id)
{
    dds::sub::qos::SubscriberQos sq;
    const dds_qos_t *c_sq = NULL;
    dds_return_t ret;

    ret = dds_qos_provider_get_qos(qosProvider, DDS_SUBSCRIBER_QOS, id.c_str(), &c_sq);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Unable to obtain requested subscriber QoS.");

    sq.delegate().ddsc_qos(c_sq, false);

    return sq;
}

dds::sub::qos::DataReaderQos
QosProviderDelegate::datareader_qos(const std::string &id)
{
    dds::sub::qos::DataReaderQos drq;
    const dds_qos_t *c_drq = NULL;
    dds_return_t ret;

    ret = dds_qos_provider_get_qos(qosProvider, DDS_READER_QOS, id.c_str(), &c_drq);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Unable to obtain requested datareader QoS.");

    drq.delegate().ddsc_qos(c_drq, false);

    return drq;
}

dds::pub::qos::PublisherQos
QosProviderDelegate::publisher_qos(const std::string &id)
{
    dds::pub::qos::PublisherQos pq;
    const dds_qos_t *c_pq = NULL;
    dds_return_t ret;

    ret = dds_qos_provider_get_qos(qosProvider, DDS_PUBLISHER_QOS, id.c_str(), &c_pq);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Unable to obtain requested publisher QoS.");

    pq.delegate().ddsc_qos(c_pq, false);

    return pq;
}

dds::pub::qos::DataWriterQos
QosProviderDelegate::datawriter_qos(const std::string &id)
{
    dds::pub::qos::DataWriterQos dwq;
    const dds_qos_t *c_dwq = NULL;
    dds_return_t ret;

    ret = dds_qos_provider_get_qos(qosProvider, DDS_WRITER_QOS, id.c_str(), &c_dwq);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Unable to obtain requested datawriter QoS.");

    dwq.delegate().ddsc_qos(c_dwq, false);

    return dwq;
}

}
}
}
}
