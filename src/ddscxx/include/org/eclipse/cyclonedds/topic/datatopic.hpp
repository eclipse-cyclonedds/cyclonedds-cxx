// Copyright 2019 ADLINK Technology
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#ifndef DDSCXXDATATOPIC_HPP_
#define DDSCXXDATATOPIC_HPP_

#include <memory>
#include <string>
#include <cstring>
#include <vector>

#include "dds/ddsrt/endian.h"
#include "dds/ddsrt/md5.h"
#include "dds/ddsi/q_radmin.h"
#include "dds/ddsi/ddsi_serdata.h"
#include "dds/ddsi/ddsi_sertopic.h"

typedef struct ddsi_serdata ddsi_serdata_t;

enum class endianness
{
  little_endian = DDSRT_LITTLE_ENDIAN,
  big_endian = DDSRT_BIG_ENDIAN
};

constexpr endianness native_endianness() { return endianness(DDSRT_ENDIAN); }

static inline void* calc_offset(void* ptr, ptrdiff_t n)
{
  return static_cast<void*>(static_cast<unsigned char*>(ptr) + n);
}

static inline const void* calc_offset(const void* ptr, ptrdiff_t n)
{
  return static_cast<const void*>(static_cast<const unsigned char*>(ptr) + n);
}

template <typename T>
class ddscxx_sertopic : public ddsi_sertopic {
public:
  static const struct ddsi_sertopic_ops ddscxx_sertopic_ops;
  ddscxx_sertopic(const char* topic_name, const char* type_name);
};

template <typename T>
class ddscxx_serdata : public ddsi_serdata {
  size_t m_size{ 0 };
  std::unique_ptr<unsigned char[]> m_data{ nullptr };
  ddsi_keyhash_t m_key;
  bool m_key_md5_hashed;

public:
  static const struct ddsi_serdata_ops ddscxx_serdata_ops;
  ddscxx_serdata(const ddsi_sertopic* topic, ddsi_serdata_kind kind);

  void resize(size_t requested_size);
  size_t size() const { return m_size; }
  void* data() const { return m_data.get(); }
  ddsi_keyhash_t& key() { return m_key; }
  const ddsi_keyhash_t& key() const { return m_key; }
  bool& key_md5_hashed() { return m_key_md5_hashed; }
  const bool& key_md5_hashed() const { return m_key_md5_hashed; }
};

template <typename T>
bool serdata_eqkey(const struct ddsi_serdata* a, const struct ddsi_serdata* b)
{
  auto s_a = static_cast<const ddscxx_serdata<T>*>(a);
  auto s_b = static_cast<const ddscxx_serdata<T>*>(b);

  return 0 == memcmp(s_a->key().value, s_b->key().value, 16);
}

template <typename T>
uint32_t serdata_size(const struct ddsi_serdata* dcmn)
{
  return static_cast<uint32_t>(static_cast<const ddscxx_serdata<T>*>(dcmn)->size());
}

template <typename T>
ddsi_serdata_t *serdata_from_ser(
  const struct ddsi_sertopic* topic,
  enum ddsi_serdata_kind kind,
  const struct nn_rdata* fragchain, size_t size)
{
  auto d = new ddscxx_serdata<T>(topic, kind);

  uint32_t off = 0;
  assert(fragchain->min == 0);
  assert(fragchain->maxp1 >= off);    //CDR header must be in first fragment

  d->resize(size);

  auto cursor = (unsigned char*)d->data();
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

  T temp;
  temp.read_struct(calc_offset(d->data(), 4), 0);
  d->key_md5_hashed() = temp.key(d->key());

  return d;
}

#if DDSI_SERDATA_HAS_FROM_SER_IOV
template <typename T>
ddsi_serdata_t *serdata_from_ser_iov(
  const struct ddsi_sertopic* topic,
  enum ddsi_serdata_kind kind,
  ddsrt_msg_iovlen_t niov,
  const ddsrt_iovec_t* iov,
  size_t size)
{
  (void)topic;
  (void)kind;
  (void)niov;
  (void)iov;
  (void)size;
  assert(0);
  return NULL;
}
#endif

template <typename T>
ddsi_serdata_t *serdata_from_keyhash(
  const struct ddsi_sertopic* topic,
  const struct ddsi_keyhash* keyhash)
{
  (void)keyhash;
  /* there is no key field, so from_keyhash is trivial */
  return new ddscxx_serdata<T>(topic, SDK_KEY);
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
  std::memset(calc_offset(m_data.get(), requested_size), '\0', n_pad_bytes);
}

template <typename T>
ddsi_serdata_t *serdata_from_sample(
  const struct ddsi_sertopic* topiccmn,
  enum ddsi_serdata_kind kind,
  const void* sample)
{
  try {
    auto topic = static_cast<const ddscxx_sertopic<T>*>(topiccmn);
    auto d = new ddscxx_serdata<T>(topic, kind);

    if (kind != SDK_DATA) {
      //???
    }
    else  {
      auto msg = static_cast<const T*>(sample);

      size_t sz = msg->write_size(0);
      d->resize(sz + 4);  //4 bytes extra to also include the header
      unsigned char* ptr = (unsigned char*)d->data();
      memset(ptr, 0x0, 4);
      if (native_endianness() == endianness::little_endian)
        *(ptr + 1) = 0x1;

      msg->write_struct(calc_offset(d->data(), 4), 0);
      d->key_md5_hashed() = msg->key(d->key());
    }

    return d;
  }
  catch (std::exception&) {
    return nullptr;
  }
}

template <typename T>
void serdata_to_ser(const struct ddsi_serdata* dcmn, size_t off, size_t sz, void* buf)
{
  auto d = static_cast<const ddscxx_serdata<T>*>(dcmn);
  memcpy(buf, calc_offset(d->data(), off), sz);
}

template <typename T>
ddsi_serdata_t *serdata_to_ser_ref(
  const struct ddsi_serdata* dcmn, size_t off,
  size_t sz, ddsrt_iovec_t* ref)
{
  auto d = static_cast<const ddscxx_serdata<T>*>(dcmn);
  uintptr_t a, b;
  a = (uintptr_t)d->data();
  b = (uintptr_t)calc_offset(d->data(), off);
  ref->iov_base = calc_offset(d->data(), off);
  ref->iov_len = (ddsrt_iov_len_t)sz;
  return ddsi_serdata_ref(d);
}

template <typename T>
void serdata_to_ser_unref(struct ddsi_serdata* dcmn, const ddsrt_iovec_t* ref)
{
  static_cast<void>(ref);    // unused
  ddsi_serdata_unref(static_cast<ddscxx_serdata<T>*>(dcmn));
}

template <typename T>
bool serdata_to_sample(
  const struct ddsi_serdata* dcmn, void* sample, void** bufptr,
  void* buflim)
{
  (void)bufptr;
  (void)buflim;
  auto ptr = static_cast<const ddscxx_serdata<T>*>(dcmn);
  (static_cast<T*>(sample))->read_struct((char*)ptr->data() + 4, 0);

  return false;
}

template <typename T>
ddsi_serdata_t *serdata_to_topicless(const struct ddsi_serdata* dcmn)
{
  auto d = static_cast<const ddscxx_serdata<T>*>(dcmn);
  auto d1 = new ddscxx_serdata<T>(d->topic, SDK_KEY);
  d1->topic = nullptr;
  return d1;
}

template <typename T>
bool serdata_topicless_to_sample(
  const struct ddsi_sertopic* topic,
  const struct ddsi_serdata* dcmn, void* sample,
  void** bufptr, void* buflim)
{
  (void)topic;
  (void)dcmn;
  (void)sample;
  (void)bufptr;
  (void)buflim;
  assert(0);
  /* ROS 2 doesn't do keys in a meaningful way yet */
  return true;
}

template <typename T>
void serdata_free(struct ddsi_serdata* dcmn)
{
  auto* d = static_cast<const ddscxx_serdata<T>*>(dcmn);
  delete d;
}

#if DDSI_SERDATA_HAS_PRINT
template <typename T>
size_t serdata_print(
  const struct ddsi_sertopic* tpcmn, const struct ddsi_serdata* dcmn, char* buf, size_t bufsize)
{
  (void)tpcmn;
  (void)dcmn;
  (void)buf;
  (void)bufsize;
  assert(0);
  return 0;
}
#endif

#if DDSI_SERDATA_HAS_GET_KEYHASH
template <typename T>
void serdata_get_keyhash(
  const struct ddsi_serdata* d, struct ddsi_keyhash* buf,
  bool force_md5)
{
  auto ptr = static_cast<const ddscxx_serdata<T>*>(d);
  assert(buf);
  if (force_md5 && !ptr->key_md5_hashed())
  {
    ddsrt_md5_state_t md5st;
    ddsrt_md5_init(&md5st);
    ddsrt_md5_append(&md5st, (ddsrt_md5_byte_t*)(ptr->key().value), 16);
    ddsrt_md5_finish(&md5st, (ddsrt_md5_byte_t*)(buf->value));
  }
  else
  {
    memcpy(buf->value, ptr->key().value, 16);
  }
}
#endif

template <typename T>
const ddsi_serdata_ops ddscxx_serdata<T>::ddscxx_serdata_ops = {
  &serdata_eqkey<T>,
  &serdata_size<T>,
  &serdata_from_ser<T>,
#if DDSI_SERDATA_HAS_FROM_SER_IOV
  &serdata_from_ser_iov<T>,
#endif
  &serdata_from_keyhash<T>,
  &serdata_from_sample<T>,
  &serdata_to_ser<T>,
  &serdata_to_ser_ref<T>,
  &serdata_to_ser_unref<T>,
  &serdata_to_sample<T>,
  &serdata_to_topicless<T>,
  &serdata_topicless_to_sample<T>,
  &serdata_free<T>
#if DDSI_SERDATA_HAS_PRINT
  , &serdata_print<T>
#endif
#if DDSI_SERDATA_HAS_GET_KEYHASH
  , &serdata_get_keyhash<T>
#endif
};

template <typename T>
ddscxx_serdata<T>::ddscxx_serdata(const ddsi_sertopic* topic, ddsi_serdata_kind kind)
  : ddsi_serdata{}
{
  ddsi_serdata_init(this, topic, kind);
}

template <typename T>
ddscxx_sertopic<T>::ddscxx_sertopic(
  const char* topic_name, const char* type_name) : ddsi_sertopic{}
{
  ddsi_sertopic_init(
    static_cast<struct ddsi_sertopic*>(this),
    topic_name,
    type_name,
    &ddscxx_sertopic<T>::ddscxx_sertopic_ops,
    &ddscxx_serdata<T>::ddscxx_serdata_ops,
    true);
}

template <typename T>
void sertopic_free(struct ddsi_sertopic* tpcmn)
{
  auto tp = static_cast<ddscxx_sertopic<T>*>(tpcmn);
#if DDSI_SERTOPIC_HAS_TOPICKIND_NO_KEY
  ddsi_sertopic_fini(tpcmn);
#endif

  delete tp;
}

template <typename T>
void sertopic_zero_samples(const struct ddsi_sertopic* d, void* samples, size_t count)
{
  (void)d;
  (void)samples;
  (void)count;
}

template <typename T>
void sertopic_realloc_samples(
  void** ptrs, const struct ddsi_sertopic* d, void* old,
  size_t oldcount, size_t count)
{
  (void)d;
  (void)oldcount;
  auto msgs = static_cast<std::vector<T>*>(old);
  msgs->resize(count);
  size_t i = 0;
  for (auto& msg : *msgs)
    ptrs[i++] = &msg;
}

template <typename T>
void sertopic_free_samples(
  const struct ddsi_sertopic* d, void** ptrs, size_t count,
  dds_free_op_t op)
{
  (void)d;
  (void)ptrs;
  (void)count;
  (void)op;
  assert(0);
}

#if DDSI_SERTOPIC_HAS_EQUAL_AND_HASH
template <typename T>
bool sertopic_equal(
  const struct ddsi_sertopic* acmn, const struct ddsi_sertopic* bcmn)
{
  /* A bit of a guess: topics with the same name & type name are really the same if they have
   the same type support identifier as well */
  (void)acmn;
  (void)bcmn;
  return true;
}

template <typename T>
uint32_t sertopic_hash(const struct ddsi_sertopic* tpcmn)
{
  (void)tpcmn;
  return 0x0;
}

template <typename T>
const ddsi_sertopic_ops ddscxx_sertopic<T>::ddscxx_sertopic_ops = {
  &sertopic_free<T>,
  &sertopic_zero_samples<T>,
  &sertopic_realloc_samples<T>,
  &sertopic_free_samples<T>
#if DDSI_SERTOPIC_HAS_EQUAL_AND_HASH
  , &sertopic_equal<T>,
  &sertopic_hash<T>
#endif
};

#endif

#endif  // DDSCXXDATATOPIC_HPP_
