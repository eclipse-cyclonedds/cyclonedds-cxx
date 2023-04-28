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

#include <org/eclipse/cyclonedds/core/MiscUtils.hpp>

#include "dds/dds.h"
#include "dds/ddsrt/string.h"

void
org::eclipse::cyclonedds::core::convertByteSeq(
        const dds::core::ByteSeq &from,
        void*& to,
        int32_t  size)
{
    uint8_t *byteArray;

    if (to != NULL) {
        dds_free(to);
        to = NULL;
    }

    if (size > 0) {
        byteArray = static_cast<uint8_t*>(dds_alloc(static_cast<size_t>(size) * sizeof(uint8_t)));
        if (!byteArray) {
            ISOCPP_THROW_EXCEPTION(ISOCPP_OUT_OF_RESOURCES_ERROR, "Could not create internal byte sequence.");
        }
        for(int32_t i = 0; i < size; i++)
        {
            byteArray[i] = from[static_cast<dds::core::ByteSeq::size_type>(i)];
        }
        to = static_cast<void*>(byteArray);
    }
}

void
org::eclipse::cyclonedds::core::convertByteSeq(
        const void* from,
        const int32_t    size,
        dds::core::ByteSeq  &to)
{
    const uint8_t *byteArray = reinterpret_cast<const uint8_t*>(from);
    to.clear();
    to.insert(to.end(), byteArray, byteArray + size);
}

void
org::eclipse::cyclonedds::core::convertStringSeq(
        const dds::core::StringSeq &from,
        char **&to)
{
    to = new char*[from.size()];
    for(uint32_t i = 0; i < from.size(); i++)
    {
        size_t len = from[i].length();
        DDSCXX_WARNING_MSVC_OFF(6386)
        to[i] = new char[len + 1];
        DDSCXX_WARNING_MSVC_ON(6386)
        to[i][len] = '\0';
        DDSCXX_WARNING_MSVC_OFF(6385)
        memcpy(to[i], from[i].c_str(), len);
        DDSCXX_WARNING_MSVC_ON(6385)
    }
}

void
org::eclipse::cyclonedds::core::convertStringSeq(
        char **from,
        uint32_t size,
        dds::core::StringSeq &to)
{
    if(from != NULL)
    {
        to.clear();
        to.reserve(size);
        for(uint32_t i = 0; i < size; i++)
        {
            to.push_back(static_cast<char *>(from[i]) ? static_cast<char *>(from[i]) : "");
        }
    }
}

dds::core::Duration
org::eclipse::cyclonedds::core::convertDuration(
        const dds_duration_t &from)
{
    if(from == DDS_INFINITY)
    {
        return dds::core::Duration::infinite();
    }
    else
    {
        return dds::core::Duration(
            static_cast<int64_t>(from / DDS_NSECS_IN_SEC), static_cast<uint32_t>(from - ((from / DDS_NSECS_IN_SEC) * DDS_NSECS_IN_SEC)));
    }
}

dds_duration_t
org::eclipse::cyclonedds::core::convertDuration(
        const dds::core::Duration &from)
{
    if(from == dds::core::Duration::infinite())
    {
        return DDS_INFINITY;
    }
    else
    {
        return (from.sec() * DDS_NSECS_IN_SEC) + from.nanosec();
    }
}

dds::core::Time
org::eclipse::cyclonedds::core::convertTime(
        const dds_time_t &from)
{
    return dds::core::Time(from / DDS_NSECS_IN_SEC,
                           static_cast<uint32_t>(from - ((from / DDS_NSECS_IN_SEC) * DDS_NSECS_IN_SEC)));
}

dds_time_t
org::eclipse::cyclonedds::core::convertTime(
        const dds::core::Time &from)
{
    if (from == dds::core::Time::invalid()) {
        return DDS_NEVER;
    }
    return (from.sec() * DDS_NSECS_IN_SEC) + from.nanosec();
}

dds::core::status::StatusMask
org::eclipse::cyclonedds::core::convertStatusMask(
        const uint32_t from)
{
    dds::core::status::StatusMask status_mask(dds::core::status::StatusMask::none());

    if (from & DDS_INCONSISTENT_TOPIC_STATUS) {
        status_mask << dds::core::status::StatusMask::inconsistent_topic();
    }
    if (from & DDS_OFFERED_DEADLINE_MISSED_STATUS) {
        status_mask << dds::core::status::StatusMask::offered_deadline_missed();
    }
    if (from & DDS_REQUESTED_DEADLINE_MISSED_STATUS) {
        status_mask << dds::core::status::StatusMask::requested_deadline_missed();
    }
    if (from & DDS_OFFERED_INCOMPATIBLE_QOS_STATUS) {
        status_mask << dds::core::status::StatusMask::offered_incompatible_qos();
    }
    if (from & DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS) {
        status_mask << dds::core::status::StatusMask::requested_incompatible_qos();
    }
    if (from & DDS_SAMPLE_LOST_STATUS) {
        status_mask << dds::core::status::StatusMask::sample_lost();
    }
    if (from & DDS_SAMPLE_REJECTED_STATUS) {
        status_mask << dds::core::status::StatusMask::sample_rejected();
    }
    if (from & DDS_DATA_ON_READERS_STATUS) {
        status_mask << dds::core::status::StatusMask::data_on_readers();
    }
    if (from & DDS_DATA_AVAILABLE_STATUS) {
        status_mask << dds::core::status::StatusMask::data_available();
    }
    if (from & DDS_LIVELINESS_LOST_STATUS) {
        status_mask << dds::core::status::StatusMask::liveliness_lost();
    }
    if (from & DDS_LIVELINESS_CHANGED_STATUS) {
        status_mask << dds::core::status::StatusMask::liveliness_changed();
    }
    if (from & DDS_PUBLICATION_MATCHED_STATUS) {
        status_mask << dds::core::status::StatusMask::publication_matched();
    }
    if (from & DDS_SUBSCRIPTION_MATCHED_STATUS) {
        status_mask << dds::core::status::StatusMask::subscription_matched();
    }

    return status_mask;
}


uint32_t
org::eclipse::cyclonedds::core::convertStatusMask(
        const dds::core::status::StatusMask& from)
{
    uint32_t ddsc_status_mask = 0;
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::inconsistent_topic())) {
        ddsc_status_mask |= DDS_INCONSISTENT_TOPIC_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::offered_deadline_missed())) {
        ddsc_status_mask |= DDS_OFFERED_DEADLINE_MISSED_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::requested_deadline_missed())) {
        ddsc_status_mask |= DDS_REQUESTED_DEADLINE_MISSED_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::offered_incompatible_qos())) {
        ddsc_status_mask |= DDS_OFFERED_INCOMPATIBLE_QOS_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::requested_incompatible_qos())) {
        ddsc_status_mask |= DDS_REQUESTED_INCOMPATIBLE_QOS_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::sample_lost())) {
        ddsc_status_mask |= DDS_SAMPLE_LOST_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::sample_rejected())) {
        ddsc_status_mask |= DDS_SAMPLE_REJECTED_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::data_on_readers())) {
        ddsc_status_mask |= DDS_DATA_ON_READERS_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::data_available())) {
        ddsc_status_mask |= DDS_DATA_AVAILABLE_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::liveliness_lost())) {
        ddsc_status_mask |= DDS_LIVELINESS_LOST_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::liveliness_changed())) {
        ddsc_status_mask |= DDS_LIVELINESS_CHANGED_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::publication_matched())) {
        ddsc_status_mask |= DDS_PUBLICATION_MATCHED_STATUS;
    }
    if (STATUS_MASK_CONTAINS(from, dds::core::status::StatusMask::subscription_matched())) {
        ddsc_status_mask |= DDS_SUBSCRIPTION_MATCHED_STATUS;
    }

    return ddsc_status_mask;
}
