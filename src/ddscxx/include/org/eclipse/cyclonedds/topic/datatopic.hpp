// Copyright(c) 2020 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#ifndef DDSCXXDATATOPIC_HPP_
#define DDSCXXDATATOPIC_HPP_

#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <atomic>

#include "dds/ddsrt/md5.h"
#include "dds/ddsc/dds_loaned_sample.h"
#include "dds/ddsc/dds_psmx.h"
#include "org/eclipse/cyclonedds/core/cdr/basic_cdr_ser.hpp"
#include "org/eclipse/cyclonedds/core/cdr/extended_cdr_v1_ser.hpp"
#include "org/eclipse/cyclonedds/core/cdr/extended_cdr_v2_ser.hpp"
#include "org/eclipse/cyclonedds/core/cdr/fragchain.hpp"
#include "org/eclipse/cyclonedds/topic/TopicTraits.hpp"
#include "org/eclipse/cyclonedds/topic/hash.hpp"

constexpr size_t CDR_HEADER_SIZE = 4U;
#define BO_LITTLE   0X01
#define PLAIN_CDR   0x00
#define PL_CDR      0x02
#define PLAIN_CDR2  0x06
#define D_CDR       0x08
#define PL_CDR2     0x0A

// macro to check if a pointer is nullptr and return false
#define CHECK_FOR_NULL(val)  if ((val) == nullptr) return false;

using org::eclipse::cyclonedds::core::cdr::endianness;
using org::eclipse::cyclonedds::core::cdr::native_endianness;
using org::eclipse::cyclonedds::core::cdr::cdr_stream;
using org::eclipse::cyclonedds::core::cdr::basic_cdr_stream;
using org::eclipse::cyclonedds::core::cdr::xcdr_v1_stream;
using org::eclipse::cyclonedds::core::cdr::xcdr_v2_stream;
using org::eclipse::cyclonedds::core::cdr::extensibility;
using org::eclipse::cyclonedds::core::cdr::encoding_version;
using org::eclipse::cyclonedds::core::cdr::key_mode;
using org::eclipse::cyclonedds::topic::TopicTraits;

template<typename T, class S, key_mode K>
bool get_serialized_size(const T& sample, size_t &sz);

template<typename T>
bool to_key(const T& tokey, ddsi_keyhash_t& hash)
{
  if (TopicTraits<T>::isKeyless())
  {
    memset(&(hash.value), 0x0, sizeof(hash.value));  //just set all key bytes to 0 as all instances have the same hash value, and hashing is pointless
    return true;
  } else
  {
    basic_cdr_stream str(endianness::big_endian);
    size_t sz = 0;
    if (!get_serialized_size<T, basic_cdr_stream, key_mode::sorted>(tokey, sz)) {
      assert(false);
      return false;
    }
    size_t padding = 0;
    if (sz < 16)
      padding = (16 - sz % 16)%16;
    std::vector<unsigned char> buffer(sz + padding);
    if (padding)
      memset(buffer.data() + sz, 0x0, padding);
    str.set_buffer(buffer.data(), sz);
    if (!write(str, tokey, key_mode::sorted)) {
      assert(false);
      return false;
    }
    static thread_local bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t&) = NULL;
    if (fptr == NULL)
    {
      if (!max(str, tokey, key_mode::sorted)) {
        assert(false);
        return false;
      }
      if (str.position() <= 16)
      {
        //bind to unmodified function which just copies buffer into the keyhash
        fptr = &org::eclipse::cyclonedds::topic::simple_key;
      }
      else
      {
        //bind to MD5 hash function
        fptr = &org::eclipse::cyclonedds::topic::complex_key;
      }
    }
    return (*fptr)(buffer, hash);
  }
}

static inline void* calc_offset(void* ptr, ptrdiff_t n)
{
  return static_cast<void*>(static_cast<unsigned char*>(ptr) + n);
}

static inline const void* calc_offset(const void* ptr, ptrdiff_t n)
{
  return static_cast<const void*>(static_cast<const unsigned char*>(ptr) + n);
}

template<typename T,
         class S,
         std::enable_if_t<std::is_same<basic_cdr_stream, S>::value, bool> = true >
bool write_header(void *buffer)
{
  CHECK_FOR_NULL(buffer);
  memset(buffer, 0x0, 4);

  auto ptr = static_cast<unsigned char*>(calc_offset(buffer, 1));

  assert(TopicTraits<T>::getExtensibility() == extensibility::ext_final);
  *ptr = PLAIN_CDR;

  if (native_endianness() == endianness::little_endian)
    *ptr |= BO_LITTLE;

  return true;
}

template<typename T,
         class S,
         std::enable_if_t<std::is_same<xcdr_v2_stream, S>::value, bool> = true >
bool write_header(void *buffer)
{
  CHECK_FOR_NULL(buffer);
  memset(buffer, 0x0, 4);

  auto ptr = static_cast<unsigned char*>(calc_offset(buffer, 1));

  switch (TopicTraits<T>::getExtensibility()) {
    case extensibility::ext_final:
      *ptr = PLAIN_CDR2;
      break;
    case extensibility::ext_appendable:
      *ptr = D_CDR;
      break;
    case extensibility::ext_mutable:
      *ptr = PL_CDR2;
      break;
  }

  if (native_endianness() == endianness::little_endian)
    *ptr |= BO_LITTLE;

  return true;
}

template<typename T,
         class S,
         std::enable_if_t<std::is_same<xcdr_v1_stream, S>::value, bool> = true >
bool write_header(void *buffer)
{
  CHECK_FOR_NULL(buffer);
  memset(buffer, 0x0, 4);

  auto ptr = static_cast<unsigned char*>(calc_offset(buffer, 1));

  switch (TopicTraits<T>::getExtensibility()) {
    case extensibility::ext_final:
    case extensibility::ext_appendable:
      *ptr = PLAIN_CDR;
      break;
    case extensibility::ext_mutable:
      *ptr = PL_CDR;
      break;
  }

  if (native_endianness() == endianness::little_endian)
    *ptr |= BO_LITTLE;

  return true;
}

template<typename T>
bool finish_header(void *buffer, size_t bytes_written)
{
  CHECK_FOR_NULL(buffer);
  auto alignbytes = static_cast<unsigned char>((4 - (bytes_written % 4)) % 4);
  auto ptr = static_cast<unsigned char*>(calc_offset(buffer, 3));

  *ptr = alignbytes;

  return true;
}

template<typename T>
bool read_header(const void *buffer, encoding_version &ver, endianness &end)
{
  CHECK_FOR_NULL(buffer);
  auto ptr = static_cast<const unsigned char*>(calc_offset(buffer, 1));

  if (*ptr & BO_LITTLE)
    end = endianness::little_endian;
  else
    end = endianness::big_endian;

  auto field = *ptr & ~BO_LITTLE;
  switch (TopicTraits<T>::getExtensibility()) {
    case extensibility::ext_final:
      switch (field) {
        case PLAIN_CDR:
        /** this can either mean:
         *  - legacy cdr encoding
         *  - PLAIN_CDR xcdr_v1 encoding (deprecated)
         *  And this might cause issues, since these support different features, e.g.: if you have a final struct data type,
         *  but it contains an appendable enumerator, or a member which is an appendable struct, this is encoded as PLAIN_CDR,
         *  but if you try to deserialize this with a legacy (or basic_cdr_stream) deserializer, this may cause all sorts of
         *  "interesting" errors. This is why it looks at the allowableEncodings() topic trait: this encodes which types of
         *  encoding are supported for a datatype. If some properties requiring XTypes functionality are encountered (like
         *  appendable/mutable types, optional members, etc.) the IDL_DATAREPRESENTATION_FLAG_XCDR1 flag is unset in the trait,
         *  as the CycloneDDS serialization only wants to write xcdr_v2.
         */
          if (TopicTraits<T>::allowableEncodings() & DDS_DATA_REPRESENTATION_FLAG_XCDR1)
            ver = encoding_version::basic_cdr;
          else
            ver = encoding_version::xcdr_v1;
          break;
        case PLAIN_CDR2:
          ver = encoding_version::xcdr_v2;
          break;
        default:
          return false;
      }
      break;
    case extensibility::ext_appendable:
      switch (field) {
        case PLAIN_CDR:
          ver = encoding_version::xcdr_v1;
          break;
        case D_CDR:
          ver = encoding_version::xcdr_v2;
          break;
        default:
          return false;
      }
      break;
    case extensibility::ext_mutable:
      switch (field) {
        case PL_CDR:
          ver = encoding_version::xcdr_v1;
          break;
        case PL_CDR2:
          ver = encoding_version::xcdr_v2;
          break;
        default:
          return false;
      }
      break;
    default:
      return false;
  }

  return true;
}

template<typename T, class S, key_mode K>
bool get_serialized_fixed_size(const T& sample, size_t &sz)
{
  static thread_local size_t serialized_size = 0;
  static thread_local std::mutex mtx;
  static thread_local std::atomic_bool initialized {false};
  if (initialized.load(std::memory_order_relaxed)) {
    sz = serialized_size;
    return true;
  }
  std::lock_guard<std::mutex> lock(mtx);
  if (initialized.load(std::memory_order_relaxed)) {
    sz = serialized_size;
    return true;
  }
  S str;
  if (!move(str, sample, K))
    return false;
  serialized_size = str.position();
  initialized.store(true, std::memory_order_release);
  sz = serialized_size;
  return true;
}

template<typename T, class S, key_mode K>
bool get_serialized_size(const T& sample, size_t &sz)
{
  if (TopicTraits<T>::isSelfContained()) {
    if (!get_serialized_fixed_size<T,S,K>(sample,sz))
      return false;
  } else {
    S str;
    if (!move(str, sample, K))
      return false;
    sz = str.position();
  }

  return true;
}
template<typename T, class S>
bool serialize_into_impl(void *hdr,
                         void *buffer,
                         size_t buf_sz,
                         const T &sample,
                         key_mode mode)
{
  CHECK_FOR_NULL(buffer);
  CHECK_FOR_NULL(hdr);

  S str;
  str.set_buffer(buffer, buf_sz);
  return (write_header<T,S>(hdr)
        && write(str, sample, mode)
        && finish_header<T>(hdr, buf_sz));
}

template<typename T, class S>
bool serialize_into(void *buffer,
                    size_t buf_sz,
                    const T &sample,
                    key_mode mode)
{
  assert(buf_sz >= CDR_HEADER_SIZE);
  void *cdr_start = calc_offset(buffer, CDR_HEADER_SIZE);
  return serialize_into_impl<T,S>(buffer,cdr_start,buf_sz, sample, mode);
}

template <typename T, typename S>
bool deserialize_sample_from_buffer_impl(void *buffer,
                                    size_t buf_sz,
                                    T &sample,
                                    const ddsi_serdata_kind data_kind,
                                    endianness end)
{
  S str(end);
  str.set_buffer(buffer, buf_sz);
  return read(str, sample, data_kind == SDK_KEY ? key_mode::unsorted : key_mode::not_key);
}

/// \brief De-serialize the buffer into the sample
/// \param[in] buffer The buffer to be de-serialized
/// \param[out] sample Type to which the buffer will be de-serialized
/// \param[in] data_kind The data kind (data, or key)
/// \tparam T The sample type
/// \return True if the deserialization is successful
///         False if the deserialization failed
template <typename T>
bool deserialize_sample_from_buffer(void *buffer,
                                    size_t buf_sz,
                                    T &sample,
                                    const ddsi_serdata_kind data_kind=SDK_DATA)
{
  CHECK_FOR_NULL(buffer);
  assert(data_kind != SDK_EMPTY);

  encoding_version ver;
  endianness end;
  if (!read_header<T>(buffer, ver, end))
    return false;

  switch (ver) {
    case encoding_version::basic_cdr:
      return deserialize_sample_from_buffer_impl<T, basic_cdr_stream>(calc_offset(buffer, CDR_HEADER_SIZE), buf_sz-CDR_HEADER_SIZE, sample, data_kind, end);
      break;
    case encoding_version::xcdr_v1:
      return deserialize_sample_from_buffer_impl<T, xcdr_v1_stream>(calc_offset(buffer, CDR_HEADER_SIZE), buf_sz-CDR_HEADER_SIZE, sample, data_kind, end);
      break;
    case encoding_version::xcdr_v2:
      return deserialize_sample_from_buffer_impl<T, xcdr_v2_stream>(calc_offset(buffer, CDR_HEADER_SIZE), buf_sz-CDR_HEADER_SIZE, sample, data_kind, end);
      break;
    default:
      return false;
  }
}

template <typename T> class ddscxx_serdata;

template <typename T>
bool serdata_eqkey(const ddsi_serdata* a, const ddsi_serdata* b)
{
  auto s_a = static_cast<const ddscxx_serdata<T>*>(a);
  auto s_b = static_cast<const ddscxx_serdata<T>*>(b);

  return 0 == memcmp(s_a->key().value, s_b->key().value, 16);
}

template <typename T>
uint32_t serdata_size(const ddsi_serdata* dcmn)
{
  return static_cast<uint32_t>(static_cast<const ddscxx_serdata<T>*>(dcmn)->size());
}

template <typename T>
ddsi_serdata *serdata_from_ser(
  const ddsi_sertype* type,
  enum ddsi_serdata_kind kind,
  const struct ddsi_rdata* fragchain,
  size_t size)
{
  auto d = new ddscxx_serdata<T>(type, kind);
  d->resize(size);
  auto cursor = static_cast<unsigned char*>(d->data());
  org::eclipse::cyclone::core::cdr::serdata_from_ser_copyin_fragchain (cursor, fragchain, size);

  if (d->getT())
  {
    d->key_md5_hashed() = to_key(*d->getT(), d->key());
    d->populate_hash();
  }
  else
  {
    delete d;
    d = nullptr;
  }

  return d;
}

template <typename T>
ddsi_serdata *serdata_from_ser_iov(
  const ddsi_sertype* type,
  enum ddsi_serdata_kind kind,
  ddsrt_msg_iovlen_t niov,
  const ddsrt_iovec_t* iov,
  size_t size)
{
  auto d = new ddscxx_serdata<T>(type, kind);
  d->resize(size);

  size_t off = 0;
  auto cursor = static_cast<unsigned char*>(d->data());
  for (ddsrt_msg_iovlen_t i = 0; i < niov && off < size; i++)
  {
    size_t n_bytes = iov[i].iov_len;
    if (n_bytes + off > size) n_bytes = size - off;
    memcpy(cursor, iov[i].iov_base, n_bytes);
    cursor += n_bytes;
    off += n_bytes;
  }

  T* ptr = d->getT();
  if (ptr) {
    d->key_md5_hashed() = to_key(*ptr, d->key());
    d->populate_hash();
  } else {
    delete d;
    d = nullptr;
  }

  return d;

}

template <typename T>
ddsi_serdata *serdata_from_keyhash(
  const ddsi_sertype* type,
  const struct ddsi_keyhash* keyhash)
{
  (void)keyhash;
  (void)type;
  //replace with (if key_size_max <= 16) then populate the data class with the key hash
  return nullptr;
}

template <typename T, class S>
ddsi_serdata *serdata_from_sample(
  const ddsi_sertype* typecmn,
  enum ddsi_serdata_kind kind,
  const void* sample)
{
  assert(kind != SDK_EMPTY);
  auto d = new ddscxx_serdata<T>(typecmn, kind);
  const auto& msg = *static_cast<const T*>(sample);
  size_t sz = 0;
  const bool k = (kind == SDK_KEY);

  if ((k && !get_serialized_size<T,S,key_mode::unsorted>(msg, sz)) ||
      (!k && !get_serialized_size<T,S,key_mode::not_key>(msg, sz)))
    goto failure;

  sz += CDR_HEADER_SIZE;
  d->resize(sz);

  if (!serialize_into<T,S>(d->data(), sz, msg, k ? key_mode::unsorted : key_mode::not_key))
    goto failure;

  d->key_md5_hashed() = to_key(msg, d->key());
  d->setT(&msg);
  d->populate_hash();
  return d;

failure:
  if (d)
    delete d;
  return nullptr;
}

template <typename T>
void serdata_to_ser(const ddsi_serdata* dcmn, size_t off, size_t sz, void* buf)
{
  auto d = static_cast<const ddscxx_serdata<T>*>(dcmn);
  memcpy(buf, calc_offset(d->data(), static_cast<ptrdiff_t>(off)), sz);
}

template <typename T>
ddsi_serdata *serdata_to_ser_ref(
  const ddsi_serdata* dcmn, size_t off,
  size_t sz, ddsrt_iovec_t* ref)
{
  auto d = static_cast<const ddscxx_serdata<T>*>(dcmn);
  ref->iov_base = calc_offset(d->data(), static_cast<ptrdiff_t>(off));
  ref->iov_len = static_cast<ddsrt_iov_len_t>(sz);
  return ddsi_serdata_ref(d);
}

template <typename T>
void serdata_to_ser_unref(ddsi_serdata* dcmn, const ddsrt_iovec_t* ref)
{
  static_cast<void>(ref);    // unused
  ddsi_serdata_unref(static_cast<ddscxx_serdata<T>*>(dcmn));
}

template <typename T>
bool serdata_to_sample(
  const ddsi_serdata* dcmn, void* sample, void** bufptr,
  void* buflim)
{
  (void)bufptr;
  (void)buflim;

  auto typed_sample_ptr = static_cast<T*>(sample);
  // cast away const, with the reasoning that we don't modify the underlying ddsi_serdata which
  // is actually const, we only modify the ddscxx_serdata non const contents
  auto d = const_cast<ddscxx_serdata<T>*>(static_cast<const ddscxx_serdata<T>*>(dcmn));

  auto t_ptr = d->getT();
  if (!t_ptr)
    return false;

  *typed_sample_ptr = *t_ptr;
  return true;
}

template <typename T, class S>
ddsi_serdata *serdata_to_untyped(const ddsi_serdata* dcmn)
{
  /* Cast away const: the serialized ddsi_serdata itself is not touched: only its C++ representation
   * in the C++ wrapper may initialized if this was not done before. So conceptually the const for
   * ddsi_serdata is not violated.
   */
  auto d = const_cast<ddscxx_serdata<T>*>(static_cast<const ddscxx_serdata<T>*>(dcmn));
  auto d1 = new ddscxx_serdata<T>(d->type, SDK_KEY);
  d1->type = nullptr;

  auto t = d->getT();
  size_t sz = 0;
  if (t == nullptr || !get_serialized_size<T,S,key_mode::unsorted>(*t, sz))
    goto failure;

  sz += CDR_HEADER_SIZE;
  d1->resize(sz);

  if (!serialize_into<T,S>(d1->data(), sz, *t, key_mode::unsorted))
    goto failure;

  d1->key_md5_hashed() = to_key(*t, d1->key());
  d1->hash = d->hash;

  return d1;

failure:
  delete d1;
  return nullptr;
}

template <typename T>
bool serdata_untyped_to_sample(
  const ddsi_sertype* type,
  const ddsi_serdata* dcmn, void* sample,
  void** bufptr, void* buflim)
{
  (void)type;
  (void)bufptr;
  (void)buflim;

  auto d = static_cast<const ddscxx_serdata<T>*>(dcmn);
  T* ptr = static_cast<T*>(sample);

  return deserialize_sample_from_buffer(d->data(), d->size(), *ptr, SDK_KEY);
}

template <typename T>
void serdata_free(ddsi_serdata* dcmn)
{
  delete static_cast<ddscxx_serdata<T>*>(dcmn);
}

template <typename T>
size_t serdata_print(
  const ddsi_sertype* tpcmn, const ddsi_serdata* dcmn, char* buf, size_t bufsize)
{
  (void)tpcmn;
  (void)dcmn;
  //implementation to follow!!!
  if (bufsize > 0)
    buf[0] = 0x0;
  return 0;
}

template <typename T>
void serdata_get_keyhash(
  const ddsi_serdata* d, struct ddsi_keyhash* buf,
  bool force_md5)
{
  auto ptr = static_cast<const ddscxx_serdata<T>*>(d);
  assert(buf);
  if (force_md5 && !ptr->key_md5_hashed())
  {
    ddsrt_md5_state_t md5st;
    ddsrt_md5_init(&md5st);
    ddsrt_md5_append(&md5st, static_cast<const ddsrt_md5_byte_t*>(ptr->key().value), 16);
    ddsrt_md5_finish(&md5st, static_cast<ddsrt_md5_byte_t*>(buf->value));
  }
  else
  {
    memcpy(buf->value, ptr->key().value, 16);
  }
}

static bool psmx_endpoint_serialization_required(struct dds_psmx_endpoint *psmx_endpoint)
{
  assert (psmx_endpoint && psmx_endpoint->psmx_topic);
  return psmx_endpoint->psmx_topic->ops.serialization_required (psmx_endpoint->psmx_topic->data_type_props);
}

template<typename T>
struct ddsi_serdata* serdata_from_loaned_sample (
  const struct ddsi_sertype *type,
  enum ddsi_serdata_kind kind,
  const char *sample,
  struct dds_loaned_sample *loan,
  bool force_serialization)
{
  assert (loan->loan_origin.origin_kind == DDS_LOAN_ORIGIN_KIND_PSMX);
  bool serialize_data = force_serialization || psmx_endpoint_serialization_required(loan->loan_origin.psmx_endpoint);

  const T* sample_in = reinterpret_cast<const T*>(sample);
  T* sample_out = static_cast<T*>(loan->sample_ptr);
  struct dds_psmx_metadata *metadata = loan->metadata;
  ddscxx_serdata<T> *d = new ddscxx_serdata<T>(type, kind);

  uint16_t hdr[CDR_HEADER_SIZE*sizeof(uint8_t)/sizeof(uint16_t)] = {DDSI_RTPS_SAMPLE_NATIVE, 0};
  if (sample_out != sample_in) //the sample we are sending across PSMX has been supplied during the call to dds_write, not a loan from the user
  {
    assert (metadata->sample_state == DDS_LOANED_SAMPLE_STATE_UNITIALIZED);
    if (serialize_data)
    {
      bool ser_result = false;
      const bool key = (kind == SDK_KEY);
      if(TopicTraits<T>::allowableEncodings() & DDS_DATA_REPRESENTATION_FLAG_XCDR1)
          ser_result = serialize_into_impl<T,basic_cdr_stream>(hdr, sample_out, metadata->sample_size, *sample_in, key ? key_mode::unsorted : key_mode::not_key);
      else if (TopicTraits<T>::allowableEncodings() & DDS_DATA_REPRESENTATION_FLAG_XCDR2)
          ser_result = serialize_into_impl<T,xcdr_v2_stream>(hdr, sample_out, metadata->sample_size, *sample_in, key ? key_mode::unsorted : key_mode::not_key);
      if (!ser_result)  //serialization unsuccesful, abort
      {
        delete d;
        return nullptr;
      }
      metadata->sample_state = (key ? DDS_LOANED_SAMPLE_STATE_SERIALIZED_KEY : DDS_LOANED_SAMPLE_STATE_SERIALIZED_DATA);
    }
    else
    {
      // if you are not serializing, you still need to copy into the loaned sample
      *sample_out = *sample_in;
    }
  }

  if (!serialize_data)
    metadata->sample_state = DDS_LOANED_SAMPLE_STATE_RAW;

  metadata->cdr_identifier = hdr[0];  //remove endianness?
  metadata->cdr_options = hdr[1];
  metadata->hash = d->hash;

  d->key_md5_hashed() = to_key(*sample_in, d->key());
  d->populate_hash();
  d->loan = loan;

  return d;
}


template<typename T>
struct ddsi_serdata* serdata_from_psmx (
  const struct ddsi_sertype *type,
  struct dds_loaned_sample *loan)
{
  struct dds_psmx_metadata *md = loan->metadata;
  enum ddsi_serdata_kind kind = SDK_EMPTY;
  switch (md->sample_state)
  {
    case DDS_LOANED_SAMPLE_STATE_RAW:
    case DDS_LOANED_SAMPLE_STATE_SERIALIZED_DATA:
      kind = SDK_DATA;
      break;
    case DDS_LOANED_SAMPLE_STATE_SERIALIZED_KEY:
      kind = SDK_KEY;
      break;
    default:
      return nullptr;
  }

  ddscxx_serdata<T> *d = new ddscxx_serdata<T>(type, kind);
  if (DDS_LOANED_SAMPLE_STATE_RAW != md->sample_state)
  {
    bool deser_result = false;
    const endianness end = (md->cdr_identifier & BO_LITTLE ? endianness::little_endian : endianness::big_endian);
    switch (md->cdr_identifier & ~BO_LITTLE)
    {
      case PLAIN_CDR:
        if (TopicTraits<T>::allowableEncodings() & DDS_DATA_REPRESENTATION_FLAG_XCDR1)
          deser_result = deserialize_sample_from_buffer_impl<T,basic_cdr_stream>(loan->sample_ptr, md->sample_size, *(d->getT(false)), kind, end);
        else
          deser_result = deserialize_sample_from_buffer_impl<T,xcdr_v1_stream>(loan->sample_ptr, md->sample_size, *(d->getT(false)), kind, end);
        break;
      case PL_CDR:
        deser_result = deserialize_sample_from_buffer_impl<T,xcdr_v1_stream>(loan->sample_ptr, md->sample_size, *(d->getT(false)), kind, end);
        break;
      case PLAIN_CDR2:
      case D_CDR:
      case PL_CDR2:
        deser_result = deserialize_sample_from_buffer_impl<T,xcdr_v2_stream>(loan->sample_ptr, md->sample_size, *(d->getT(false)), kind, end);
        break;
    }
    if (!deser_result)  //deserialization unsuccesful, abort
    {
      delete d;
      return nullptr;
    }
  }
  else
  {
    d->setLoan(loan);
  }


  d->key_md5_hashed() = to_key(*(d->getT()), d->key());
  d->populate_hash();
  d->statusinfo = md->statusinfo;
  d->timestamp.v = md->timestamp;

  return d;
}

template<typename T,
         class S >
struct ddscxx_serdata_ops: public ddsi_serdata_ops {
  ddscxx_serdata_ops(): ddsi_serdata_ops {
  &serdata_eqkey<T>,
  &serdata_size<T>,
  &serdata_from_ser<T>,
  &serdata_from_ser_iov<T>,
  &serdata_from_keyhash<T>,
  &serdata_from_sample<T, S>,
  &serdata_to_ser<T>,
  &serdata_to_ser_ref<T>,
  &serdata_to_ser_unref<T>,
  &serdata_to_sample<T>,
  &serdata_to_untyped<T, S>,
  &serdata_untyped_to_sample<T>,
  &serdata_free<T>,
  &serdata_print<T>,
  &serdata_get_keyhash<T>,
  &serdata_from_loaned_sample<T>,
  &serdata_from_psmx<T>
  } { ; }
};

template <typename T>
class ddscxx_serdata : public ddsi_serdata {
  size_t m_size{ 0 };
  std::unique_ptr<unsigned char[]> m_data{ nullptr };
  ddsi_keyhash_t m_key;
  bool m_key_md5_hashed = false;
  std::atomic<T *> m_t;  //use a recursive mutex and do all modifications inside it?

public:
  bool hash_populated = false;
  ddscxx_serdata(const ddsi_sertype* type, ddsi_serdata_kind kind);
  ~ddscxx_serdata();
  void resize(size_t requested_size);
  size_t size() const { return m_size; }
  void* data() const { return m_data.get(); }
  ddsi_keyhash_t& key() { return m_key; }
  const ddsi_keyhash_t& key() const { return m_key; }
  bool& key_md5_hashed() { return m_key_md5_hashed; }
  const bool& key_md5_hashed() const { return m_key_md5_hashed; }
  void populate_hash();
  T* setT(const T* toset);
  T* getT(bool force_deserialization = true);
  void setLoan(dds_loaned_sample_t *newloan);

private:
  void deserialize_and_update_sample(uint8_t * buffer, size_t sz, T *& t, bool force_deserialization);
};

template <typename T>
ddscxx_serdata<T>::ddscxx_serdata(const ddsi_sertype* type, ddsi_serdata_kind kind)
  : ddsi_serdata{}, m_t(nullptr)
{
  memset(m_key.value, 0x0, 16);
  ddsi_serdata_init(this, type, kind);
}

template <typename T>
ddscxx_serdata<T>::~ddscxx_serdata()
{
  T* t = m_t.load(std::memory_order_acquire);
  if (!loan || loan->sample_ptr != t)
    delete t;
  if (loan)
    dds_loaned_sample_unref (loan);
}

template <typename T>
void ddscxx_serdata<T>::resize(size_t requested_size)
{
  if (!requested_size) {
    m_size = 0;
    m_data.reset();
    return;
  }

  /* FIXME: CDR padding in DDSI makes me do this to avoid reading beyond the bounds
  when copying data to network.  Should fix Cyclone to handle that more elegantly.  */
  size_t n_pad_bytes = (0 - requested_size) % 4;
  m_data.reset(new unsigned char[requested_size + n_pad_bytes]);
  m_size = requested_size + n_pad_bytes;

  // zero the very end. The caller isn't necessarily going to overwrite it.
  std::memset(calc_offset(m_data.get(), static_cast<ptrdiff_t>(requested_size)), '\0', n_pad_bytes);
}

template <typename T>
void ddscxx_serdata<T>::populate_hash()
{
  if (hash_populated)
    return;

  key_md5_hashed() = to_key(*getT(), key());
  if (!key_md5_hashed())
  {
    ddsi_keyhash_t buf;
    ddsrt_md5_state_t md5st;
    ddsrt_md5_init(&md5st);
    ddsrt_md5_append(&md5st, static_cast<const ddsrt_md5_byte_t*>(key().value), 16);
    ddsrt_md5_finish(&md5st, static_cast<ddsrt_md5_byte_t*>(buf.value));
    memcpy(&(hash), buf.value, 4);
  }
  else
  {
    memcpy(&(hash), key().value, 4);
  }

  hash ^= type->serdata_basehash;
  hash_populated = true;
}

template <typename T>
T* ddscxx_serdata<T>::setT(const T* toset)
{
  assert(toset);
  T* t = m_t.load(std::memory_order_acquire);
  if (t == nullptr) {
    t = new T(*toset);
    T* exp = nullptr;
    if (!m_t.compare_exchange_strong(exp, t, std::memory_order_seq_cst)) {
      delete t;
      t = exp;
    }
  } else {
    *t = *toset;
  }
  return t;
}

template <typename T>
T* ddscxx_serdata<T>::getT(bool force_deserialization) {
  // check if m_t is already set
  T *t = m_t.load(std::memory_order_acquire);
  // if m_t is not set
  if (t == nullptr) {
    deserialize_and_update_sample(static_cast<uint8_t *>(data()), size(), t, force_deserialization);
  }
  return t;
}

template <typename T>
void ddscxx_serdata<T>::deserialize_and_update_sample(uint8_t * buffer, size_t sz, T *& t, bool force_deserialization) {
  t = new T();
  // if deserialization failed
  if (force_deserialization &&
      !deserialize_sample_from_buffer(buffer, sz, *t, kind)) {
    delete t;
    t = nullptr;
  }

  T* exp = nullptr;
  if (!m_t.compare_exchange_strong(exp, t, std::memory_order_seq_cst)) {
    delete t;
    t = exp;
  }
}

template<typename T>
void ddscxx_serdata<T>::setLoan(dds_loaned_sample_t *newloan)
{
  T *t = m_t.load(std::memory_order_acquire);
  if (!loan || t != loan->sample_ptr)
    delete t;
  if (loan)
    dds_loaned_sample_unref(loan);

  if (!m_t.compare_exchange_strong(t, static_cast<T*>(newloan->sample_ptr), std::memory_order_seq_cst))
  {
    //something went wrong
  }

  dds_loaned_sample_ref(newloan);
  loan = newloan;
}

template <typename T, class S>
class ddscxx_sertype;

template <typename T, class S>
void sertype_free(ddsi_sertype* tpcmn)
{
  auto tp = static_cast<ddscxx_sertype<T,S>*>(tpcmn);
  ddsi_sertype_fini(tpcmn);
  delete tp;
}

template <typename T>
void sertype_zero_samples(const ddsi_sertype*, void*, size_t)
{
  return;
}

template <typename T>
void sertype_realloc_samples(
  void** ptrs, const ddsi_sertype*, void*, size_t, size_t)
{
  /* For C++ we make one big assumption about the caller of this function:
   * it can only be invoked by the ddsi_sertype_alloc_sample, and will therefore
   * never be used to reallocate an existing sample collection. This is caused by
   * the fact that the C++ API lets either the user specify the exact dimensions
   * of his preallocated collection (in which case there is no need to realloc them),
   * or if the user didn't preallocate any memory it peeks at the available
   * samples prior to allocating the sample collection that is returned (so that
   * again there is no need to reallocate it).
   * Because of this, we can safely assume that sertype_realloc_samples can only
   * be invoked by ddsi_sertype_alloc_sample, in which case oldCount is always 0,
   * count is always 1 and the old pointer is always null.
   */
  ptrs[0] = new T();
}

template <typename T>
void sertype_free_samples(
  const ddsi_sertype*, void** ptrs, size_t, dds_free_op_t op)
{
  /* For C++ we make one big assumption about the caller of this function:
   * it can only be invoked by the ddsi_sertype_free_sample, and will therefore
   * never be used to free an existing sample collection. This is caused by
   * the fact that the C++ API lets either the user specify the exact dimensions
   * of his preallocated collection (in which case there is no need to release
   * it in the cyclone code base), or if the user didn't preallocate any memory it
   * returns a collection of samples that will be owned by the user (in which case
   * cyclone doesn't need to release the collection either).
   * Because of this, we can safely assume that sertype_free_samples can only
   * be invoked by ddsi_sertype_free_sample, in which case count is always 1,
   * and the op flags can either be set to DDS_FREE_ALL_BIT, or to DDS_FREE_CONTENTS_BIT.
   */
  T* ptr = reinterpret_cast<T *>(ptrs[0]);
  if (op & DDS_FREE_ALL_BIT) {
    delete ptr;
  } else {
    assert(op & DDS_FREE_CONTENTS_BIT);
    *ptr = T();
  }
}

template <typename T>
bool sertype_equal(
  const ddsi_sertype* acmn, const ddsi_sertype* bcmn)
{
  /* A bit of a guess: types with the same name & type name are really the same if they have
   the same type support identifier as well */
  (void)acmn;
  (void)bcmn;
  return true;
}

template <typename T>
uint32_t sertype_hash(const ddsi_sertype* tpcmn)
{
  (void)tpcmn;
  return 0x0;
}

template <typename T, class S>
size_t sertype_get_serialized_size(const ddsi_sertype*, const void * sample)
{
  const auto& msg = *static_cast<const T*>(sample);

  // get the serialized size of the sample (with out serializing)
  size_t sz = 0;
  if (!get_serialized_size<T,S,key_mode::not_key>(msg, sz)) {
    // the max value is treated as an error in the Cyclone core
    return SIZE_MAX;
  }

  return sz + CDR_HEADER_SIZE;  // Include the additional bytes for the CDR header
}

template <typename T, class S>
bool sertype_serialize_into(const ddsi_sertype*,
                            const void * sample,
                            void * dst_buffer,
                            size_t sz)
{
  // cast to the type
  const auto& msg = *static_cast<const T*>(sample);

  return serialize_into<T,S>(dst_buffer, sz, msg, key_mode::not_key);
}

template<typename T,
         class S >
struct ddscxx_sertype_ops: public ddsi_sertype_ops {
  ddscxx_sertype_ops(): ddsi_sertype_ops {
  ddsi_sertype_v0,
  nullptr,
  sertype_free<T,S>,
  sertype_zero_samples<T>,
  sertype_realloc_samples<T>,
  sertype_free_samples<T>,
  sertype_equal<T>,
  sertype_hash<T>,
  #ifdef DDSCXX_HAS_TYPELIB
  TopicTraits<T>::getTypeId,
  TopicTraits<T>::getTypeMap,
  TopicTraits<T>::getTypeInfo,
  #else
  nullptr,
  nullptr,
  nullptr,
  #endif //DDSCXX_HAS_TYPELIB
  TopicTraits<T>::deriveSertype,
  sertype_get_serialized_size<T,S>,
  sertype_serialize_into<T,S>
    } { ; }
};

template <typename T,
          class S >
class ddscxx_sertype : public ddsi_sertype {
public:
  static const ddscxx_sertype_ops<T,S> sertype_ops;
  static const ddscxx_serdata_ops<T,S> serdata_ops;
  ddscxx_sertype();
};

template <typename T, class S>
const ddscxx_sertype_ops<T,S> ddscxx_sertype<T,S>::sertype_ops;

template <typename T, class S>
const ddscxx_serdata_ops<T,S> ddscxx_sertype<T,S>::serdata_ops;

template <typename T, class S>
ddscxx_sertype<T,S>::ddscxx_sertype()
  : ddsi_sertype{}
{
  uint32_t flags = (TopicTraits<T>::isKeyless() ? DDSI_SERTYPE_FLAG_TOPICKIND_NO_KEY : 0u) | (TopicTraits<T>::isSelfContained() ? DDSI_SERTYPE_FLAG_FIXED_SIZE : 0u);

  ddsi_sertype_init_flags(
      static_cast<ddsi_sertype*>(this),
      TopicTraits<T>::getTypeName(),
      &sertype_ops,
      &serdata_ops,
      flags);

  allowed_data_representation = TopicTraits<T>::allowableEncodings();
}

#endif  // DDSCXXDATATOPIC_HPP_
