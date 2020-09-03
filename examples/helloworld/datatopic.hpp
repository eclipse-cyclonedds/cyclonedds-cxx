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
#ifndef IDLDATA_HPP_
#define IDLDATA_HPP_

#include <memory>
#include <string>

//include statements for generated streamers here?

#include "dds/ddsi/ddsi_serdata.h"
#include "dds/ddsi/ddsi_sertopic.h"
#include "HelloWorldData.h"
#include "bytewise.hpp"

/*
template <typename T> size_t write_struct(const T&, void* ptr, size_t offset);
template <typename T> size_t write_size(const T&, size_t offset);
template <typename T> size_t read_struct(T&, void* ptr, size_t offset);
*/

class helloworld_sertopic : public ddsi_sertopic {
public:
#if !DDSI_SERTOPIC_HAS_TOPICKIND_NO_KEY
  std::string cpp_name;
  std::string cpp_type_name;
  std::string cpp_name_type_name;
#endif

  static const struct ddsi_sertopic_ops helloworld_sertopic_ops;

  static helloworld_sertopic* create_sertopic(
    const std::string &topic_name, const char *tn);
  static void helloworld_sertopic_free(struct ddsi_sertopic* tpcmn);
  static void helloworld_sertopic_zero_samples(const struct ddsi_sertopic* d, void* samples, size_t count);
  static void helloworld_sertopic_realloc_samples(
    void** ptrs, const struct ddsi_sertopic* d, void* old,
    size_t oldcount, size_t count);
  static void helloworld_sertopic_free_samples(
    const struct ddsi_sertopic* d, void** ptrs, size_t count,
    dds_free_op_t op);

#if DDSI_SERTOPIC_HAS_EQUAL_AND_HASH
  static bool helloworld_sertopic_equal(
    const struct ddsi_sertopic* acmn, const struct ddsi_sertopic* bcmn);
  static uint32_t helloworld_sertopic_hash(const struct ddsi_sertopic* tpcmn);
#endif
};

class helloworld_serdata : public ddsi_serdata
{
  size_t m_size {0};
  std::unique_ptr<byte[]> m_data {nullptr};
  //HelloWorldData::Msg m_data;
  //size_t m_offset;

//  mutable std::vector<char>* const m_raw;

public:
  static const struct ddsi_serdata_ops helloworld_serdata_ops;
  helloworld_serdata(const ddsi_sertopic* topic, ddsi_serdata_kind kind);

  void resize(size_t requested_size);
  size_t size() const {return m_size;}
  void * data() const {return m_data.get();}

  //const HelloWorldData::Msg& data() const { return m_data; }
  //HelloWorldData::Msg& data() { return m_data; }
  //const size_t& offset() const { return m_offset; }
  //size_t& offset() { return m_offset; }
  //std::vector<char>& raw() const;

  static bool helloworld_serdata_eqkey(const struct ddsi_serdata* a, const struct ddsi_serdata* b);
  static uint32_t helloworld_serdata_size(const struct ddsi_serdata* dcmn);
  static struct ddsi_serdata* helloworld_serdata_from_ser(
    const struct ddsi_sertopic* topic,
    enum ddsi_serdata_kind kind,
    const struct nn_rdata* fragchain, size_t size);
#if DDSI_SERDATA_HAS_FROM_SER_IOV
  static struct ddsi_serdata* helloworld_serdata_from_ser_iov(
    const struct ddsi_sertopic* topic,
    enum ddsi_serdata_kind kind,
    ddsrt_msg_iovlen_t niov, const ddsrt_iovec_t* iov,
    size_t size);
#endif
  static struct ddsi_serdata* helloworld_serdata_from_keyhash(
    const struct ddsi_sertopic* topic,
    const struct ddsi_keyhash* keyhash);
  static struct ddsi_serdata* helloworld_serdata_from_sample(
    const struct ddsi_sertopic* topiccmn,
    enum ddsi_serdata_kind kind,
    const void* sample);
  static void helloworld_serdata_to_ser(const struct ddsi_serdata* dcmn, size_t off, size_t sz, void* buf);
  static struct ddsi_serdata* helloworld_serdata_to_ser_ref(
    const struct ddsi_serdata* dcmn, size_t off,
    size_t sz, ddsrt_iovec_t* ref);
  static void helloworld_serdata_to_ser_unref(struct ddsi_serdata* dcmn, const ddsrt_iovec_t* ref);
  static bool helloworld_serdata_to_sample(
    const struct ddsi_serdata* dcmn, void* sample, void** bufptr,
    void* buflim);
  static struct ddsi_serdata* helloworld_serdata_to_topicless(const struct ddsi_serdata* dcmn);
  static bool helloworld_serdata_topicless_to_sample(
    const struct ddsi_sertopic* topic,
    const struct ddsi_serdata* dcmn, void* sample,
    void** bufptr, void* buflim);
  static void helloworld_serdata_free(struct ddsi_serdata* dcmn);
#if DDSI_SERDATA_HAS_PRINT
  static size_t helloworld_serdata_print(
    const struct ddsi_sertopic* tpcmn, const struct ddsi_serdata* dcmn, char* buf, size_t bufsize);
#endif
#if DDSI_SERDATA_HAS_GET_KEYHASH
  static void helloworld_serdata_get_keyhash(
    const struct ddsi_serdata* d, struct ddsi_keyhash* buf,
    bool force_md5);
#endif
};

#endif  // IDLDATA_HPP_
