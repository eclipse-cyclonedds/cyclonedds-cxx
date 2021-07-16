/*
 * Copyright(c) 2006 to 2021 ADLINK Technology Limited and others
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

#ifndef CYCLONEDDS_TOP_BUILTINDATATOPIC_HPP_
#define CYCLONEDDS_TOP_BUILTINDATATOPIC_HPP_

#include <org/eclipse/cyclonedds/topic/datatopic.hpp>
#include <dds/topic/BuiltinTopic.hpp>
#include <org/eclipse/cyclonedds/topic/BuiltinTopicTraits.hpp>
#include <dds/core/macros.hpp>

/**
 * Generic builtin topic serdata function declarations.
 */

OMG_DDS_API_DETAIL
bool builtin_serdata_eqkey(const ddsi_serdata* a, const ddsi_serdata* b);

OMG_DDS_API_DETAIL
uint32_t builtin_serdata_size(const ddsi_serdata* dcmn);

OMG_DDS_API_DETAIL
ddsi_serdata *builtin_serdata_from_ser(
  const ddsi_sertype* type,
  enum ddsi_serdata_kind kind,
  const struct nn_rdata* fragchain,
  size_t size);

OMG_DDS_API_DETAIL
ddsi_serdata *builtin_serdata_from_ser_iov(
  const ddsi_sertype* type,
  enum ddsi_serdata_kind kind,
  ddsrt_msg_iovlen_t niov,
  const ddsrt_iovec_t* iov,
  size_t size);

OMG_DDS_API_DETAIL
ddsi_serdata *builtin_serdata_from_keyhash(
  const ddsi_sertype* type,
  const struct ddsi_keyhash* keyhash);

OMG_DDS_API_DETAIL
ddsi_serdata *builtin_serdata_from_sample(
  const ddsi_sertype* typecmn,
  enum ddsi_serdata_kind kind,
  const void* sample);

OMG_DDS_API_DETAIL
void builtin_serdata_to_ser(const ddsi_serdata* dcmn, size_t off, size_t sz, void* buf);

OMG_DDS_API_DETAIL ddsi_serdata *builtin_serdata_to_ser_ref(
  const ddsi_serdata* dcmn, size_t off,
  size_t sz, ddsrt_iovec_t* ref);

OMG_DDS_API_DETAIL
void builtin_serdata_to_ser_unref(ddsi_serdata* dcmn, const ddsrt_iovec_t* ref);

OMG_DDS_API_DETAIL
bool builtin_serdata_to_sample(
  const ddsi_serdata* dcmn, void* sample, void** bufptr,
  void* buflim);

OMG_DDS_API_DETAIL
ddsi_serdata *builtin_serdata_to_untyped(const ddsi_serdata* dcmn);

OMG_DDS_API_DETAIL
bool builtin_serdata_untyped_to_sample(
  const ddsi_sertype* type,
  const ddsi_serdata* dcmn, void* sample,
  void** bufptr, void* buflim);

OMG_DDS_API_DETAIL
void builtin_serdata_get_keyhash(
  const ddsi_serdata* d, struct ddsi_keyhash* buf,
  bool force_md5);

/**
 * Builtin topic serdata function template specialization declarations.
 */

template <>
OMG_DDS_API_DETAIL
dds::topic::ParticipantBuiltinTopicData* ddscxx_serdata<dds::topic::ParticipantBuiltinTopicData>::getT();

template <>
OMG_DDS_API_DETAIL
dds::topic::TopicBuiltinTopicData* ddscxx_serdata<dds::topic::TopicBuiltinTopicData>::getT();

template <>
OMG_DDS_API_DETAIL
dds::topic::PublicationBuiltinTopicData* ddscxx_serdata<dds::topic::PublicationBuiltinTopicData>::getT();

template <>
OMG_DDS_API_DETAIL
dds::topic::SubscriptionBuiltinTopicData* ddscxx_serdata<dds::topic::SubscriptionBuiltinTopicData>::getT();

/**
 * Due to a bug in MSVC template class static const member need to be initialized in-place.
 */

#ifdef _WIN32

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::ParticipantBuiltinTopicData>::ddscxx_serdata_ops = {
  &builtin_serdata_eqkey,
  &builtin_serdata_size,
  &builtin_serdata_from_ser,
  &builtin_serdata_from_ser_iov,
  &builtin_serdata_from_keyhash,
  &builtin_serdata_from_sample,
  &builtin_serdata_to_ser,
  &builtin_serdata_to_ser_ref,
  &builtin_serdata_to_ser_unref,
  &builtin_serdata_to_sample,
  &builtin_serdata_to_untyped,
  &builtin_serdata_untyped_to_sample,
  &serdata_free<dds::topic::ParticipantBuiltinTopicData>,
  &serdata_print<dds::topic::ParticipantBuiltinTopicData>,
  &builtin_serdata_get_keyhash
};

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::TopicBuiltinTopicData>::ddscxx_serdata_ops = {
  &builtin_serdata_eqkey,
  &builtin_serdata_size,
  &builtin_serdata_from_ser,
  &builtin_serdata_from_ser_iov,
  &builtin_serdata_from_keyhash,
  &builtin_serdata_from_sample,
  &builtin_serdata_to_ser,
  &builtin_serdata_to_ser_ref,
  &builtin_serdata_to_ser_unref,
  &builtin_serdata_to_sample,
  &builtin_serdata_to_untyped,
  &builtin_serdata_untyped_to_sample,
  &serdata_free<dds::topic::TopicBuiltinTopicData>,
  &serdata_print<dds::topic::TopicBuiltinTopicData>,
  &builtin_serdata_get_keyhash
};

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::SubscriptionBuiltinTopicData>::ddscxx_serdata_ops = {
  &builtin_serdata_eqkey,
  &builtin_serdata_size,
  &builtin_serdata_from_ser,
  &builtin_serdata_from_ser_iov,
  &builtin_serdata_from_keyhash,
  &builtin_serdata_from_sample,
  &builtin_serdata_to_ser,
  &builtin_serdata_to_ser_ref,
  &builtin_serdata_to_ser_unref,
  &builtin_serdata_to_sample,
  &builtin_serdata_to_untyped,
  &builtin_serdata_untyped_to_sample,
  &serdata_free<dds::topic::SubscriptionBuiltinTopicData>,
  &serdata_print<dds::topic::SubscriptionBuiltinTopicData>,
  &builtin_serdata_get_keyhash
};

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::PublicationBuiltinTopicData>::ddscxx_serdata_ops = {
  &builtin_serdata_eqkey,
  &builtin_serdata_size,
  &builtin_serdata_from_ser,
  &builtin_serdata_from_ser_iov,
  &builtin_serdata_from_keyhash,
  &builtin_serdata_from_sample,
  &builtin_serdata_to_ser,
  &builtin_serdata_to_ser_ref,
  &builtin_serdata_to_ser_unref,
  &builtin_serdata_to_sample,
  &builtin_serdata_to_untyped,
  &builtin_serdata_untyped_to_sample,
  &serdata_free<dds::topic::PublicationBuiltinTopicData>,
  &serdata_print<dds::topic::PublicationBuiltinTopicData>,
  &builtin_serdata_get_keyhash
};

#else

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::ParticipantBuiltinTopicData>::ddscxx_serdata_ops;

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::TopicBuiltinTopicData>::ddscxx_serdata_ops;

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::SubscriptionBuiltinTopicData>::ddscxx_serdata_ops;

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::PublicationBuiltinTopicData>::ddscxx_serdata_ops;

#endif  /* _WIN32 */

#endif  /* CYCLONEDDS_TOP_BUILTINDATATOPIC_HPP_ */
