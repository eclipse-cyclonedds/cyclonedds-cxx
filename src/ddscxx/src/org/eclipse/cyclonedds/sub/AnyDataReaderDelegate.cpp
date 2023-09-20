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

#include <dds/sub/AnyDataReader.hpp>

#include <org/eclipse/cyclonedds/sub/QueryDelegate.hpp>
#include <org/eclipse/cyclonedds/sub/AnyDataReaderDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/core/MiscUtils.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>
#include <dds/sub/status/detail/DataStateImpl.hpp>
#include <org/eclipse/cyclonedds/topic/BuiltinTopicCopy.hpp>

#include "dds/dds.h"
#include "dds/ddsc/dds_internal_api.h"

#define NORMALIZE_LENGTH(maxs) maxs == static_cast<uint32_t>(dds::core::LENGTH_UNLIMITED) ? static_cast<uint32_t>(INT32_MAX) : maxs

dds::sub::SampleInfo
dds::sub::detail::SamplesHolder::sample_info_from_c(const dds_sample_info_t *si)
{
    return dds::sub::SampleInfo(org::eclipse::cyclonedds::sub::SampleInfoImpl(si));
}


namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace sub
{

struct ReaderCopyInfo {
    const org::eclipse::cyclonedds::sub::AnyDataReaderDelegate *helper;
    const void *key;
};


/* For dynamic casting to AnyDataReaderDelegate to work for a few older compilers,
 * it is needed that (at least) the constructor is moved to the cpp file. */
AnyDataReaderDelegate::AnyDataReaderDelegate(
        const dds::sub::qos::DataReaderQos& qos,
        const dds::topic::TopicDescription& td)
  : qos_(qos), td_(td), sample_(0)
{
}

AnyDataReaderDelegate::~AnyDataReaderDelegate()
{
}

const dds::topic::TopicDescription&
AnyDataReaderDelegate::topic_description() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return this->td_;
}

dds::sub::qos::DataReaderQos
AnyDataReaderDelegate::qos() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return qos_;
}


void
AnyDataReaderDelegate::qos(const dds::sub::qos::DataReaderQos& qos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    qos->check();
    dds_qos_t* ddsc_qos = qos.delegate().ddsc_qos();
    dds_return_t ret = dds_set_qos(ddsc_entity, ddsc_qos);
    dds_delete_qos(ddsc_qos);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Could not set reader qos.");
    this->qos_ = qos;
}

void
AnyDataReaderDelegate::wait_for_historical_data(const dds::core::Duration& timeout)
{
    this->check();
    dds_duration_t ddsc_timeout = org::eclipse::cyclonedds::core::convertDuration(timeout);
    dds_return_t ret = dds_reader_wait_for_historical_data(ddsc_entity, ddsc_timeout);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_reader_wait_for_historical_data failed.");
}


void
AnyDataReaderDelegate::reset_data_available()
{
}

uint32_t
AnyDataReaderDelegate::get_ddsc_state_mask(const dds::sub::status::DataState& state)
{
    /* Translate DataState to sample, view and instance ulong states. */
    unsigned long s_state = state.sample_state().to_ulong();
    unsigned long v_state = state.view_state().to_ulong();
    unsigned long i_state = state.instance_state().to_ulong();

    /* Truncate 'any' status to specific bits. */
    s_state &= 0x3;
    v_state &= 0x3;
    i_state &= 0x7;

    /*
     * The IsoCpp state bits should match the ddsc state bits.
     * The only difference is the location within the uint32.
     * So, perform a shift to let the IsoCpp bits match ddsc bits.
     */
    /* s_state <<= 0; */
    v_state <<= 2;
    i_state <<= 4;

    /* The mask is all states or-ed. */
    return static_cast<uint32_t>(s_state | v_state | i_state);
}

dds_return_t
AnyDataReaderDelegate::collector_callback_fn (
    void *arg,
    const dds_sample_info_t *si,
    const struct ddsi_sertype *,
    struct ddsi_serdata *sd)
{
    dds::sub::detail::SamplesHolder *sh = reinterpret_cast<dds::sub::detail::SamplesHolder *>(arg);
    sh->append_sample(sd, si);
    return DDS_RETCODE_OK;
}


bool
AnyDataReaderDelegate::is_loan_supported(const dds_entity_t reader) const
{
    DDSRT_WARNING_DEPRECATED_OFF
    return dds_is_loan_available(reader);
    DDSRT_WARNING_DEPRECATED_ON
}

void
AnyDataReaderDelegate::read_cdr(
    const dds_entity_t reader,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    dds_return_t ret;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    /* The reader can also be a condition. */
    ret = dds_read_with_collector(reader,
                                  NORMALIZE_LENGTH(requested_max_samples),
                                  DDS_HANDLE_NIL,
                                  ddsc_mask,
                                  collector_callback_fn,
                                  &samples);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
}

void
AnyDataReaderDelegate::take_cdr(
    const dds_entity_t reader,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    dds_return_t ret;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    /* The reader can also be a condition. */
    ret = dds_take_with_collector(reader,
                                  NORMALIZE_LENGTH(requested_max_samples),
                                  DDS_HANDLE_NIL,
                                  ddsc_mask,
                                  collector_callback_fn,
                                  &samples);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
}

void
AnyDataReaderDelegate::loaned_read(
    const dds_entity_t reader,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    dds_return_t ret;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    /* The reader can also be a condition. */
    ret = dds_read_with_collector(reader,
                                  NORMALIZE_LENGTH(requested_max_samples),
                                  DDS_HANDLE_NIL,
                                  ddsc_mask,
                                  collector_callback_fn,
                                  &samples);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
}


void
AnyDataReaderDelegate::loaned_take(
    const dds_entity_t reader,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    dds_return_t ret;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    /* The reader can also be a condition. */
    ret = dds_take_with_collector(reader,
                                  NORMALIZE_LENGTH(requested_max_samples),
                                  DDS_HANDLE_NIL,
                                  ddsc_mask,
                                  collector_callback_fn,
                                  &samples);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
}

void
AnyDataReaderDelegate::loaned_read_instance(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    dds_return_t ret;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    /* The reader can also be a condition. */
    ret = dds_read_with_collector(reader,
                                  NORMALIZE_LENGTH(requested_max_samples),
                                  handle->handle(),
                                  ddsc_mask,
                                  collector_callback_fn,
                                  &samples);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
}

void
AnyDataReaderDelegate::loaned_take_instance(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    dds_return_t ret;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    /* The reader can also be a condition. */
    ret = dds_take_with_collector(reader,
                                  NORMALIZE_LENGTH(requested_max_samples),
                                  handle->handle(),
                                  ddsc_mask,
                                  collector_callback_fn,
                                  &samples);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
}

void
AnyDataReaderDelegate::loaned_read_next_instance(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t max_samples)
{
    (void)reader;
    (void)handle;
    (void)mask;
    (void)samples;
    (void)max_samples;
    /* Probably use dds_read_next(). Doesn't support conditions though... */
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Read next instance not currently supported");
}

void
AnyDataReaderDelegate::loaned_take_next_instance(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t max_samples)
{
    (void)reader;
    (void)handle;
    (void)mask;
    (void)samples;
    (void)max_samples;
    /* Probably use dds_take_next(). Doesn't support conditions though... */
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Take next instance not currently supported");
}

void
AnyDataReaderDelegate::read(
    const dds_entity_t reader,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    dds_return_t ret;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    /* The reader can also be a condition. */
    ret = dds_read_with_collector(reader, NORMALIZE_LENGTH(requested_max_samples), DDS_HANDLE_NIL, ddsc_mask, collector_callback_fn, &samples);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
}


void
AnyDataReaderDelegate::take(
    const dds_entity_t reader,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    dds_return_t ret;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    /* The reader can also be a condition. */
    ret = dds_take_with_collector(reader, NORMALIZE_LENGTH(requested_max_samples), DDS_HANDLE_NIL, ddsc_mask, collector_callback_fn, &samples);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
}

void
AnyDataReaderDelegate::read_instance(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    dds_return_t ret;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    /* The reader can also be a condition. */
    ret = dds_read_with_collector(reader, NORMALIZE_LENGTH(requested_max_samples), handle->handle(), ddsc_mask, collector_callback_fn, &samples);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
}

void
AnyDataReaderDelegate::take_instance(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    dds_return_t ret;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    /* The reader can also be a condition. */
    ret = dds_take_with_collector(reader, NORMALIZE_LENGTH(requested_max_samples), handle->handle(), ddsc_mask, collector_callback_fn, &samples);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
}

void
AnyDataReaderDelegate::read_next_instance(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t max_samples)
{
    (void)reader;
    (void)handle;
    (void)mask;
    (void)samples;
    (void)max_samples;
    /* Probably use dds_read_next(). Doesn't support conditions though... */
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Read next instance not currently supported");
}

void
AnyDataReaderDelegate::take_next_instance(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t max_samples)
{
    (void)reader;
    (void)handle;
    (void)mask;
    (void)samples;
    (void)max_samples;
    /* Probably use dds_take_next(). Doesn't support conditions though... */
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Take next instance not currently supported");
}

void
AnyDataReaderDelegate::get_key_value(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    void *key)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    dds_return_t ret = dds_instance_get_key(reader, handle.delegate().handle(), key);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_instance_get_key failed.");
}

dds_instance_handle_t AnyDataReaderDelegate::lookup_instance
  (const dds_entity_t reader, const void *key) const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return dds_lookup_instance(reader, key);
}

dds::core::status::LivelinessChangedStatus
AnyDataReaderDelegate::liveliness_changed_status()
{
    dds::core::status::LivelinessChangedStatus status;
    dds_liveliness_changed_status_t cStatus;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds_return_t ret = dds_get_liveliness_changed_status(
                                    ddsc_entity,
                                    &cStatus);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_get_liveliness_changed_status failed.");
    status->ddsc_status(&cStatus);

    return status;
}

dds::core::status::SampleRejectedStatus
AnyDataReaderDelegate::sample_rejected_status()
{
    dds::core::status::SampleRejectedStatus status;
    dds_sample_rejected_status_t cStatus;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds_return_t ret = dds_get_sample_rejected_status(
                                    ddsc_entity,
                                    &cStatus);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_get_sample_rejected_status failed.");
    status->ddsc_status(&cStatus);

    return status;
}

dds::core::status::SampleLostStatus
AnyDataReaderDelegate::sample_lost_status()
{
    dds::core::status::SampleLostStatus status;
    dds_sample_lost_status_t cStatus;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds_return_t ret = dds_get_sample_lost_status(
                                    ddsc_entity,
                                    &cStatus);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_get_sample_lost_status failed.");
    status->ddsc_status(&cStatus);

    return status;
}

dds::core::status::RequestedDeadlineMissedStatus
AnyDataReaderDelegate::requested_deadline_missed_status()
{
    dds::core::status::RequestedDeadlineMissedStatus status;
    dds_requested_deadline_missed_status_t cStatus;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds_return_t ret = dds_get_requested_deadline_missed_status(
                                    ddsc_entity,
                                    &cStatus);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_get_requested_deadline_missed_status failed.");
    status->ddsc_status(&cStatus);

    return status;
}

dds::core::status::RequestedIncompatibleQosStatus
AnyDataReaderDelegate::requested_incompatible_qos_status()
{
    dds::core::status::RequestedIncompatibleQosStatus status;
    dds_requested_incompatible_qos_status_t cStatus;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds_return_t ret = dds_get_requested_incompatible_qos_status(
                                    ddsc_entity,
                                    &cStatus);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_get_requested_incompatible_qos_status failed.");
    status->ddsc_status(&cStatus);

    return status;
}

dds::core::status::SubscriptionMatchedStatus
AnyDataReaderDelegate::subscription_matched_status()
{
    dds::core::status::SubscriptionMatchedStatus status;
    dds_subscription_matched_status_t cStatus;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds_return_t ret = dds_get_subscription_matched_status(
                                    ddsc_entity,
                                    &cStatus);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_get_subscription_matched_status failed.");
    status->ddsc_status(&cStatus);

    return status;
}

::dds::core::InstanceHandleSeq
AnyDataReaderDelegate::matched_publications()
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
    ::dds::core::InstanceHandleSeq handleSeq;
    return handleSeq;
}

const dds::topic::PublicationBuiltinTopicData
AnyDataReaderDelegate::matched_publication_data(const ::dds::core::InstanceHandle& h)
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
    dds::topic::PublicationBuiltinTopicData dataSample;
    (void)h;
    return dataSample;
}

void
AnyDataReaderDelegate::close()
{
    this->queries.all_close();

    org::eclipse::cyclonedds::core::EntityDelegate::close();
}

void
AnyDataReaderDelegate::add_query(
    org::eclipse::cyclonedds::sub::QueryDelegate& query)
{
    this->queries.insert(query);
}

void
AnyDataReaderDelegate::remove_query(
    org::eclipse::cyclonedds::sub::QueryDelegate& query)
{
    this->queries.erase(query);
}

dds::sub::TAnyDataReader<AnyDataReaderDelegate>
AnyDataReaderDelegate::wrapper_to_any()
{
    AnyDataReaderDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<AnyDataReaderDelegate>(this->get_strong_ref());
    dds::sub::AnyDataReader any_reader(ref);
    return any_reader;
}

void AnyDataReaderDelegate::setSample(void* sample)
{
    sample_ = sample;
}

void* AnyDataReaderDelegate::getSample() const
{
    return sample_;
}

}
}
}
}
