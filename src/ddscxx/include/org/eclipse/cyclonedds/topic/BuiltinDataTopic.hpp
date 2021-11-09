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

#ifndef CYCLONEDDS_TOPIC_BUILTINDATATOPIC_HPP_
#define CYCLONEDDS_TOPIC_BUILTINDATATOPIC_HPP_

#include <org/eclipse/cyclonedds/topic/datatopic.hpp>
#include <dds/topic/BuiltinTopic.hpp>
#include <org/eclipse/cyclonedds/topic/BuiltinTopicTraits.hpp>
#include <dds/core/macros.hpp>

/**
 * Generic builtin topic serdata function declarations.
 */

OMG_DDS_API_DETAIL
bool builtin_serdata_eqkey(
  const ddsi_serdata*,
  const ddsi_serdata*);

OMG_DDS_API_DETAIL
uint32_t builtin_serdata_size(
  const ddsi_serdata*);

OMG_DDS_API_DETAIL
ddsi_serdata *builtin_serdata_from_ser(
  const ddsi_sertype*,
  enum ddsi_serdata_kind,
  const struct nn_rdata*,
  size_t size);

OMG_DDS_API_DETAIL
ddsi_serdata *builtin_serdata_from_ser_iov(
  const ddsi_sertype*,
  enum ddsi_serdata_kind,
  ddsrt_msg_iovlen_t,
  const ddsrt_iovec_t*,
  size_t);

OMG_DDS_API_DETAIL
ddsi_serdata *builtin_serdata_from_keyhash(
  const ddsi_sertype*,
  const struct ddsi_keyhash*);

OMG_DDS_API_DETAIL
ddsi_serdata *builtin_serdata_from_sample(
  const ddsi_sertype*,
  enum ddsi_serdata_kind,
  const void*);

OMG_DDS_API_DETAIL
void builtin_serdata_to_ser(
  const ddsi_serdata*,
  size_t,
  size_t,
  void*);

OMG_DDS_API_DETAIL ddsi_serdata *builtin_serdata_to_ser_ref(
  const ddsi_serdata*,
  size_t,
  size_t,
  ddsrt_iovec_t*);

OMG_DDS_API_DETAIL
void builtin_serdata_to_ser_unref(
  ddsi_serdata*,
  const ddsrt_iovec_t*);

OMG_DDS_API_DETAIL
bool builtin_serdata_to_sample(
  const ddsi_serdata*,
  void*,
  void**,
  void*);

OMG_DDS_API_DETAIL
ddsi_serdata *builtin_serdata_to_untyped(
  const ddsi_serdata*);

OMG_DDS_API_DETAIL
bool builtin_serdata_untyped_to_sample(
  const ddsi_sertype*,
  const ddsi_serdata*,
  void*,
  void**,
  void*);

OMG_DDS_API_DETAIL
void builtin_serdata_get_keyhash(
  const ddsi_serdata*,
  struct ddsi_keyhash*,
  bool);

#ifdef DDSCXX_HAS_SHM
OMG_DDS_API_DETAIL
uint32_t builtin_serdata_iox_size(
  const struct ddsi_serdata*);

OMG_DDS_API_DETAIL
ddsi_serdata * builtin_serdata_from_iox_buffer(
  const struct ddsi_sertype *,
  enum ddsi_serdata_kind,
  void *,
  void *);
#endif

/**
 * Builtin topic serdata function template specialization declarations.
 */

#ifdef DDSCXX_HAS_SHM
#define builtin_topic_ops(builtin_topic_type)\
template <> const ddsi_serdata_ops ddscxx_serdata<dds::topic::builtin_topic_type>::ddscxx_serdata_ops = {\
  &builtin_serdata_eqkey,\
  &builtin_serdata_size,\
  &builtin_serdata_from_ser,\
  &builtin_serdata_from_ser_iov,\
  &builtin_serdata_from_keyhash,\
  &builtin_serdata_from_sample,\
  &builtin_serdata_to_ser,\
  &builtin_serdata_to_ser_ref,\
  &builtin_serdata_to_ser_unref,\
  &builtin_serdata_to_sample,\
  &builtin_serdata_to_untyped,\
  &builtin_serdata_untyped_to_sample,\
  &serdata_free<dds::topic::builtin_topic_type>,\
  &serdata_print<dds::topic::builtin_topic_type>,\
  &builtin_serdata_get_keyhash,\
  &builtin_serdata_iox_size,\
  &builtin_serdata_from_iox_buffer\
};
#else
#define builtin_topic_ops(builtin_topic_type)\
template <> const ddsi_serdata_ops ddscxx_serdata<dds::topic::builtin_topic_type>::ddscxx_serdata_ops = {\
  &builtin_serdata_eqkey,\
  &builtin_serdata_size,\
  &builtin_serdata_from_ser,\
  &builtin_serdata_from_ser_iov,\
  &builtin_serdata_from_keyhash,\
  &builtin_serdata_from_sample,\
  &builtin_serdata_to_ser,\
  &builtin_serdata_to_ser_ref,\
  &builtin_serdata_to_ser_unref,\
  &builtin_serdata_to_sample,\
  &builtin_serdata_to_untyped,\
  &builtin_serdata_untyped_to_sample,\
  &serdata_free<dds::topic::builtin_topic_type>,\
  &serdata_print<dds::topic::builtin_topic_type>,\
  &builtin_serdata_get_keyhash\
};
#endif

/**
 * Due to a bug in MSVC template class static const member need to be initialized in-place.
 */

#ifdef _WIN32
#define builtin_topic_ops_decl(builtin_topic_type)\
builtin_topic_ops(builtin_topic_type)
#else
#define builtin_topic_ops_decl(builtin_topic_type)\
template <> const ddsi_serdata_ops ddscxx_serdata<dds::topic::builtin_topic_type>::ddscxx_serdata_ops;
#endif

#define builtin_topic_decl(builtin_topic_type)\
template <> OMG_DDS_API_DETAIL \
dds::topic::builtin_topic_type* ddscxx_serdata<dds::topic::builtin_topic_type>::getT();\
template <> OMG_DDS_API_DETAIL \
bool sertype_serialize_into<dds::topic::builtin_topic_type>(const ddsi_sertype*, const void*, void*, size_t);\
template <> OMG_DDS_API_DETAIL \
size_t sertype_get_serialized_size<dds::topic::builtin_topic_type>(const ddsi_sertype*, const void*);\
builtin_topic_ops_decl(builtin_topic_type)

builtin_topic_decl(ParticipantBuiltinTopicData)
builtin_topic_decl(TopicBuiltinTopicData)
builtin_topic_decl(PublicationBuiltinTopicData)
builtin_topic_decl(SubscriptionBuiltinTopicData)

#endif  /* CYCLONEDDS_TOPIC_BUILTINDATATOPIC_HPP_ */
