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

#include <dds/pub/AnyDataWriter.hpp>
#include <org/eclipse/cyclonedds/pub/AnyDataWriterDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/core/MiscUtils.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>
#include <org/eclipse/cyclonedds/topic/BuiltinTopicCopy.hpp>
#include <dds/dds.h>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace pub
{

/* For dynamic casting to AnyDataWriterDelegate to work for a few older compilers,
 * it is needed that (at least) the constructor is moved to the cpp file. */
AnyDataWriterDelegate::AnyDataWriterDelegate(
        const dds::pub::qos::DataWriterQos& qos,
        const dds::topic::TopicDescription& td)
    : copyIn(NULL), copyOut(NULL), sampleSize(0), qos_(qos), td_(td)
{
}

AnyDataWriterDelegate::~AnyDataWriterDelegate()
{
}

void
AnyDataWriterDelegate::close()
{
    this->td_ = dds::topic::TopicDescription(dds::core::null);
    org::eclipse::cyclonedds::core::EntityDelegate::close();
}

const dds::topic::TopicDescription&
AnyDataWriterDelegate::topic_description() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return this->td_;
}

dds::pub::qos::DataWriterQos
AnyDataWriterDelegate::qos() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return qos_;
}


void
AnyDataWriterDelegate::qos(const dds::pub::qos::DataWriterQos& qos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    qos.delegate().check();
    dds_qos_t *dwQos = qos.delegate().ddsc_qos();
    dds_return_t ret = dds_set_qos(ddsc_entity, dwQos);
    dds_delete_qos(dwQos);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Could not set writer qos.");
    this->qos_ = qos;
}

void
AnyDataWriterDelegate::write(
    dds_entity_t writer,
    const void *data,
    const dds::core::InstanceHandle& handle,
    const dds::core::Time& timestamp)
{
    dds_return_t ret;

    /* Ignore the handle until ddsc supports writes with instance handles. */
    (void)handle;

    if (timestamp != dds::core::Time::invalid()) {
        dds_time_t ddsc_time = org::eclipse::cyclonedds::core::convertTime(timestamp);
        ret = dds_write_ts(writer, data, ddsc_time);
    } else {
        ret = dds_write(writer, data);
    }

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "write failed.");
}

void
AnyDataWriterDelegate::writedispose(
    dds_entity_t writer,
    const void *data,
    const dds::core::InstanceHandle& handle,
    const dds::core::Time& timestamp)
{
    dds_return_t ret;
    void *c_sample;

    /* Ignore the handle until ddsc supports writes with instance handles. */
    (void)handle;

    c_sample = dds_alloc(this->sampleSize);
    (*(this->copyIn))(data, c_sample);

    if (timestamp != dds::core::Time::invalid()) {
        dds_time_t ddsc_time = org::eclipse::cyclonedds::core::convertTime(timestamp);
        ret = dds_writedispose_ts(writer, c_sample, ddsc_time);
    } else {
        ret = dds_writedispose(writer, c_sample);
    }

    dds_free (c_sample);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "writedispose failed.");
}

dds_instance_handle_t
AnyDataWriterDelegate::register_instance(
    dds_entity_t writer,
    const void *data,
    const dds::core::Time& timestamp)
{
    dds_instance_handle_t ih;
    dds_return_t ret;
    void *c_sample;

    if (timestamp != dds::core::Time::invalid()) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR,
                               "Registering with a timestamp is not supported.");
    }

    c_sample = dds_alloc(this->sampleSize);
    (*(this->copyIn))(data, c_sample);

    ret = dds_register_instance(writer, &ih, c_sample);

    dds_free (c_sample);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_instance_register failed.");

    return ih;
}

void
AnyDataWriterDelegate::unregister_instance(
    dds_entity_t writer,
    const dds::core::InstanceHandle& handle,
    const dds::core::Time& timestamp)
{
    dds_instance_handle_t ih;
    dds_return_t ret;

    if (handle == dds::core::null) {
      ISOCPP_THROW_EXCEPTION(ISOCPP_PRECONDITION_NOT_MET_ERROR,
                            "handle is null");
    }
    ih = handle.delegate().handle();

    if (timestamp != dds::core::Time::invalid()) {
        dds_time_t ddsc_time = org::eclipse::cyclonedds::core::convertTime(timestamp);
        ret = dds_unregister_instance_ih_ts(writer, ih, ddsc_time);
    } else {
        ret = dds_unregister_instance_ih(writer, ih);
    }

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "unregister_instance failed.");
}

void
AnyDataWriterDelegate::unregister_instance(
    dds_entity_t writer,
    const void *data,
    const dds::core::Time& timestamp)
{
    dds_return_t ret;
    void *c_sample;

    if (data == NULL)   {
        ISOCPP_THROW_EXCEPTION(ISOCPP_PRECONDITION_NOT_MET_ERROR,
                               "data is null");
    }

    c_sample = dds_alloc(this->sampleSize);
    (*(this->copyIn))(data, c_sample);

    if (timestamp != dds::core::Time::invalid()) {
        dds_time_t ddsc_time = org::eclipse::cyclonedds::core::convertTime(timestamp);
        ret = dds_unregister_instance_ts(writer, c_sample, ddsc_time);
    } else {
        ret = dds_unregister_instance(writer, c_sample);
    }

    dds_free (c_sample);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "unregister failed.");
}

void
AnyDataWriterDelegate::dispose_instance(
    dds_entity_t writer,
    const dds::core::InstanceHandle& handle,
    const dds::core::Time& timestamp)
{
    dds_instance_handle_t ih;
    dds_return_t ret;

    if (handle == dds::core::null) {
      ISOCPP_THROW_EXCEPTION(ISOCPP_PRECONDITION_NOT_MET_ERROR,
                            "handle is null");
    }
    ih = handle.delegate().handle();

    if (timestamp != dds::core::Time::invalid()) {
        dds_time_t ddsc_time = org::eclipse::cyclonedds::core::convertTime(timestamp);
        ret = dds_dispose_ih_ts(writer, ih, ddsc_time);
    } else {
        ret = dds_dispose_ih(writer, ih);
    }

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dispose_instance failed.");
}

void
AnyDataWriterDelegate::dispose_instance(
    dds_entity_t writer,
    const void *data,
    const dds::core::Time& timestamp)
{
    dds_return_t ret;
    void *c_sample;

    if (data == NULL)   {
        ISOCPP_THROW_EXCEPTION(ISOCPP_PRECONDITION_NOT_MET_ERROR,
                               "data is null");
    }

    c_sample = dds_alloc(this->sampleSize);
    (*(this->copyIn))(data, c_sample);

    if (timestamp != dds::core::Time::invalid()) {
        dds_time_t ddsc_time = org::eclipse::cyclonedds::core::convertTime(timestamp);
        ret = dds_dispose_ts(writer, c_sample, ddsc_time);
    } else {
        ret = dds_dispose(writer, c_sample);
    }

    dds_free (c_sample);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dispose failed.");
}

void
AnyDataWriterDelegate::get_key_value(
    dds_entity_t writer,
    void *data,
    const dds::core::InstanceHandle& handle)
{
    void* cKey = dds_alloc (sampleSize);
    dds_return_t ret = dds_instance_get_key(writer, handle.delegate().handle(), cKey);
    copyOut(cKey, data);
    dds_free(cKey);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_instance_get_key failed.");
}

dds_instance_handle_t
AnyDataWriterDelegate::lookup_instance(
    dds_entity_t writer,
    const void *data)
{
    void *c_sample;
    c_sample = dds_alloc (sampleSize);
    (*copyIn)(data, c_sample);

    dds_instance_handle_t handle = dds_lookup_instance (writer, c_sample);
    dds_free (c_sample);
    return handle;
}

const ::dds::core::status::LivelinessLostStatus
AnyDataWriterDelegate::liveliness_lost_status()
{
    this->check();
    ::dds::core::status::LivelinessLostStatus result;
    dds_liveliness_lost_status_t ddsc_status;
    int ddsc_ret = dds_get_liveliness_lost_status (ddsc_entity, &ddsc_status);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_ret, "dds_get_liveliness_lost_status failed.");

    result.delegate().ddsc_status (&ddsc_status);
    return result;
}

const ::dds::core::status::OfferedDeadlineMissedStatus
AnyDataWriterDelegate::offered_deadline_missed_status()
{
    dds::core::status::OfferedDeadlineMissedStatus status;
    dds_offered_deadline_missed_status_t ddsc_status;
    int ddsc_ret = dds_get_offered_deadline_missed_status (ddsc_entity, &ddsc_status);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_ret, "dds_get_offered_deadline_missed_status failed.");

    status.delegate().ddsc_status (&ddsc_status);
    return status;
}


const ::dds::core::status::OfferedIncompatibleQosStatus
AnyDataWriterDelegate::offered_incompatible_qos_status()
{
    dds::core::status::OfferedIncompatibleQosStatus status;
    dds_offered_incompatible_qos_status_t ddsc_status;
    int ddsc_ret = dds_get_offered_incompatible_qos_status (ddsc_entity, &ddsc_status);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_ret, "dds_get_offered_incompatible_qos_status failed.");

    status.delegate().ddsc_status (&ddsc_status);
    return status;
}

const ::dds::core::status::PublicationMatchedStatus
AnyDataWriterDelegate::publication_matched_status()
{
    dds::core::status::PublicationMatchedStatus status;
    dds_publication_matched_status_t ddsc_status;
    int ddsc_ret = dds_get_publication_matched_status (ddsc_entity, &ddsc_status);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_ret, "dds_get_publication_matched_status failed.");

    status.delegate().ddsc_status (&ddsc_status);
    return status;
}

::dds::core::InstanceHandleSeq
AnyDataWriterDelegate::matched_subscriptions()
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");

    ::dds::core::InstanceHandleSeq handleSeq;
    return handleSeq;
}

const dds::topic::SubscriptionBuiltinTopicData
AnyDataWriterDelegate::matched_subscription_data(const ::dds::core::InstanceHandle& h)
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
    dds::topic::SubscriptionBuiltinTopicData dataSample;
    (void)h;
    return dataSample;
}

void
AnyDataWriterDelegate::assert_liveliness()
{
   ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

void
AnyDataWriterDelegate::wait_for_acknowledgments(
    const dds::core::Duration& timeout)
{
   ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
   (void)timeout;
}

dds::pub::TAnyDataWriter<AnyDataWriterDelegate>
AnyDataWriterDelegate::wrapper_to_any()
{
    AnyDataWriterDelegate::ref_type ref =
            OSPL_CXX11_STD_MODULE::dynamic_pointer_cast<AnyDataWriterDelegate>(this->get_strong_ref());
    dds::pub::AnyDataWriter any_writer(ref);
    return any_writer;
}

void
AnyDataWriterDelegate::write_flush()
{
    dds_write_flush (ddsc_entity);
}

void
AnyDataWriterDelegate::set_batch(bool enable)
{
    dds_write_set_batch (enable);
}

}
}
}
}
