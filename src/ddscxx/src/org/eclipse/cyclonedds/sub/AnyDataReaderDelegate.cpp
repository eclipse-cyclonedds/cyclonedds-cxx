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
  : copyIn(NULL), copyOut(NULL), sampleSize(0), qos_(qos), td_(td), sample_(0)
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
    return (uint32_t)(s_state | v_state | i_state);
}

bool AnyDataReaderDelegate::init_samples_buffers(
                    const uint32_t            requested_max_samples,
                          uint32_t&           samples_to_read_cnt,
                          size_t&             c_sample_pointers_size,
                          void**&             c_sample_pointers,
                          dds_sample_info_t*& c_sample_infos)
{
    if(requested_max_samples == (uint32_t)dds::core::LENGTH_UNLIMITED)
    {
        //TODO: fix this when cyclonedds has been upgraded.
        /*
         * CycloneDDS doesn't support a read/take that just returns all
         * available samples. The caller should always prepare a array to
         * hold pointers to the samples.
         *
         * To solve the 'return all available samples with a read/take' problem
         * in the past, dds_reader_lock_samples() and DDS_READ_WITHOUT_LOCK
         * were added to the predecessor of cyclonedds and are still available.
         *
         *      - In short, dds_reader_lock_samples() locks the reader history
         *        cache and returns the number of available samples.
         *      - The locking is done to be sure that after getting the number
         *        of samples, no extra samples could be added before actually
         *        reading the samples. If samples were added when the C++
         *        binding doesn't know about that, it thinks that it read all
         *        samples, and will possibly wait for a 'data available'
         *        trigger that never comes because there are still samples in
         *        the rhc.
         *      - The C++ binding will prepare an array that is large enough to
         *        hold all samples.
         *      - Now, dds_read() or dds_take() is called from C++ with the
         *        DDS_READ_WITHOUT_LOCK value as max size. This indicates to
         *        cyclonedds that the rhc is already locked.
         *      - When dds_read() or dds_take() is done, the rhc is unlocked
         *        (as per usual) and is empty.
         *
         * This un-documented kludgy 'feature' of cyclonedds will probably be
         * fixed in the future (see also cyclonedds issue 74). When that's the
         * case, this kludge usage in C++ should be removed.
         */
        c_sample_pointers_size = dds_reader_lock_samples(ddsc_entity);
        samples_to_read_cnt = DDS_READ_WITHOUT_LOCK;
    }
    else
    {
        c_sample_pointers_size = (size_t)requested_max_samples;
        samples_to_read_cnt    = requested_max_samples;
    }

    /* Prepare the buffers. */
    if (c_sample_pointers_size)
    {
      c_sample_pointers = new void * [c_sample_pointers_size];
      c_sample_infos = new dds_sample_info_t[c_sample_pointers_size];
      /*
       * Unfortunately, the kludge doesn't work properly with read/take with
       * loan. So, prepare sample memory up-front. This will cause performance
       * decrease due to an extra copy action. Oh, well.
       */
      for (size_t i = 0; i < c_sample_pointers_size; i++) {
          c_sample_pointers[i] = dds_alloc(this->getSampleSize());
      }
    }

    return (c_sample_pointers_size > 0);
}


void AnyDataReaderDelegate::copy_samples(
                    dds::sub::detail::SamplesHolder& samples,
                    void**& c_sample_pointers,
                    dds_sample_info_t*& c_sample_infos,
                    int num_read)
{
    if (num_read > 0) {
        samples.set_length((uint32_t)num_read);
        for(int i = 0; i < num_read; i++)
        {
            copyOut(c_sample_pointers[i], samples.data());
            copy_sample_info(c_sample_infos[i], samples.info());
            samples++;
        }
    } else {
        samples.set_length(0);
    }
}


void AnyDataReaderDelegate::fini_samples_buffers(
                    void**& c_sample_pointers,
                    dds_sample_info_t*& c_sample_infos,
                    size_t c_sample_pointers_size)
{
    for(size_t i = 0; i < c_sample_pointers_size; i++)
    {
        dds_sample_free(c_sample_pointers[i], this->getDescriptor(), DDS_FREE_ALL);
    }
    delete [] c_sample_infos;
    delete [] c_sample_pointers;
}


void
AnyDataReaderDelegate::read(
    const dds_entity_t reader,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    void ** c_sample_pointers = NULL;
    dds_sample_info_t * c_sample_infos = NULL;
    size_t c_sample_pointers_size = 0;
    uint32_t samples_to_read_cnt = 0;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);
    bool expect_samples;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    expect_samples = this->init_samples_buffers(
                               requested_max_samples,
                               samples_to_read_cnt,
                               c_sample_pointers_size,
                               c_sample_pointers,
                               c_sample_infos);

    if (expect_samples)
    {
      dds_return_t ret;
      /* The reader can also be a condition. */
      ret = dds_read_mask(reader,
                          c_sample_pointers,
                          c_sample_infos,
                          c_sample_pointers_size,
                          samples_to_read_cnt,
                          ddsc_mask);

      copy_samples(samples, c_sample_pointers, c_sample_infos, (int)ret);
      fini_samples_buffers(c_sample_pointers, c_sample_infos, c_sample_pointers_size);
      ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
    }
}


void
AnyDataReaderDelegate::take(
    const dds_entity_t reader,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    void ** c_sample_pointers = NULL;
    dds_sample_info_t * c_sample_infos = NULL;
    size_t c_sample_pointers_size = 0;
    uint32_t samples_to_read_cnt = 0;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);
    bool expect_samples;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    expect_samples = this->init_samples_buffers(
                               requested_max_samples,
                               samples_to_read_cnt,
                               c_sample_pointers_size,
                               c_sample_pointers,
                               c_sample_infos);

    if (expect_samples)
    {
        dds_return_t ret;
        /* The reader can also be a condition. */
        ret = dds_take_mask(reader,
                            c_sample_pointers,
                            c_sample_infos,
                            c_sample_pointers_size,
                            samples_to_read_cnt,
                            ddsc_mask);

        copy_samples(samples, c_sample_pointers, c_sample_infos, (int)ret);
        fini_samples_buffers(c_sample_pointers, c_sample_infos, c_sample_pointers_size);
        ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
    }
}

void
AnyDataReaderDelegate::read_instance(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    void ** c_sample_pointers = NULL;
    dds_sample_info_t * c_sample_infos = NULL;
    size_t c_sample_pointers_size = 0;
    uint32_t samples_to_read_cnt = 0;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);
    bool expect_samples;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    expect_samples = this->init_samples_buffers(
                               requested_max_samples,
                               samples_to_read_cnt,
                               c_sample_pointers_size,
                               c_sample_pointers,
                               c_sample_infos);

    if (expect_samples)
    {
      dds_return_t ret;
      /* The reader can also be a condition. */
      ret = dds_read_instance_mask(reader,
                                   c_sample_pointers,
                                   c_sample_infos,
                                   c_sample_pointers_size,
                                   samples_to_read_cnt,
                                   handle->handle(),
                                   ddsc_mask );

      copy_samples(samples, c_sample_pointers, c_sample_infos, (int)ret);
      fini_samples_buffers(c_sample_pointers, c_sample_infos, c_sample_pointers_size);
      ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
    }
}

void
AnyDataReaderDelegate::take_instance(
    const dds_entity_t reader,
    const dds::core::InstanceHandle& handle,
    const dds::sub::status::DataState& mask,
    dds::sub::detail::SamplesHolder& samples,
    uint32_t requested_max_samples)
{
    void ** c_sample_pointers = NULL;
    dds_sample_info_t * c_sample_infos = NULL;
    size_t c_sample_pointers_size = 0;
    uint32_t samples_to_read_cnt = 0;
    uint32_t ddsc_mask = get_ddsc_state_mask(mask);
    bool expect_samples;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    expect_samples = this->init_samples_buffers(
                               requested_max_samples,
                               samples_to_read_cnt,
                               c_sample_pointers_size,
                               c_sample_pointers,
                               c_sample_infos);

    if (expect_samples)
    {
        dds_return_t ret;
        /* The reader can also be a condition. */
        ret = dds_take_instance_mask(reader,
                                     c_sample_pointers,
                                     c_sample_infos,
                                     c_sample_pointers_size,
                                     samples_to_read_cnt,
                                     handle->handle(),
                                     ddsc_mask );

      copy_samples(samples, c_sample_pointers, c_sample_infos, (int)ret);
      fini_samples_buffers(c_sample_pointers, c_sample_infos, c_sample_pointers_size);
      ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Getting sample failed.");
    }
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
    dds_return_t ret;
    void *cKey;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    cKey = dds_alloc (sampleSize);
    ret = dds_instance_get_key(reader, handle.delegate().handle(), cKey);
    copyOut(cKey, key);
    dds_free(cKey);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "dds_instance_get_key failed.");
}

dds_instance_handle_t AnyDataReaderDelegate::lookup_instance
  (const dds_entity_t reader, const void *key) const
{
  dds_instance_handle_t handle;
  void *cKey;

  org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
  this->check();

  cKey = dds_alloc (sampleSize);
  copyIn (key, cKey);
  handle = dds_lookup_instance (reader, cKey);
  dds_free (cKey);

  return handle;
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

void
AnyDataReaderDelegate::copy_sample_info(
    dds_sample_info_t &from,
    dds::sub::SampleInfo *to)
{
    org::eclipse::cyclonedds::sub::SampleInfoImpl& info = to->delegate();

    info.timestamp(org::eclipse::cyclonedds::core::convertTime(from.source_timestamp));

    dds::sub::status::SampleState ss;
    dds::sub::status::ViewState vs;
    dds::sub::status::InstanceState is;
    switch(from.sample_state)
    {
    case DDS_SST_READ:
        ss = dds::sub::status::SampleState::read();
        break;

    case DDS_SST_NOT_READ:
        ss = dds::sub::status::SampleState::not_read();
        break;
    }

    switch(from.view_state)
    {
    case DDS_VST_NEW:
        vs = dds::sub::status::ViewState::new_view();
        break;

    case DDS_VST_OLD:
        vs = dds::sub::status::ViewState::not_new_view();
        break;
    }

    switch(from.instance_state)
    {
    case DDS_IST_ALIVE:
        is = dds::sub::status::InstanceState::alive();
        break;

    case DDS_IST_NOT_ALIVE_DISPOSED:
        is = dds::sub::status::InstanceState::not_alive_disposed();
        break;

    case DDS_IST_NOT_ALIVE_NO_WRITERS:
        is = dds::sub::status::InstanceState::not_alive_no_writers();
        break;
    }
    info.state(dds::sub::status::DataState(ss, vs, is));

    info.generation_count().delegate() = org::eclipse::cyclonedds::sub::GenerationCountImpl(
                                    (int32_t)from.disposed_generation_count,
                                    (int32_t)from.no_writers_generation_count);
    info.rank().delegate() = org::eclipse::cyclonedds::sub::RankImpl(
                                    (int32_t)from.sample_rank,
                                    (int32_t)from.generation_rank,
                                    (int32_t)from.absolute_generation_rank);
    info.valid(from.valid_data);
    dds::core::InstanceHandle ih(from.instance_handle);
    info.instance_handle(ih);
    dds::core::InstanceHandle ph(from.publication_handle);
    info.publication_handle(ph);
}

dds::sub::TAnyDataReader<AnyDataReaderDelegate>
AnyDataReaderDelegate::wrapper_to_any()
{
    AnyDataReaderDelegate::ref_type ref =
            OSPL_CXX11_STD_MODULE::dynamic_pointer_cast<AnyDataReaderDelegate>(this->get_strong_ref());
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
