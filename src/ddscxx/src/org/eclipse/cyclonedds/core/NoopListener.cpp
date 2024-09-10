// Copyright(c) 2024 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "org/eclipse/cyclonedds/core/NoopListener.hpp"

#ifdef _WIN32_DLL_
  #define DDS_FN_EXPORT __declspec (dllexport)
#else
  #define DDS_FN_EXPORT
#endif

namespace org { namespace eclipse { namespace cyclonedds { namespace core {

static void callback_on_inconsistent_topic_noop (dds_entity_t, dds_inconsistent_topic_status_t, void *) { }
static void callback_on_offered_deadline_missed_noop (dds_entity_t, dds_offered_deadline_missed_status_t, void *) { }
static void callback_on_offered_incompatible_qos_noop (dds_entity_t, dds_offered_incompatible_qos_status_t, void *) { }
static void callback_on_liveliness_lost_noop (dds_entity_t, dds_liveliness_lost_status_t, void *) { }
static void callback_on_publication_matched_noop (dds_entity_t, dds_publication_matched_status_t, void *) { }
static void callback_on_requested_deadline_missed_noop (dds_entity_t, dds_requested_deadline_missed_status_t, void *) { }
static void callback_on_requested_incompatible_qos_noop (dds_entity_t, dds_requested_incompatible_qos_status_t, void *) { }
static void callback_on_sample_rejected_noop (dds_entity_t, dds_sample_rejected_status_t, void *) { }
static void callback_on_liveliness_changed_noop (dds_entity_t, dds_liveliness_changed_status_t, void *) { }
static void callback_on_data_available_noop (dds_entity_t, void *) { }
static void callback_on_subscription_matched_noop (dds_entity_t, dds_subscription_matched_status_t, void *) { }
static void callback_on_sample_lost_noop (dds_entity_t, dds_sample_lost_status_t, void *) { }
static void callback_on_data_readers_noop (dds_entity_t, void *) { }

DDS_FN_EXPORT std::unique_ptr<dds_listener_t, std::function<void(dds_listener_t *)>> make_noop_listener()
{
    dds_listener_t *callbacks = dds_create_listener(nullptr);
    dds_lset_inconsistent_topic_arg(callbacks, callback_on_inconsistent_topic_noop, nullptr, false);
    dds_lset_offered_deadline_missed_arg(callbacks, callback_on_offered_deadline_missed_noop, nullptr, false);
    dds_lset_offered_incompatible_qos_arg(callbacks, callback_on_offered_incompatible_qos_noop, nullptr, false);
    dds_lset_liveliness_lost_arg(callbacks, callback_on_liveliness_lost_noop, nullptr, false);
    dds_lset_publication_matched_arg(callbacks, callback_on_publication_matched_noop, nullptr, false);
    dds_lset_requested_deadline_missed_arg(callbacks, callback_on_requested_deadline_missed_noop, nullptr, false);
    dds_lset_requested_incompatible_qos_arg(callbacks, callback_on_requested_incompatible_qos_noop, nullptr, false);
    dds_lset_sample_rejected_arg(callbacks, callback_on_sample_rejected_noop, nullptr, false);
    dds_lset_liveliness_changed_arg(callbacks, callback_on_liveliness_changed_noop, nullptr, false);
    dds_lset_data_available_arg(callbacks, callback_on_data_available_noop, nullptr, false);
    dds_lset_subscription_matched_arg(callbacks, callback_on_subscription_matched_noop, nullptr, false);
    dds_lset_sample_lost_arg(callbacks, callback_on_sample_lost_noop, nullptr, false);
    dds_lset_data_on_readers_arg(callbacks, callback_on_data_readers_noop, nullptr, false);
    return std::unique_ptr<dds_listener_t, std::function<void(dds_listener_t *)>>(callbacks, &dds_delete_listener);
}

} } } }
