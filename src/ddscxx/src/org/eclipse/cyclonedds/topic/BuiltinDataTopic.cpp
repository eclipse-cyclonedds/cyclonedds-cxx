#include <cstdint>
#include <vector>
#include <string>

#include "org/eclipse/cyclonedds/topic/TopicTraits.hpp"
#include "org/eclipse/cyclonedds/topic/BuiltinDataTopic.hpp"

bool builtin_serdata_eqkey(const ddsi_serdata* a, const ddsi_serdata* b)
{
  (void)a;
  (void)b;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");

  return false;
}

uint32_t builtin_serdata_size(const ddsi_serdata* dcmn)
{
  (void)dcmn;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");

  return 0;
}

ddsi_serdata *builtin_serdata_from_ser(
  const ddsi_sertype* type,
  enum ddsi_serdata_kind kind,
  const struct nn_rdata* fragchain,
  size_t size)
{
  (void)type;
  (void)kind;
  (void)fragchain;
  (void)size;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");

  return nullptr;
}

ddsi_serdata *builtin_serdata_from_ser_iov(
  const ddsi_sertype* type,
  enum ddsi_serdata_kind kind,
  ddsrt_msg_iovlen_t niov,
  const ddsrt_iovec_t* iov,
  size_t size)
{
  (void)type;
  (void)kind;
  (void)niov;
  (void)iov;
  (void)size;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");

  return nullptr;

}

ddsi_serdata *builtin_serdata_from_keyhash(
  const ddsi_sertype* type,
  const struct ddsi_keyhash* keyhash)
{
  (void)keyhash;
  (void)type;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");

  return nullptr;
}

ddsi_serdata *builtin_serdata_from_sample(
  const ddsi_sertype* typecmn,
  enum ddsi_serdata_kind kind,
  const void* sample)
{
  (void)typecmn;
  (void)kind;
  (void)sample;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");

  return nullptr;
}

void builtin_serdata_to_ser(const ddsi_serdata* dcmn, size_t off, size_t sz, void* buf)
{
  (void)dcmn;
  (void)off;
  (void)sz;
  (void)buf;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");
}

ddsi_serdata *builtin_serdata_to_ser_ref(
  const ddsi_serdata* dcmn, size_t off,
  size_t sz, ddsrt_iovec_t* ref)
{
  (void)dcmn;
  (void)off;
  (void)sz;
  (void)ref;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");

  return nullptr;
}

void builtin_serdata_to_ser_unref(ddsi_serdata* dcmn, const ddsrt_iovec_t* ref)
{
  (void)dcmn;
  (void)ref;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");
}

bool builtin_serdata_to_sample(
  const ddsi_serdata* dcmn, void* sample, void** bufptr,
  void* buflim)
{
  (void)bufptr;
  (void)buflim;
  (void)dcmn;
  (void)sample;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");

  return false;
}

ddsi_serdata *builtin_serdata_to_untyped(const ddsi_serdata* dcmn)
{
  (void)dcmn;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");

  return nullptr;
}

bool builtin_serdata_untyped_to_sample(
  const ddsi_sertype* type,
  const ddsi_serdata* dcmn, void* sample,
  void** bufptr, void* buflim)
{
  (void)type;
  (void)dcmn;
  (void)sample;
  (void)bufptr;
  (void)buflim;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");

  return false;
}

void builtin_serdata_get_keyhash(
  const ddsi_serdata* d, struct ddsi_keyhash* buf,
  bool force_md5)
{
  (void)d;
  (void)buf;
  (void)force_md5;

  ISOCPP_DDSC_RESULT_CHECK_AND_THROW(DDS_RETCODE_NOT_ENABLED, "Unsupported method for builtin topics.");
}

template <>
dds::topic::ParticipantBuiltinTopicData* ddscxx_serdata<dds::topic::ParticipantBuiltinTopicData>::getT()
{
  dds::topic::ParticipantBuiltinTopicData *t
    = m_t.load(std::memory_order_acquire);
  if (t == nullptr) {
    t = new dds::topic::ParticipantBuiltinTopicData();
      dds::topic::ParticipantBuiltinTopicData* exp = nullptr;
      if (!m_t.compare_exchange_strong(exp, t, std::memory_order_seq_cst)) {
        delete t;
        t = exp;
      }
  }
  return t;
}

template <>
dds::topic::TopicBuiltinTopicData* ddscxx_serdata<dds::topic::TopicBuiltinTopicData>::getT()
{
  dds::topic::TopicBuiltinTopicData *t
    = m_t.load(std::memory_order_acquire);
  if (t == nullptr) {
    t = new dds::topic::TopicBuiltinTopicData();
      dds::topic::TopicBuiltinTopicData* exp = nullptr;
      if (!m_t.compare_exchange_strong(exp, t, std::memory_order_seq_cst)) {
        delete t;
        t = exp;
      }
  }
  return t;
}

template <>
dds::topic::PublicationBuiltinTopicData* ddscxx_serdata<dds::topic::PublicationBuiltinTopicData>::getT()
{
  dds::topic::PublicationBuiltinTopicData *t
    = m_t.load(std::memory_order_acquire);
  if (t == nullptr) {
    t = new dds::topic::PublicationBuiltinTopicData();
      dds::topic::PublicationBuiltinTopicData* exp = nullptr;
      if (!m_t.compare_exchange_strong(exp, t, std::memory_order_seq_cst)) {
        delete t;
        t = exp;
      }
  }
  return t;
}

template <>
dds::topic::SubscriptionBuiltinTopicData* ddscxx_serdata<dds::topic::SubscriptionBuiltinTopicData>::getT()
{
  dds::topic::SubscriptionBuiltinTopicData *t
    = m_t.load(std::memory_order_acquire);
  if (t == nullptr) {
    t = new dds::topic::SubscriptionBuiltinTopicData();
      dds::topic::SubscriptionBuiltinTopicData* exp = nullptr;
      if (!m_t.compare_exchange_strong(exp, t, std::memory_order_seq_cst)) {
        delete t;
        t = exp;
      }
  }
  return t;
}

#ifndef _WIN32

template<typename T>
struct builtin_serdata_ops: public ddsi_serdata_ops {
  builtin_serdata_ops(): ddsi_serdata_ops({
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
  &serdata_free<T>,
  &serdata_print<T>,
  &builtin_serdata_get_keyhash,
  #ifdef DDS_HAS_SHM
  NULL, //get_sample_size
  NULL  //from_iox_buffer
  #endif
}) {;}
};

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::ParticipantBuiltinTopicData>::ddscxx_serdata_ops =
  builtin_serdata_ops<dds::topic::ParticipantBuiltinTopicData>();

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::TopicBuiltinTopicData>::ddscxx_serdata_ops =
  builtin_serdata_ops<dds::topic::TopicBuiltinTopicData>();

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::SubscriptionBuiltinTopicData>::ddscxx_serdata_ops =
  builtin_serdata_ops<dds::topic::SubscriptionBuiltinTopicData>();

template <>
const ddsi_serdata_ops ddscxx_serdata<dds::topic::PublicationBuiltinTopicData>::ddscxx_serdata_ops =
  builtin_serdata_ops<dds::topic::PublicationBuiltinTopicData>();
#endif /* !_WIN32 */
