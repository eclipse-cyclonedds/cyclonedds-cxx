/*
 * Copyright(c) 2020 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef DDSCXXDATATOPIC_HPP_
#define DDSCXXDATATOPIC_HPP_

#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <atomic>

#include "dds/ddsrt/endian.h"
#include "dds/ddsrt/md5.h"
#include "dds/ddsi/q_radmin.h"
#include "dds/ddsi/q_xmsg.h"
#include "dds/ddsi/ddsi_serdata.h"
#include "org/eclipse/cyclonedds/core/cdr/basic_cdr_ser.hpp"
#include "org/eclipse/cyclonedds/core/cdr/extended_cdr_v1_ser.hpp"
#include "org/eclipse/cyclonedds/core/cdr/extended_cdr_v2_ser.hpp"
#include "dds/ddsi/ddsi_keyhash.h"
#include "org/eclipse/cyclonedds/topic/hash.hpp"
#include "dds/features.hpp"

#ifdef DDSCXX_HAS_SHM
#include "dds/ddsi/ddsi_shm_transport.h"
#endif

constexpr size_t CDR_HEADER_SIZE = 4U;

using org::eclipse::cyclonedds::core::cdr::endianness;
using org::eclipse::cyclonedds::core::cdr::native_endianness;
using org::eclipse::cyclonedds::core::cdr::basic_cdr_stream;

template<class streamer, typename T>
bool to_key(streamer& str, const T& tokey, ddsi_keyhash_t& hash)
{
  if (!move(str, tokey, true))
    return false;
  size_t sz = str.position();
  size_t padding = 16 - sz % 16;
  if (sz != 0 && padding == 16) padding = 0;
  std::vector<unsigned char> buffer(sz + padding);
  memset(buffer.data() + sz, 0x0, padding);
  str.set_buffer(buffer.data(), buffer.size());
  /* TODO: what is key endianness to be used here?
   * since, this may be different between nodes, and if this value is used
   * for global lookups or the like, this
   * may cause discrepancies. */
  if (!write(str, tokey, true))
    return false;
  static bool (*fptr)(const std::vector<unsigned char>&, ddsi_keyhash_t&) = NULL;
  if (fptr == NULL)
  {
    max(str, tokey, true);
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

static inline void* calc_offset(void* ptr, ptrdiff_t n)
{
  return static_cast<void*>(static_cast<unsigned char*>(ptr) + n);
}

static inline const void* calc_offset(const void* ptr, ptrdiff_t n)
{
  return static_cast<const void*>(static_cast<const unsigned char*>(ptr) + n);
}

/// \brief De-serialize the buffer into the sample
/// \param[in] buffer The buffer to be de-serialized
/// \param[out] sample Type to which the buffer will be de-serialized
/// \param[in] data_kind The data kind (data, or key)
/// \tparam T The sample type
/// \return True if the deserialization is successful
///         False if the deserialization failed
template <typename T>
bool deserialize_sample_from_buffer(unsigned char * buffer,
                                    T & sample,
                                    const ddsi_serdata_kind data_kind=SDK_DATA)
{
  assert(data_kind != SDK_EMPTY);

  org::eclipse::cyclonedds::core::cdr::basic_cdr_stream str(*(buffer + 1) == 0x1 ? endianness::little_endian : endianness::big_endian);
  str.set_buffer(calc_offset(buffer, CDR_HEADER_SIZE));
  return read(str, sample, data_kind == SDK_KEY);
}

template <typename T>
class ddscxx_sertype : public ddsi_sertype {
public:
  static const ddsi_sertype_ops ddscxx_sertype_ops;
  ddscxx_sertype();
};

template <typename T>
class ddscxx_serdata : public ddsi_serdata {
  size_t m_size{ 0 };
  std::unique_ptr<unsigned char[]> m_data{ nullptr };
  ddsi_keyhash_t m_key;
  bool m_key_md5_hashed = false;
  std::atomic<T *> m_t{ nullptr };

public:
  bool hash_populated = false;
  static const ddsi_serdata_ops ddscxx_serdata_ops;
  ddscxx_serdata(const ddsi_sertype* type, ddsi_serdata_kind kind);
  ~ddscxx_serdata() { delete m_t.load(std::memory_order_acquire); }

  void resize(size_t requested_size);
  size_t size() const { return m_size; }
  void* data() const { return m_data.get(); }
  ddsi_keyhash_t& key() { return m_key; }
  const ddsi_keyhash_t& key() const { return m_key; }
  bool& key_md5_hashed() { return m_key_md5_hashed; }
  const bool& key_md5_hashed() const { return m_key_md5_hashed; }
  void populate_hash();

  T* setT(const T* toset)
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

  T* getT() {
    // check if m_t is already set
    T *t = m_t.load(std::memory_order_acquire);
    // if m_t is not set
    if (t == nullptr) {
      // if the data is available on iox_chunk, update and get the sample
      update_sample_from_iox_chunk(t);
      // if its not possible to get the sample from iox_chunk
      if(t == nullptr) {
        // deserialize and get the sample
        deserialize_and_update_sample(static_cast<uint8_t *>(data()), t);
      }
    }
    return t;
  }

private:
  void deserialize_and_update_sample(uint8_t * buffer, T *& t) {
    t = new T();
    // if deserialization failed
    if(!deserialize_sample_from_buffer(buffer, *t, kind)) {
      delete t;
      t = nullptr;
    }

    T* exp = nullptr;
    if (!m_t.compare_exchange_strong(exp, t, std::memory_order_seq_cst)) {
      delete t;
      t = exp;
    }
  }

  void update_sample_from_iox_chunk(T *& t) {
#ifdef DDSCXX_HAS_SHM
    // if data is available on the iox_chunk (and doesn't have a serialized representation)
    if (iox_chunk != nullptr && data() == nullptr) {
        auto shm_data_state = shm_get_data_state(iox_chunk);
        // if the iox chunk has the data in serialized form
        if (shm_data_state == IOX_CHUNK_CONTAINS_SERIALIZED_DATA) {
          deserialize_and_update_sample(static_cast<uint8_t *>(iox_chunk), t);
        } else if (shm_data_state == IOX_CHUNK_CONTAINS_RAW_DATA) {
          // get the chunk directly without any copy
          t = static_cast<T*>(this->iox_chunk);
        } else {
          // Data is in un-initialized state, which shouldn't happen
          t = nullptr;
        }
      } else {
      // data is not available on iox_chunk
      t = nullptr;
    }
#else
    t = nullptr;
#endif  // DDSCXX_HAS_SHM
  }
};

template <typename T>
void ddscxx_serdata<T>::populate_hash()
{
  if (hash_populated)
    return;

  org::eclipse::cyclonedds::core::cdr::basic_cdr_stream str;
  key_md5_hashed() = to_key(str, *getT(), key());
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

  hash_populated = true;
}

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
  const struct nn_rdata* fragchain,
  size_t size)
{
  auto d = new ddscxx_serdata<T>(type, kind);

  uint32_t off = 0;
  assert(fragchain->min == 0);
  assert(fragchain->maxp1 >= off);    //CDR header must be in first fragment

  d->resize(size);

  auto cursor = static_cast<unsigned char*>(d->data());
  while (fragchain) {
    if (fragchain->maxp1 > off) {
      //only copy if this fragment adds data
      const unsigned char* payload =
        NN_RMSG_PAYLOADOFF(fragchain->rmsg, NN_RDATA_PAYLOAD_OFF(fragchain));
      auto src = payload + off - fragchain->min;
      auto n_bytes = fragchain->maxp1 - off;
      memcpy(cursor, src, n_bytes);
      cursor += n_bytes;
      off = fragchain->maxp1;
      assert(off <= size);
    }
    fragchain = fragchain->nextfrag;
  }

  if (d->getT())
  {
    org::eclipse::cyclonedds::core::cdr::basic_cdr_stream str;
    str.set_buffer(calc_offset(d->data(), 4), size-4);
    d->key_md5_hashed() = to_key(str, *d->getT(), d->key());
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
    org::eclipse::cyclonedds::core::cdr::basic_cdr_stream str;
    str.set_buffer(calc_offset(d->data(), 4), size-4);
    d->key_md5_hashed() = to_key(str, *ptr, d->key());
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
ddsi_serdata *serdata_from_sample(
  const ddsi_sertype* typecmn,
  enum ddsi_serdata_kind kind,
  const void* sample)
{
  assert(kind != SDK_EMPTY);
  auto d = new ddscxx_serdata<T>(typecmn, kind);
  org::eclipse::cyclonedds::core::cdr::basic_cdr_stream str;
  const auto& msg = *static_cast<const T*>(sample);
  unsigned char *ptr = nullptr;
  size_t sz = 0;

  if (!move(str, msg, kind == SDK_KEY))
    goto failure;

  sz = 4 + str.position();  //4 bytes extra to also include the header
  d->resize(sz);
  ptr = static_cast<unsigned char*>(d->data());
  memset(ptr, 0x0, 4);
  if (native_endianness() == endianness::little_endian)
    *(ptr + 1) = 0x1;

  str.set_buffer(calc_offset(d->data(), 4), sz-4);

  if (!write(str, msg, kind == SDK_KEY))
    goto failure;

  d->key_md5_hashed() = to_key(str, msg, d->key());
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
  auto ptr = static_cast<const ddscxx_serdata<T>*>(dcmn);

  auto& msg = *static_cast<T*>(sample);
  return deserialize_sample_from_buffer(static_cast<unsigned char*>(ptr->data()), msg);
}

template <typename T>
ddsi_serdata *serdata_to_untyped(const ddsi_serdata* dcmn)
{
  /* Cast away const: the serialized ddsi_serdata itself is not touched: only its C++ representation
   * in the C++ wrapper may initialized if this was not done before. So conceptually the const for
   * ddsi_serdata is not violated.
   */
  auto d = const_cast<ddscxx_serdata<T>*>(static_cast<const ddscxx_serdata<T>*>(dcmn));
  auto d1 = new ddscxx_serdata<T>(d->type, SDK_KEY);
  unsigned char *ptr = nullptr;
  d1->type = nullptr;

  basic_cdr_stream str;
  auto t = d->getT();
  if (t == nullptr)
    goto failure;

  if (!move(str, *t, true))
    goto failure;
  d1->resize(4 + str.position());

  ptr = static_cast<unsigned char*>(d1->data());
  memset(ptr, 0x0, 4);
  if (native_endianness() == endianness::little_endian)
    *(ptr + 1) = 0x1;

  str.set_buffer(calc_offset(d1->data(), 4), d1->size()-4);  //4 offset due to header field
  if (!write(str, *t, true))
    goto failure;

  d1->key_md5_hashed() = to_key(str, *t, d1->key());
  d1->hash = d->hash;
  d1->hash_populated = true;

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

  return deserialize_sample_from_buffer(static_cast<unsigned char*>(d->data()), *ptr, SDK_KEY);
}

template <typename T>
void serdata_free(ddsi_serdata* dcmn)
{
  auto* d = static_cast<ddscxx_serdata<T>*>(dcmn);

#ifdef DDSCXX_HAS_SHM
  if (d->iox_chunk && d->iox_subscriber)
  {
    // Explicit cast to iox_subscriber is required here, since the C++ binding has no notion of
    // iox subscriber, but the underlying C API expects this to be a typed iox_subscriber.
    // TODO (Sumanth), Fix this when we cleanup the interfaces to not use iceoryx directly in
    //  the C++ plugin
    free_iox_chunk(static_cast<iox_sub_t *>(d->iox_subscriber), &d->iox_chunk);
  }
#endif
  delete d;
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

#ifdef DDSCXX_HAS_SHM
template<typename T>
uint32_t serdata_iox_size(const struct ddsi_serdata* d)
{
  assert(sizeof(T) == d->type->iox_size);
  return d->type->iox_size;
}

template<typename T>
ddsi_serdata * serdata_from_iox_buffer(
    const struct ddsi_sertype * typecmn, enum ddsi_serdata_kind kind,
    void * sub, void * iox_buffer)
{
  try {
    auto d = new ddscxx_serdata<T>(typecmn, kind);

    // serdata from the loaned sample (when using iceoryx)
    d->iox_chunk = iox_buffer;

    // Update the iox subscriber, when constructing the serdata in the case of sample received
    // from iceoryx
    if (sub != nullptr) {
      d->iox_subscriber = sub;
    }

    // key handling
    org::eclipse::cyclonedds::core::cdr::basic_cdr_stream str;
    const auto& msg = *static_cast<const T*>(d->iox_chunk);
    d->key_md5_hashed() = to_key(str, msg, d->key());
    d->populate_hash();

    return d;
  }
  catch (std::exception&) {
    return nullptr;
  }
}
#endif

template <typename T>
const ddsi_serdata_ops ddscxx_serdata<T>::ddscxx_serdata_ops = {
  &serdata_eqkey<T>,
  &serdata_size<T>,
  &serdata_from_ser<T>,
  &serdata_from_ser_iov<T>,
  &serdata_from_keyhash<T>,
  &serdata_from_sample<T>,
  &serdata_to_ser<T>,
  &serdata_to_ser_ref<T>,
  &serdata_to_ser_unref<T>,
  &serdata_to_sample<T>,
  &serdata_to_untyped<T>,
  &serdata_untyped_to_sample<T>,
  &serdata_free<T>,
  &serdata_print<T>,
  &serdata_get_keyhash<T>,
#ifdef DDSCXX_HAS_SHM
  &serdata_iox_size<T>,
  &serdata_from_iox_buffer<T>
#endif
};

template <typename T>
ddscxx_serdata<T>::ddscxx_serdata(const ddsi_sertype* type, ddsi_serdata_kind kind)
  : ddsi_serdata{}
{
  memset(m_key.value, 0x0, 16);
  ddsi_serdata_init(this, type, kind);
}

template <typename T>
ddscxx_sertype<T>::ddscxx_sertype()
  : ddsi_sertype{}
{
  uint32_t flags = (org::eclipse::cyclonedds::topic::TopicTraits<T>::isKeyless() ?
                    DDSI_SERTYPE_FLAG_TOPICKIND_NO_KEY : 0);
#ifdef DDSCXX_HAS_SHM
  flags |= (org::eclipse::cyclonedds::topic::TopicTraits<T>::isSelfContained() ?
      DDSI_SERTYPE_FLAG_FIXED_SIZE : 0);
#endif

  ddsi_sertype_init_flags(
      static_cast<ddsi_sertype*>(this),
      org::eclipse::cyclonedds::topic::TopicTraits<T>::getTypeName(),
      &ddscxx_sertype<T>::ddscxx_sertype_ops,
      &ddscxx_serdata<T>::ddscxx_serdata_ops,
      flags);

#ifdef DDSCXX_HAS_SHM
  // update the size of the type, if its fixed
  // this needs to be done after sertype init! TODO need an API in Cyclone DDS to set this
  this->iox_size =
      static_cast<uint32_t>(org::eclipse::cyclonedds::topic::TopicTraits<T>::getSampleSize());
#endif
}

template <typename T>
void sertype_free(ddsi_sertype* tpcmn)
{
  auto tp = static_cast<ddscxx_sertype<T>*>(tpcmn);
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

template <typename T>
size_t sertype_get_serialized_size(const ddsi_sertype*, const void * sample)
{
  const auto& msg = *static_cast<const T*>(sample);

  // get the serialized size of the sample (with out serializing)
  org::eclipse::cyclonedds::core::cdr::basic_cdr_stream str;
  if (!move(str, msg, false)) {
    // the max value is treated as an error in the Cyclone core
    return SIZE_MAX;
  }

  return str.position() + CDR_HEADER_SIZE;  // Include the additional bytes for the CDR header
}

template <typename T>
bool sertype_serialize_into(const ddsi_sertype*,
                            const void * sample,
                            void * dst_buffer,
                            size_t)
{
  // cast to the type
  const auto& msg = *static_cast<const T*>(sample);

  // set the endianess
  auto ptr = static_cast<unsigned char*>(dst_buffer);
  memset(ptr, 0x0, 4);
  if (native_endianness() == endianness::little_endian)
    *(ptr + 1) = 0x1;

  // serialize the sample into the destination buffer
  org::eclipse::cyclonedds::core::cdr::basic_cdr_stream str;
  // TODO(Sumanth), considering the header offset
  str.set_buffer(calc_offset(dst_buffer, 4));  //buffer size?
  return write(str, msg, false);
}

template <typename T>
const ddsi_sertype_ops ddscxx_sertype<T>::ddscxx_sertype_ops = {
  ddsi_sertype_v0,
  nullptr,
  sertype_free<T>,
  sertype_zero_samples<T>,
  sertype_realloc_samples<T>,
  sertype_free_samples<T>,
  sertype_equal<T>,
  sertype_hash<T>,
  nullptr, // type_id
  nullptr, // type_map
  nullptr, // type_info
  nullptr, // assignable_from
  nullptr, // derive_sertype
  sertype_get_serialized_size<T>,
  sertype_serialize_into<T>
};

#endif  // DDSCXXDATATOPIC_HPP_
