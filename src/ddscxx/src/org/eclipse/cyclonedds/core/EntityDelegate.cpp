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

#include <org/eclipse/cyclonedds/core/EntityDelegate.hpp>
#include <org/eclipse/cyclonedds/sub/AnyDataReaderDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/core/MiscUtils.hpp>
#include <org/eclipse/cyclonedds/core/ListenerDispatcher.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>

#include <dds/core/cond/StatusCondition.hpp>
#include <dds/sub/AnyDataReaderListener.hpp>

#include "dds/dds.h"
#include "dds/ddsrt/sync.h"

#include <cassert>


org::eclipse::cyclonedds::core::EntityDelegate::EntityDelegate() :
  enabled_(false),
  listener_mask(0),
  listener_callbacks(NULL),
  listener(NULL)
{
  this->callback_mutex = dds_alloc (sizeof (ddsrt_mutex_t));
  this->callback_cond = dds_alloc (sizeof (ddsrt_cond_t));

  ddsrt_mutex_init (static_cast<ddsrt_mutex_t*>(this->callback_mutex));
  ddsrt_cond_init (static_cast<ddsrt_cond_t*>(this->callback_cond));

  callback_count = 0;
}

org::eclipse::cyclonedds::core::EntityDelegate::~EntityDelegate()
{
  if (this->listener_callbacks != NULL)
  {
    dds_delete_listener(this->listener_callbacks);
  }
  this->listener_callbacks = NULL;

  ddsrt_cond_destroy (static_cast<ddsrt_cond_t*>(this->callback_cond));
  ddsrt_mutex_destroy (static_cast<ddsrt_mutex_t*>(this->callback_mutex));
  dds_free (this->callback_cond);
  dds_free (this->callback_mutex);
}

void org::eclipse::cyclonedds::core::EntityDelegate::enable()
{
  dds_return_t ret;
  enabled_ = true;
  ret = dds_set_listener (this->ddsc_entity, this->listener_callbacks);
  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Could not set internal listener.");
}

::dds::core::status::StatusMask
org::eclipse::cyclonedds::core::EntityDelegate::status_changes() const
{
    ::dds::core::status::StatusMask mask;
    dds_return_t ret;
    uint32_t ddsc_mask;

    check();
    ret = dds_get_status_changes(ddsc_entity, &ddsc_mask);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Could not set internal listener.");
    mask = convertStatusMask(ddsc_mask);

    return mask;
}

::dds::core::InstanceHandle
org::eclipse::cyclonedds::core::EntityDelegate::instance_handle() const
{
    ::dds::core::InstanceHandle handle(::dds::core::null);
    dds_instance_handle_t ddsc_handle;
    dds_return_t ret;

    this->check();

    ret = dds_get_instance_handle(this->ddsc_entity, &ddsc_handle);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Failed get instance handle");
    handle = dds::core::InstanceHandle(ddsc_handle);

    return handle;
}

bool
org::eclipse::cyclonedds::core::EntityDelegate::contains_entity(
    const ::dds::core::InstanceHandle& handle)
{
    DDSCXX_UNUSED_ARG(handle);
    return false;
}

org::eclipse::cyclonedds::core::ObjectDelegate::ref_type
org::eclipse::cyclonedds::core::EntityDelegate::get_statusCondition()
{

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    org::eclipse::cyclonedds::core::ObjectDelegate::ref_type mySCObj = this->myStatusCondition.lock();
    if (!mySCObj) {
        dds::core::cond::TStatusCondition<org::eclipse::cyclonedds::core::cond::StatusConditionDelegate> mySC(
                new org::eclipse::cyclonedds::core::cond::StatusConditionDelegate(this, this->ddsc_entity));
        mySC.delegate()->init(mySC.delegate());
        this->myStatusCondition = mySC.delegate()->get_weak_ref();
        mySCObj = mySC.delegate()->get_strong_ref();
    }

    return mySCObj;
}


void
org::eclipse::cyclonedds::core::EntityDelegate::close()
{
    org::eclipse::cyclonedds::core::ObjectDelegate::ref_type mySCObj = this->myStatusCondition.lock();
    if (mySCObj) {
        mySCObj->close();
    }
    org::eclipse::cyclonedds::core::DDScObjectDelegate::close();
}

void
org::eclipse::cyclonedds::core::EntityDelegate::retain()
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

void
org::eclipse::cyclonedds::core::EntityDelegate::listener_set(
                 void *_listener,
                 const dds::core::status::StatusMask& mask)
{
    dds_listener_t *callbacks;
    this->listener = _listener;
    this->listener_mask = mask;

    callbacks = dds_create_listener(this);

    // Set topic callbacks
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::inconsistent_topic()))
    {
        dds_lset_inconsistent_topic(callbacks, callback_on_inconsistent_topic);
    }

    // Set writer callbacks
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::offered_deadline_missed()))
    {
        dds_lset_offered_deadline_missed(callbacks, callback_on_offered_deadline_missed);
    }
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::offered_incompatible_qos()))
    {
        dds_lset_offered_incompatible_qos(callbacks, callback_on_offered_incompatible_qos);
    }
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::liveliness_lost()))
    {
        dds_lset_liveliness_lost(callbacks, callback_on_liveliness_lost);
    }
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::publication_matched()))
    {
        dds_lset_publication_matched(callbacks, callback_on_publication_matched);
    }

    // Set reader callbacks
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::requested_deadline_missed()))
    {
        dds_lset_requested_deadline_missed(callbacks, callback_on_requested_deadline_missed);
    }
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::requested_incompatible_qos()))
    {
        dds_lset_requested_incompatible_qos(callbacks, callback_on_requested_incompatible_qos);
    }
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::sample_rejected()))
    {
        dds_lset_sample_rejected(callbacks, callback_on_sample_rejected);
    }
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::liveliness_changed()))
    {
        dds_lset_liveliness_changed(callbacks, callback_on_liveliness_changed);
    }
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::data_available()))
    {
        dds_lset_data_available(callbacks, callback_on_data_available);
    }
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::subscription_matched()))
    {
        dds_lset_subscription_matched(callbacks, callback_on_subscription_matched);
    }
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::sample_lost()))
    {
        dds_lset_sample_lost(callbacks, callback_on_sample_lost);
    }

    // Set subscriber callbacks
    if (STATUS_MASK_CONTAINS(mask, dds::core::status::StatusMask::data_on_readers()))
    {
        dds_lset_data_on_readers(callbacks, callback_on_data_readers);
    }

    // If entity enabled: set listener on ddsc entity
    if (this->enabled_)
    {
        dds_return_t ret;
        ret = dds_set_listener(this->ddsc_entity, callbacks);
        ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Setting listener failed.");
    }

    // Delete previous ddsc listener callbacks object
    if (this->listener_callbacks != NULL)
    {
        dds_delete_listener(this->listener_callbacks);
    }

    // Store new listener
    this->listener_callbacks = callbacks;
}

void * org::eclipse::cyclonedds::core::EntityDelegate::listener_get () const
{
  return this->listener;
}

void org::eclipse::cyclonedds::core::EntityDelegate::prevent_callbacks ()
{
  ddsrt_mutex_lock (static_cast<ddsrt_mutex_t*>(this->callback_mutex));

  if (this->get_weak_ref().expired () && (this->callback_count == 1))
  {
    // This condition leads to deadlock: the thread is a callback
    // thread, it has held the last reference to this object, the
    // reference has gone out of scope, so the destructor is
    // running, it has called close(), which is trying to prevent
    // further callbacks, so ends up here with the thread trying to
    // wait for itself.
    //
    // If the main thread calls close() explicitly, then this
    // situation is avoided.
    //
    assert (false);
  }

  while (callback_count > 0)
  {
    ddsrt_cond_wait (static_cast<ddsrt_cond_t*>(this->callback_cond), static_cast<ddsrt_mutex_t*>(this->callback_mutex));
  }
  callback_count = -1;

  ddsrt_mutex_unlock (static_cast<ddsrt_mutex_t*>(this->callback_mutex));
}

bool org::eclipse::cyclonedds::core::EntityDelegate::obtain_callback_lock ()
{
  bool result = false;

  ddsrt_mutex_lock (static_cast<ddsrt_mutex_t*>(this->callback_mutex));
  if (callback_count >= 0)
  {
    result = true;
    ++callback_count;
  }
  ddsrt_mutex_unlock (static_cast<ddsrt_mutex_t*>(this->callback_mutex));

  return result;
}

void org::eclipse::cyclonedds::core::EntityDelegate::release_callback_lock ()
{
    ddsrt_mutex_lock (static_cast<ddsrt_mutex_t*>(this->callback_mutex));

  --callback_count;
  if (callback_count == 0)
  {
    ddsrt_cond_broadcast (static_cast<ddsrt_cond_t*>(this->callback_cond));
  }

  ddsrt_mutex_unlock (static_cast<ddsrt_mutex_t*>(this->callback_mutex));
}

const dds::core::status::StatusMask
org::eclipse::cyclonedds::core::EntityDelegate::get_listener_mask () const
{
  return this->listener_mask;
}

// These are to satisfy the linker, they should never be called
void org::eclipse::cyclonedds::core::EntityDelegate::on_inconsistent_topic
  (dds_entity_t, org::eclipse::cyclonedds::core::InconsistentTopicStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_offered_deadline_missed
  (dds_entity_t, org::eclipse::cyclonedds::core::OfferedDeadlineMissedStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_offered_incompatible_qos
  (dds_entity_t, org::eclipse::cyclonedds::core::OfferedIncompatibleQosStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_liveliness_lost
  (dds_entity_t, org::eclipse::cyclonedds::core::LivelinessLostStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_publication_matched
  (dds_entity_t, org::eclipse::cyclonedds::core::PublicationMatchedStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_requested_deadline_missed
  (dds_entity_t, org::eclipse::cyclonedds::core::RequestedDeadlineMissedStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_requested_incompatible_qos
  (dds_entity_t, org::eclipse::cyclonedds::core::RequestedIncompatibleQosStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_sample_rejected
  (dds_entity_t, org::eclipse::cyclonedds::core::SampleRejectedStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_liveliness_changed
  (dds_entity_t, org::eclipse::cyclonedds::core::LivelinessChangedStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_data_available (dds_entity_t)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_subscription_matched
  (dds_entity_t, org::eclipse::cyclonedds::core::SubscriptionMatchedStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_sample_lost
  (dds_entity_t, org::eclipse::cyclonedds::core::SampleLostStatusDelegate &)
{
  assert (false);
}

void org::eclipse::cyclonedds::core::EntityDelegate::on_data_readers(dds_entity_t)
{
  assert (false);
}
