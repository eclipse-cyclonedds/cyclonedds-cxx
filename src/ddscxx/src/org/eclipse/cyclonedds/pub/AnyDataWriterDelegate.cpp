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

#include <dds/pub/AnyDataWriter.hpp>
#include <org/eclipse/cyclonedds/pub/AnyDataWriterDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/core/MiscUtils.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>
#include <org/eclipse/cyclonedds/topic/BuiltinTopicCopy.hpp>
#include <dds/dds.h>
#include <dds/ddsc/dds_loan_api.h>

#include "dds/ddsi/ddsi_protocol.h"
#include "dds/features.hpp"

#ifdef DDSCXX_HAS_SHM
#include <dds/ddsi/ddsi_shm_transport.h>
#endif


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
    : qos_(qos), td_(td)
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
AnyDataWriterDelegate::write_cdr(
    dds_entity_t writer,
    const org::eclipse::cyclonedds::topic::CDRBlob *data,
    const dds::core::InstanceHandle& handle,
    const dds::core::Time& timestamp,
    uint32_t statusinfo)
{
    dds_return_t ret;
    struct ddsi_serdata *ser_data;
    ddsrt_iovec_t blob_holders[2];

    /* Ignore the handle until ddsc supports writes with instance handles. */
    (void)handle;

    /* Create an array of ddsrt_iovec_t to contain both the encoding and the CDR payload. */
    blob_holders[0].iov_len = 4;
    blob_holders[0].iov_base = const_cast<char *>(data->encoding().data());
    blob_holders[1].iov_len = static_cast<ddsrt_iov_len_t>(data->payload().size());
    blob_holders[1].iov_base = const_cast<uint8_t *>(data->payload().data());

    /* Now create a dedicated ser_data that contains both encoding and payload as contiguous memory. */
    ser_data = ddsi_serdata_from_ser_iov(
        td_->get_ser_type(),
        static_cast<ddsi_serdata_kind>(data->kind()),
        2,
        blob_holders,
        data->payload().size() + 4);

    ser_data->statusinfo = statusinfo;

    // if shared memory is supported by the writer
    if(dds_is_shared_memory_available(writer)) {
#ifdef DDSCXX_HAS_SHM
        void *iox_chunk ;
        // request a loan from the shared memory buffer
        ret = dds_loan_shared_memory_buffer(writer,
                                            data->payload().size() + data->encoding().size(),  // include the size for the encoding (CDR header)
                                            &iox_chunk);
        ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "write_cdr - Loaning of chunk failed");
        ISOCPP_BOOL_CHECK_AND_THROW(iox_chunk, ISOCPP_NULL_REFERENCE_ERROR, "write_cdr - Loaning of chunk failed");
        // copy the header
        memcpy(iox_chunk, data->encoding().data(), data->encoding().size());
        // copy the actual data
        memcpy(static_cast<unsigned char *>(iox_chunk) + data->encoding().size(),
               data->payload().data(), data->payload().size());
        // update SHM data state to serialized, since this API is used only to publish the serialized data
        shm_set_data_state(iox_chunk, IOX_CHUNK_CONTAINS_SERIALIZED_DATA);
        // update the loaned iox chunk in serdata
        ser_data->iox_chunk = iox_chunk;
#endif
    }

    if (timestamp != dds::core::Time::invalid()) {
        dds_time_t ddsc_time = org::eclipse::cyclonedds::core::convertTime(timestamp);
        ser_data->timestamp.v = ddsc_time;
        ret = dds_forwardcdr(writer, ser_data);
    } else {
        ret = dds_writecdr(writer, ser_data);
    }

#ifdef DDSCXX_HAS_SHM
    if (ret != DDS_RETCODE_OK) {
        if (ser_data->iox_chunk != nullptr) {
            // write cdr failed, so free the chunk
            ret = dds_return_loan(writer, &ser_data->iox_chunk, 1);
            ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "write_cdr, freeing loan failed");
        }
        ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "write_cdr failed.");
    }
#else
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "write_cdr failed.");
#endif
}

void
AnyDataWriterDelegate::write_cdr(
    dds_entity_t writer,
    const org::eclipse::cyclonedds::topic::CDRBlob *data,
    const dds::core::InstanceHandle& handle,
    const dds::core::Time& timestamp)
{
    this->write_cdr(writer, data, handle, timestamp, 0);
}

void
AnyDataWriterDelegate::dispose_cdr(
    dds_entity_t writer,
    const org::eclipse::cyclonedds::topic::CDRBlob *data,
    const dds::core::InstanceHandle& handle,
    const dds::core::Time& timestamp)
{
    this->write_cdr(writer, data, handle, timestamp, DDSI_STATUSINFO_DISPOSE);
}

void
AnyDataWriterDelegate::unregister_instance_cdr(
    dds_entity_t writer,
    const org::eclipse::cyclonedds::topic::CDRBlob *data,
    const dds::core::InstanceHandle& handle,
    const dds::core::Time& timestamp)
{
    this->write_cdr(writer, data, handle, timestamp, DDSI_STATUSINFO_UNREGISTER);
}

bool
AnyDataWriterDelegate::is_loan_supported(const dds_entity_t writer)
{
  return dds_is_loan_available(writer);
}

void
AnyDataWriterDelegate::loan_sample(
    dds_entity_t writer,
    void **sample)
{
    dds_return_t ret;

    ret = dds_loan_sample(writer, sample);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "sample loan failed.");
}

void
AnyDataWriterDelegate::return_loan(
    dds_entity_t writer,
    void *sample)
{
    dds_return_t ret;

    ret = dds_return_loan(writer, &sample, 1);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "return of sample loan failed.");
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

    /* Ignore the handle until ddsc supports writes with instance handles. */
    (void)handle;

    if (timestamp != dds::core::Time::invalid()) {
        dds_time_t ddsc_time = org::eclipse::cyclonedds::core::convertTime(timestamp);
        ret = dds_writedispose_ts(writer, data, ddsc_time);
    } else {
        ret = dds_writedispose(writer, data);
    }

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "writedispose failed.");
}

dds_instance_handle_t
AnyDataWriterDelegate::register_instance(
    dds_entity_t writer,
    const void *data,
    const dds::core::Time& timestamp)
{
    if (timestamp != dds::core::Time::invalid()) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR,
                               "Registering with a timestamp is not supported.");
    }

    dds_instance_handle_t ih;
    dds_return_t ret = dds_register_instance(writer, &ih, data);

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

    if (data == NULL)   {
        ISOCPP_THROW_EXCEPTION(ISOCPP_PRECONDITION_NOT_MET_ERROR,
                               "data is null");
    }

    if (timestamp != dds::core::Time::invalid()) {
        dds_time_t ddsc_time = org::eclipse::cyclonedds::core::convertTime(timestamp);
        ret = dds_unregister_instance_ts(writer, data, ddsc_time);
    } else {
        ret = dds_unregister_instance(writer, data);
    }

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

    if (data == NULL)   {
        ISOCPP_THROW_EXCEPTION(ISOCPP_PRECONDITION_NOT_MET_ERROR,
                               "data is null");
    }

    if (timestamp != dds::core::Time::invalid()) {
        dds_time_t ddsc_time = org::eclipse::cyclonedds::core::convertTime(timestamp);
        ret = dds_dispose_ts(writer, data, ddsc_time);
    } else {
        ret = dds_dispose(writer, data);
    }

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dispose failed.");
}

void
AnyDataWriterDelegate::get_key_value(
    dds_entity_t writer,
    void *data,
    const dds::core::InstanceHandle& handle)
{
    dds_return_t ret = dds_instance_get_key(writer, handle.delegate().handle(), data);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_instance_get_key failed.");
}

dds_instance_handle_t
AnyDataWriterDelegate::lookup_instance(
    dds_entity_t writer,
    const void *data)
{
    return dds_lookup_instance(writer, data);
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
            ::std::dynamic_pointer_cast<AnyDataWriterDelegate>(this->get_strong_ref());
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
DDSRT_WARNING_DEPRECATED_OFF
    dds_write_set_batch (enable);
DDSRT_WARNING_DEPRECATED_ON
}

}
}
}
}
