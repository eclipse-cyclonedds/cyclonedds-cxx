#include <cstdint>
#include <vector>
#include <string>

#include "org/eclipse/cyclonedds/topic/TopicTraits.hpp"
#include "org/eclipse/cyclonedds/topic/BuiltinDataTopic.hpp"

#define unsupported_method_throw ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.")

bool builtin_serdata_eqkey(
  const ddsi_serdata*,
  const ddsi_serdata*)
{
  unsupported_method_throw;
  return false;
}

uint32_t builtin_serdata_size(
  const ddsi_serdata*)
{
  unsupported_method_throw;
  return 0;
}

ddsi_serdata *builtin_serdata_from_ser(
  const ddsi_sertype*,
  enum ddsi_serdata_kind,
  const struct nn_rdata*,
  size_t)
{
  unsupported_method_throw;
  return nullptr;
}

ddsi_serdata *builtin_serdata_from_ser_iov(
  const ddsi_sertype*,
  enum ddsi_serdata_kind,
  ddsrt_msg_iovlen_t,
  const ddsrt_iovec_t*,
  size_t)
{
  unsupported_method_throw;
  return nullptr;

}

ddsi_serdata *builtin_serdata_from_keyhash(
  const ddsi_sertype*,
  const struct ddsi_keyhash*)
{
  unsupported_method_throw;
  return nullptr;
}

ddsi_serdata *builtin_serdata_from_sample(
  const ddsi_sertype*,
  enum ddsi_serdata_kind,
  const void*)
{
  unsupported_method_throw;
  return nullptr;
}

void builtin_serdata_to_ser(
  const ddsi_serdata*,
  size_t,
  size_t,
  void*)
{
  unsupported_method_throw;
}

ddsi_serdata *builtin_serdata_to_ser_ref(
  const ddsi_serdata*,
  size_t,
  size_t,
  ddsrt_iovec_t*)
{
  unsupported_method_throw;
  return nullptr;
}

void builtin_serdata_to_ser_unref(
  ddsi_serdata*,
  const ddsrt_iovec_t*)
{
  unsupported_method_throw;
}

bool builtin_serdata_to_sample(
  const ddsi_serdata*,
  void*,
  void**,
  void*)
{
  unsupported_method_throw;
  return false;
}

ddsi_serdata *builtin_serdata_to_untyped(
  const ddsi_serdata*)
{
  unsupported_method_throw;
  return nullptr;
}

bool builtin_serdata_untyped_to_sample(
  const ddsi_sertype*,
  const ddsi_serdata*,
  void*,
  void**,
  void*)
{
  unsupported_method_throw;
  return false;
}

void builtin_serdata_get_keyhash(
  const ddsi_serdata*,
  struct ddsi_keyhash*,
  bool)
{
  unsupported_method_throw;
}

#ifdef DDSCXX_HAS_SHM
uint32_t builtin_serdata_iox_size(
  const struct ddsi_serdata*)
{
  unsupported_method_throw;
  return 0;
}

ddsi_serdata * builtin_serdata_from_iox_buffer(
  const struct ddsi_sertype *,
  enum ddsi_serdata_kind,
  void *,
  void *)
{
  unsupported_method_throw;
  return nullptr;
}
#endif

#ifdef _WIN32
#define builtin_topic_ops_impl(builtin_topic_type)
#else
#define builtin_topic_ops_impl(builtin_topic_type)\
builtin_topic_ops(builtin_topic_type)
#endif

#define builtin_topic_impl(builtin_topic_type)\
template <> \
dds::topic::builtin_topic_type* ddscxx_serdata<dds::topic::builtin_topic_type>::getT() {\
  dds::topic::builtin_topic_type *t = m_t.load(std::memory_order_acquire);\
  if (t == nullptr) {\
    t = new dds::topic::builtin_topic_type();\
    dds::topic::builtin_topic_type* exp = nullptr;\
    if (!m_t.compare_exchange_strong(exp, t, std::memory_order_seq_cst)) {\
      delete t;\
      t = exp;\
    }\
  }\
  return t;\
}\
template <> \
bool sertype_serialize_into<dds::topic::builtin_topic_type>(const ddsi_sertype*, const void*, void*, size_t) {\
  unsupported_method_throw;\
  return false;\
}\
template <> \
size_t sertype_get_serialized_size<dds::topic::builtin_topic_type>(const ddsi_sertype*, const void*) {\
  unsupported_method_throw;\
  return 0;\
}\
builtin_topic_ops_impl(builtin_topic_type)

builtin_topic_impl(ParticipantBuiltinTopicData)
builtin_topic_impl(TopicBuiltinTopicData)
builtin_topic_impl(PublicationBuiltinTopicData)
builtin_topic_impl(SubscriptionBuiltinTopicData)
