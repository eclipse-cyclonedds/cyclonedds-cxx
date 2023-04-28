// Copyright(c) 2006 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "dds/dds.hpp"
#include <gtest/gtest.h>
#include "CdrDataModels.hpp"
#include "CdrDataModels_pragma.hpp"

#if DDSCXX_USE_BOOST
#include <boost/optional.hpp>
#include <boost/none.hpp>
#define DDSCXX_STD_IMPL boost
#define DDSCXX_STD_IMPL_NULLOPT boost::none
#else
#define DDSCXX_STD_IMPL std
#define DDSCXX_STD_IMPL_NULLOPT std::nullopt
#endif

using namespace org::eclipse::cyclonedds::core::cdr;
using namespace CDR_testing;

typedef std::vector<unsigned char> bytes;

/**
 * Fixture for the DataWriter tests
 */
class CDRStreamer : public ::testing::Test
{
public:

    bytes BS_basic_normal {
        0x00, 0x01, 0xE2, 0x40 /*basicstruct.l*/,
        'g' /*basicstruct.c*/,
        0x00, 0x00, 0x00 /*padding bytes (3)*/,
        0x00, 0x00, 0x00, 0x07 /*basicstruct.str.length*/, 'a', 'b', 'c', 'd', 'e', 'f', '\0' /*basicstruct.str.c_str*/,
        0x00, 0x00, 0x00, 0x00, 0x00 /*padding bytes (5)*/,
        0x40, 0x84, 0x72, 0x91, 0x68, 0x72, 0xB0, 0x21 /*basicstruct.d*/};
    bytes BS_basic_key {'g'/*basicstruct.c*/};

    /*xcdrv2 is max aligned to 4 bytes*/
    bytes BS_xcdrv2_normal {
        0x00, 0x01, 0xE2, 0x40 /*basicstruct.l*/,
        'g' /*basicstruct.c*/,
        0x00, 0x00, 0x00 /*padding bytes (3)*/,
        0x00, 0x00, 0x00, 0x07 /*basicstruct.str.length*/, 'a', 'b', 'c', 'd', 'e', 'f', '\0' /*basicstruct.str.c_str*/,
        0x00 /*padding bytes (1)*/,
        0x40, 0x84, 0x72, 0x91, 0x68, 0x72, 0xB0, 0x21 /*basicstruct.d*/};
    bytes AS_xcdr_v2_normal {
        0x00, 0x00, 0x00, 0x1C/*dheader*/,
        0x00, 0x01, 0xE2, 0x40 /*appendablestruct.l*/,
        'g' /*appendablestruct.c*/,
        0x00, 0x00, 0x00 /*padding bytes (3)*/,
        0x00, 0x00, 0x00, 0x07 /*appendablestruct.str.length*/, 'a', 'b', 'c', 'd', 'e', 'f', '\0' /*appendablestruct.str.c_str*/,
        0x00 /*padding bytes (1)*/,
        0x40, 0x84, 0x72, 0x91, 0x68, 0x72, 0xB0, 0x21 /*appendablestruct.d*/};

    CDRStreamer()
    {
    }

    void SetUp() { }

    void TearDown() { }

};

template<typename T, typename S>
void VerifyWrite_Impl(const T& in, const bytes &out, S &stream, bool as_key, bool write_success = true, bool compare_success = true)
{
  bytes buffer;
  bool result = move(stream, in, as_key);
  ASSERT_EQ(result, write_success);

  if (!result)
    return;

  buffer.resize(stream.position());
  stream.set_buffer(buffer.data(), buffer.size());
  ASSERT_TRUE(write(stream, in, as_key));

  result = (buffer == out);
  ASSERT_EQ(result, compare_success);
}

template<typename T, typename S>
void VerifyRead_Impl(const bytes &in, const T& out, S &stream, bool as_key, bool read_success = true, bool compare_success = true)
{
  bytes incopy(in);
  T buffer;
  stream.set_buffer(incopy.data(), incopy.size());
  bool result = read(stream, buffer, as_key);
  ASSERT_EQ(result, read_success);

  if (!result)
    return;

  if (as_key)
    result = (buffer.c() == out.c());
  else
    result = (buffer == out);

  ASSERT_EQ(result, compare_success);
}

template<typename T, typename S>
void VerifyReadOneDeeper_Impl(const bytes &in, const T& out, S &stream, bool as_key)
{
  bytes incopy(in);
  T buffer;

  stream.set_buffer(incopy.data(), incopy.size());
  ASSERT_TRUE(read(stream, buffer, as_key));

  if (as_key) {
    ASSERT_EQ(buffer.c().size(), out.c().size());
    for (size_t i = 0; i < buffer.c().size() && i < out.c().size(); i++)
      ASSERT_EQ(buffer.c()[i].c(), out.c()[i].c());
  } else {
    ASSERT_EQ(buffer, out);
  }
}

#define VerifyRead(_bytes, _struct, _streamer, _as_key, _read_success, _compare_success)\
{\
_streamer streamer_1(endianness::big_endian);\
VerifyRead_Impl(_bytes, _struct, streamer_1, _as_key, _read_success, _compare_success);\
}

#define VerifyReadOneDeeper(_bytes, _struct, _streamer, _as_key)\
{\
_streamer streamer_1(endianness::big_endian);\
VerifyReadOneDeeper_Impl(_bytes, _struct, streamer_1, _as_key);\
}

#define read_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
{\
VerifyRead(normal_bytes, test_struct, streamer, false, true, true);\
VerifyRead(key_bytes, key_struct, streamer, true, true, true);\
}

#define read_test_fail(test_struct, key_struct, key_bytes, streamer)\
{\
VerifyRead(bytes(256, 0x0), test_struct, streamer, false, false, true);\
VerifyRead(key_bytes, key_struct, streamer, true, true, true);\
}

#define read_deeper_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
{\
VerifyRead(normal_bytes, test_struct, streamer, false, true, true);\
VerifyReadOneDeeper(key_bytes, key_struct, streamer, true);\
}

#define VerifyWrite(_bytes, _struct, _streamer, _as_key, _write_success, _compare_success)\
{\
_streamer streamer_1(endianness::big_endian);\
VerifyWrite_Impl(_bytes, _struct, streamer_1, _as_key, _write_success, _compare_success);\
}

#define write_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
{\
VerifyWrite(test_struct, normal_bytes, streamer, false, true, true);\
VerifyWrite(key_struct, key_bytes, streamer, true, true, true);\
}

#define write_test_fail(test_struct, key_struct, key_bytes, streamer)\
{\
VerifyWrite(test_struct, bytes(256, 0x0), streamer, false, false, false);\
VerifyWrite(test_struct, key_bytes, streamer, true, true, true);\
}

#define readwrite_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
read_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
write_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)

#define readwrite_test_fail(test_struct, key_struct, key_bytes, streamer)\
read_test_fail(test_struct, key_struct, key_bytes, streamer)\
write_test_fail(test_struct, key_struct, key_bytes, streamer)

#define readwrite_deeper_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
read_deeper_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
write_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)

#define stream_test(test_struct, cdr_normal_bytes, key_bytes)\
readwrite_test(test_struct, test_struct, cdr_normal_bytes, key_bytes, basic_cdr_stream)\
readwrite_test(test_struct, test_struct, cdr_normal_bytes, key_bytes, xcdr_v1_stream)\
readwrite_test(test_struct, test_struct, cdr_normal_bytes, key_bytes, xcdr_v2_stream)

#define stream_test_union(test_struct, key_struct, cdr_normal_bytes, key_bytes)\
readwrite_test(test_struct, key_struct, cdr_normal_bytes, key_bytes, basic_cdr_stream)\
readwrite_test(test_struct, key_struct, cdr_normal_bytes, key_bytes, xcdr_v1_stream)\
readwrite_test(test_struct, key_struct, cdr_normal_bytes, key_bytes, xcdr_v2_stream)

#define stream_test_fail_basic(test_struct, xcdr_v1_normal_bytes, xcdr_v2_normal_bytes, key_bytes)\
readwrite_test_fail(test_struct, test_struct, key_bytes, basic_cdr_stream)\
readwrite_test(test_struct, test_struct, xcdr_v1_normal_bytes, key_bytes, xcdr_v1_stream)\
readwrite_test(test_struct, test_struct, xcdr_v2_normal_bytes, key_bytes, xcdr_v2_stream)

#define stream_deeper_test(test_struct, cdr_normal_bytes, cdr_delimited_bytes, key_bytes)\
readwrite_deeper_test(test_struct, test_struct, cdr_normal_bytes, key_bytes, basic_cdr_stream)\
readwrite_deeper_test(test_struct, test_struct, cdr_normal_bytes, key_bytes, xcdr_v1_stream)\
readwrite_deeper_test(test_struct, test_struct, cdr_delimited_bytes, key_bytes, xcdr_v2_stream)

/*verifying streamer will not read/write beyond the end of the indicated buffer*/

TEST_F(CDRStreamer, cdr_boundary)
{
  basicstruct BS(123456, 'g', "abcdef", 654.321);
  /*this struct should be 4 + 1 + 3 + 4 + 7 + 5 + 8 = 32 bytes long, in basic cdr serialization*/
  basicstruct BS2;

  std::vector<char> buffer(32,0x0);

  basic_cdr_stream str;
  str.set_buffer(buffer.data(), 12);

  ASSERT_FALSE(write(str, BS, false)); /*this write should fail, as the buffer limit is too small*/
  ASSERT_EQ(str.status(), serialization_status::write_bound_exceeded);

  str.reset();

  ASSERT_FALSE(read(str, BS2, false)); /*this read should fail too, as the buffer limit is too small*/
  ASSERT_EQ(str.status(), serialization_status::read_bound_exceeded);

  str.set_buffer(buffer.data(), 32);

  ASSERT_TRUE(write(str, BS, false)); /*this write should finish, as the buffer limit is set as "unlimited"*/

  str.reset();

  ASSERT_TRUE(read(str, BS2, false)); /*this write should finish, as the buffer limit is set as "unlimited"*/
  ASSERT_EQ(BS, BS2);
}

/*verifying reads/writes of a basic struct*/

TEST_F(CDRStreamer, cdr_basic)
{
  basicstruct BS(123456, 'g', "abcdef", 654.321);

  readwrite_test(BS, BS, BS_basic_normal, BS_basic_key, basic_cdr_stream);
  readwrite_test(BS, BS, BS_basic_normal, BS_basic_key, xcdr_v1_stream);
  readwrite_test(BS, BS, BS_xcdrv2_normal, BS_basic_key, xcdr_v2_stream);
}

/*verifying reads/writes of an appendable struct*/

TEST_F(CDRStreamer, cdr_appendable)
{
  appendablestruct AS(123456, 'g', "abcdef", 654.321);

  stream_test_fail_basic(AS, BS_basic_normal, AS_xcdr_v2_normal, BS_basic_key)
}

/*verifying reads/writes of a mutable struct*/

TEST_F(CDRStreamer, cdr_mutable)
{
  mutablestruct MS(123456, 'g', "abcdef", 654.321);

  bytes MS_xcdr_v1_normal {
      0x00, 0x07, 0x00, 0x04 /*mutablestruct.l.mheader*/,
      0x00, 0x01, 0xE2, 0x40 /*mutablestruct.l*/,
      0x40, 0x05, 0x00, 0x01 /*mutablestruct.c.mheader*/,
      'g' /*mutablestruct.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x7F, 0x01, 0x00, 0x08 /*mutablestruct.str.mheader (pid_list_extended + length = 8)*/,
      0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0B /*mutablestruct.str.mheader (extended)*/,
      0x00, 0x00, 0x00, 0x07 /*mutablestruct.str.length*/, 'a', 'b', 'c', 'd', 'e', 'f', '\0' /*mutablestruct.str.c_str*/,
      0x00 /*padding bytes (1)*/,
      0x00, 0x01, 0x00, 0x0C /*mutablestruct.d.mheader*/,
      0x00, 0x00, 0x00, 0x00 /*padding bytes (4)*/,
      0x40, 0x84, 0x72, 0x91, 0x68, 0x72, 0xB0, 0x21 /*mutablestruct.d*/,
      0x7F, 0x02, 0x00, 0x00 /*mutablestruct list termination header*/
      };
  bytes MS_xcdr_v1_normal_reordered {
      0x00, 0x01, 0x00, 0x0C /*mutablestruct.d.mheader*/,
      0x00, 0x00, 0x00, 0x00 /*padding bytes (4)*/,
      0x40, 0x84, 0x72, 0x91, 0x68, 0x72, 0xB0, 0x21 /*mutablestruct.d*/,
      0x7F, 0x01, 0x00, 0x08 /*mutablestruct.str.mheader (pid_list_extended + must_understand + length = 8)*/,
      0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0B /*mutablestruct.str.mheader (extended)*/,
      0x00, 0x00, 0x00, 0x07 /*mutablestruct.str.length*/, 'a', 'b', 'c', 'd', 'e', 'f', '\0' /*mutablestruct.str.c_str*/,
      0x00 /*padding bytes (1)*/,
      0x40, 0x05, 0x00, 0x01 /*mutablestruct.c.mheader*/,
      'g' /*mutablestruct.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x07, 0x00, 0x04 /*mutablestruct.l.mheader*/,
      0x00, 0x01, 0xE2, 0x40 /*mutablestruct.l*/,
      0x7F, 0x02, 0x00, 0x00 /*mutablestruct list termination header*/
      };
  bytes MS_xcdr_v2_normal {
      0x00, 0x00, 0x00, 0x3C /*dheader*/,
      0x40, 0x00, 0x00, 0x07 /*mutablestruct.l.emheader*/,
      0x00, 0x00, 0x00, 0x04 /*mutablestruct.l.emheader.nextint*/,
      0x00, 0x01, 0xE2, 0x40 /*mutablestruct.l*/,
      0xC0, 0x00, 0x00, 0x05 /*mutablestruct.c.emheader*/,
      0x00, 0x00, 0x00, 0x01 /*mutablestruct.l.emheader.nextint*/,
      'g' /*mutablestruct.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x40, 0x00, 0x00, 0x03 /*mutablestruct.str.emheader*/,
      0x00, 0x00, 0x00, 0x0B /*mutablestruct.str.emheader.nextint*/,
      0x00, 0x00, 0x00, 0x07 /*mutablestruct.str.length*/, 'a', 'b', 'c', 'd', 'e', 'f', '\0' /*mutablestruct.str.c_str*/,
      0x00 /*padding bytes (1)*/,
      0x40, 0x00, 0x00, 0x01 /*mutablestruct.d.emheader*/,
      0x00, 0x00, 0x00, 0x08 /*mutablestruct.d.emheader.nextint*/,
      0x40, 0x84, 0x72, 0x91, 0x68, 0x72, 0xB0, 0x21 /*mutablestruct.d*/};
  bytes MS_xcdr_v2_normal_reordered {
      0x00, 0x00, 0x00, 0x30 /*dheader*/,
      0x30, 0x00, 0x00, 0x01 /*mutablestruct.d.emheader*/,
      0x40, 0x84, 0x72, 0x91, 0x68, 0x72, 0xB0, 0x21 /*mutablestruct.d*/,
      0x40, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x0B /*mutablestruct.str.emheader*/,
      0x00, 0x00, 0x00, 0x07 /*mutablestruct.str.length*/, 'a', 'b', 'c', 'd', 'e', 'f', '\0' /*mutablestruct.str.c_str*/,
      0x00 /*padding bytes (1)*/,
      0x80, 0x00, 0x00, 0x05 /*mutablestruct.c.emheader*/,
      'g' /*mutablestruct.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x20, 0x00, 0x00, 0x07 /*mutablestruct.l.emheader*/,
      0x00, 0x01, 0xE2, 0x40 /*mutablestruct.l*/};

  stream_test_fail_basic(MS, MS_xcdr_v1_normal, MS_xcdr_v2_normal, BS_basic_key)
  VerifyRead(MS_xcdr_v1_normal_reordered, MS, xcdr_v1_stream, false, true, true);
  VerifyRead(MS_xcdr_v2_normal_reordered, MS, xcdr_v2_stream, false, true, true);
}

/*verifying reads/writes of a nested struct*/

TEST_F(CDRStreamer, cdr_nested)
{
  outer NS(inner('a',123), inner('b', 456), inner('c', 789));

  bytes NS_basic_normal {
      'a' /*outer.a.c_inner*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x7B /*outer.a.l_inner*/,
      'b' /*outer.b.c_inner*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x01, 0xC8 /*outer.b.l_inner*/,
      'c' /*outer.c.c_inner*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x03, 0x15 /*outer.c.l_inner*/};
  bytes NS_basic_key {
      'c' /*outer.c.c_inner*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x03, 0x15 /*outer.c.l_inner*/};

  stream_test(NS, NS_basic_normal, NS_basic_key)
}

/*verifying reads/writes of a struct containing inheritance*/

TEST_F(CDRStreamer, cdr_inherited)
{
  derived DS("gfedcb", 'a');
  DS.str("hjklmn");
  DS.c('o');

  bytes DS_basic_normal {
      0x00, 0x00, 0x00, 0x07 /*derived::base.str.length*/, 'h', 'j', 'k', 'l', 'm', 'n', '\0' /*derived::base.str.c_str*/,
      'o'/*derived::base.c*/,
      0x00, 0x00, 0x00, 0x07 /*derived.str_d.length*/, 'g', 'f', 'e', 'd', 'c', 'b', '\0'/*derived.str_d.c_str*/,
      'a'/*derived.c_d*/
      };
  bytes DS_basic_key {
      'o'/*derived::base.c*/
      };

  stream_test(DS, DS_basic_normal, DS_basic_key)
}

/*verifying reads/writes of a struct containing sequences*/

TEST_F(CDRStreamer, cdr_sequence)
{
  sequence_struct SS({'z','y','x'}, {4,3,2,1});
  sequence_struct_mutable SSM({'z','y','x'}, {4,3,2,1});

  bytes SS_basic_normal {
      0x00, 0x00, 0x00, 0x03/*sequence_struct.c.length*/, 'z', 'y', 'x'/*sequence_struct.c.data*/,
      0x00 /*padding bytes (1)*/,
      0x00, 0x00, 0x00, 0x04/*sequence_struct.l.length*/, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01/*sequence_struct.l.data*/
      };
  bytes SS_basic_key {
      0x00, 0x00, 0x00, 0x03/*sequence_struct.c.length*/, 'z', 'y', 'x'/*sequence_struct.c.data*/
      };

  bytes SSM_xcdr_v1_normal {
      0x7F, 0x01, 0x00, 0x08 /*sequence_struct.c.mheader (pid_list_extended + length = 8)*/,
      0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x07 /*sequence_struct.c.mheader (extended)*/,
      0x00, 0x00, 0x00, 0x03/*sequence_struct.c.length*/, 'z', 'y', 'x'/*sequence_struct.c.data*/,
      0x00 /*padding bytes (1)*/,
      0x7F, 0x01, 0x00, 0x08 /*sequence_struct.l.mheader (pid_list_extended + length = 8)*/,
      0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x14 /*sequence_struct.l.mheader (extended)*/,
      0x00, 0x00, 0x00, 0x04/*sequence_struct.l.length*/, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01/*sequence_struct.l.data*/,
      0x7F, 0x02, 0x00, 0x00 /*inner list termination header*/
      };
  bytes SSM_xcdr_v2_normal {
      0x00, 0x00, 0x00, 0x2C/*sequence_struct.dheader*/,
      0xC0, 0x00, 0x00, 0x00 /*sequence_struct.c.emheader.lc = 4 (nextint) + must_understand + id(0)*/,
      0x00, 0x00, 0x00, 0x07 /*sequence_struct.c.emheader.nextint */,
      0x00, 0x00, 0x00, 0x03/*sequence_struct.c.length*/, 'z', 'y', 'x'/*sequence_struct.c.data*/,
      0x00 /*padding bytes (1)*/,
      0x40, 0x00, 0x00, 0x01 /*sequence_struct.c.emheader.lc = 4 (nextint) + id(1)*/,
      0x00, 0x00, 0x00, 0x14 /*sequence_struct.c.emheader.nextint */,
      0x00, 0x00, 0x00, 0x04/*sequence_struct.l.length*/, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01/*sequence_struct.l.data*/
      };
  /*different length code, overlapping nextint with the length of the sequence
    our streamer implementation does not write this way, but it must be able to
    read it*/
  bytes SSM_xcdr_v2_normal_lc_not_4 {
      0x00, 0x00, 0x00, 0x24/*sequence_struct.dheader*/,
      0xD0, 0x00, 0x00, 0x00 /*derived.c_d.emheader*/, /*lc = 5: length = sequence_struct.c.length*1 + must_understand + id(0)*/
      0x00, 0x00, 0x00, 0x03/*sequence_struct.c.length*/, 'z', 'y', 'x'/*sequence_struct.c.data*/,
      0x00 /*padding bytes (1)*/,
      0x60, 0x00, 0x00, 0x01 /*derived.l.emheader*/, /*lc = 6: length = sequence_struct.c.length*4 + id(1)*/
      0x00, 0x00, 0x00, 0x04/*sequence_struct.l.length*/, 0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x01/*sequence_struct.l.data*/
      };

  stream_test(SS, SS_basic_normal, SS_basic_key)

  readwrite_test(SSM, SSM, SSM_xcdr_v1_normal, SS_basic_key,  xcdr_v1_stream)
  readwrite_test(SSM, SSM, SSM_xcdr_v2_normal, SS_basic_key,  xcdr_v2_stream)
  read_test(SSM, SSM, SSM_xcdr_v2_normal_lc_not_4, SS_basic_key, xcdr_v2_stream)
}

/*verifying reads/writes of a struct containing arrays*/

TEST_F(CDRStreamer, cdr_array)
{
  array_struct ARS({'e','d','c','b','a'},{123,234,345,456,567});

  bytes ARS_normal {
      'e', 'd', 'c', 'b', 'a'/*array_struct.c*/,
      0x00, 0x00, 0x00 /*padding bytes*/,
      0x00, 0x00, 0x00, 0x7B,
      0x00, 0x00, 0x00, 0xEA,
      0x00, 0x00, 0x01, 0x59,
      0x00, 0x00, 0x01, 0xC8,
      0x00, 0x00, 0x02, 0x37 /*array_struct.l*/,
      };
  bytes ARS_key {
      'e', 'd', 'c', 'b', 'a'/*array_struct.c*/
      };

  stream_test(ARS, ARS_normal, ARS_key)
}

/*verifying reads/writes of a struct containing typedefs*/

TEST_F(CDRStreamer, cdr_typedef)
{
  typedef_base_struct TBS({'a','b','c','d'},{'e','f','g'});

  bytes TBS_normal {
      0x00, 0x00, 0x00, 0x04/*typedef_struct.c.length*/,
      'a', 'b', 'c', 'd'/*typedef_struct.c.data*/,
      0x00, 0x00, 0x00, 0x03/*typedef_struct.l.length*/,
      'e', 'f', 'g'/*typedef_struct.l.data*/,
      };
  bytes TBS_key {
      0x00, 0x00, 0x00, 0x04/*typedef_struct.c.length*/,
      'a', 'b', 'c', 'd'/*typedef_struct.c.data*/
      };

  stream_test(TBS, TBS_normal, TBS_key)

  typedef_constr_struct TCS({base("qwe",'a'),base("wer",'b'),base("ert",'c'),base("rty",'d')},{base("tyu",'e'),base("yui",'f'),base("uio",'g')});

  bytes TCS_normal {
      0x00, 0x00, 0x00, 0x04/*typedef_struct.c.length*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'q', 'w', 'e', '\0' /*base.str.c_str*/,
      'a'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'w', 'e', 'r', '\0' /*base.str.c_str*/,
      'b'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'e', 'r', 't', '\0' /*base.str.c_str*/,
      'c'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'r', 't', 'y', '\0' /*base.str.c_str*/,
      'd'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x03/*typedef_struct.l.length*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 't', 'y', 'u', '\0' /*base.str.c_str*/,
      'e'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'y', 'u', 'i', '\0' /*base.str.c_str*/,
      'f'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'u', 'i', 'o', '\0' /*base.str.c_str*/,
      'g'/*base.c*/
      };
  bytes TCS_normal_delimited {
      0x00, 0x00, 0x00, 0x31/*typedef_struct.c.dheader*/,
      0x00, 0x00, 0x00, 0x04/*typedef_struct.c.length*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'q', 'w', 'e', '\0' /*base.str.c_str*/,
      'a'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'w', 'e', 'r', '\0' /*base.str.c_str*/,
      'b'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'e', 'r', 't', '\0' /*base.str.c_str*/,
      'c'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'r', 't', 'y', '\0' /*base.str.c_str*/,
      'd'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x25/*typedef_struct.l.dheader*/,
      0x00, 0x00, 0x00, 0x03/*typedef_struct.l.length*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 't', 'y', 'u', '\0' /*base.str.c_str*/,
      'e'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'y', 'u', 'i', '\0' /*base.str.c_str*/,
      'f'/*base.c*/,
      0x00, 0x00, 0x00 /*padding bytes (3)*/,
      0x00, 0x00, 0x00, 0x04/*base.str.length*/, 'u', 'i', 'o', '\0' /*base.str.c_str*/,
      'g'/*base.c*/
      };
  bytes TCS_key {
      0x00, 0x00, 0x00, 0x04/*typedef_struct.c.length*/,
      'a'/*base.c*/,
      'b'/*base.c*/,
      'c'/*base.c*/,
      'd'/*base.c*/
      };

  stream_deeper_test(TCS, TCS_normal, TCS_normal_delimited, TCS_key)
}

/*verifying reads/writes of a struct containing unions*/

TEST_F(CDRStreamer, cdr_union)
{
  un _c;
  _c.c('b','a');
  union_struct US(_c);

  un_k _c_k;
  _c_k.c('b','a');
  union_struct_k US_k(_c_k);

  _c_k.c('\0','a');
  union_struct_k US_k_read(_c_k);


  bytes US_normal {
      'a'/*union_struct.c.switch*/,
      'b'/*union_struct.c.c*/
      };
  bytes US_k_key {
      'a'/*union_struct.c.switch*/
      };

  stream_test(US, US_normal, US_normal)

  VerifyRead(US_normal, US_k, basic_cdr_stream, false, true, true);
  VerifyRead(US_normal, US_k, xcdr_v1_stream, false, true, true);
  VerifyRead(US_normal, US_k, xcdr_v2_stream, false, true, true);

  VerifyRead(US_k_key, US_k_read, basic_cdr_stream, true, true, true);
  VerifyRead(US_k_key, US_k_read, xcdr_v1_stream, true, true, true);
  VerifyRead(US_k_key, US_k_read, xcdr_v2_stream, true, true, true);

  write_test(US_k, US_k, US_normal, US_k_key, basic_cdr_stream)
  write_test(US_k, US_k, US_normal, US_k_key, xcdr_v1_stream)
  write_test(US_k, US_k, US_normal, US_k_key, xcdr_v2_stream)
}

/*verifying reads/writes of structs using pragma keylist*/

TEST_F(CDRStreamer, cdr_pragma)
{
  pragma_keys PS(sub_2(sub_1(123,234),sub_1(345,456)),sub_2(sub_1(567,678),sub_1(789,890))),
              PS_key_test(sub_2(sub_1(0,234),sub_1(0,456)),sub_2(sub_1(0,678),sub_1(0,890)));

  bytes PS_basic_normal {
      0x00, 0x00, 0x00, 0x7B/*pragma_keys.c.s_1.l_1*/,
      0x00, 0x00, 0x00, 0xEA/*pragma_keys.c.s_1.l_2*/,
      0x00, 0x00, 0x01, 0x59/*pragma_keys.c.s_2.l_1*/,
      0x00, 0x00, 0x01, 0xC8/*pragma_keys.c.s_2.l_2*/,
      0x00, 0x00, 0x02, 0x37/*pragma_keys.d.s_1.l_1*/,
      0x00, 0x00, 0x02, 0xA6/*pragma_keys.d.s_1.l_2*/,
      0x00, 0x00, 0x03, 0x15/*pragma_keys.d.s_2.l_1*/,
      0x00, 0x00, 0x03, 0x7A/*pragma_keys.d.s_2.l_2*/
      };
  bytes PS_basic_key {
      0x00, 0x00, 0x00, 0xEA/*pragma_keys.c.s_1.l_2*/,
      0x00, 0x00, 0x01, 0xC8/*pragma_keys.c.s_2.l_2*/,
      0x00, 0x00, 0x02, 0xA6/*pragma_keys.d.s_1.l_2*/,
      0x00, 0x00, 0x03, 0x7A/*pragma_keys.d.s_2.l_2*/
      };

  VerifyRead(PS_basic_normal, PS, basic_cdr_stream, false, true, true);
  VerifyRead(PS_basic_normal, PS, xcdr_v1_stream, false, true, true);
  VerifyRead(PS_basic_normal, PS, xcdr_v2_stream, false, true, true);

  VerifyRead(PS_basic_key, PS_key_test, basic_cdr_stream, true, true, true);
  VerifyRead(PS_basic_key, PS_key_test, xcdr_v1_stream, true, true, true);
  VerifyRead(PS_basic_key, PS_key_test, xcdr_v2_stream, true, true, true);

  write_test(PS, PS, PS_basic_normal, PS_basic_key, basic_cdr_stream)
  write_test(PS, PS, PS_basic_normal, PS_basic_key, xcdr_v1_stream)
  write_test(PS, PS, PS_basic_normal, PS_basic_key, xcdr_v2_stream)
}

/*verifying reads/writes of a struct containing enums*/

TEST_F(CDRStreamer, cdr_enum)
{
  enum_struct ES(enum_8::second_8, enum_16::third_16, enum_32::fourth_32);

  /*basic cdr treats all enums as 32 bit integers*/
  bytes ES_basic_normal {
      0x00, 0x00, 0x00 ,0x01 /*enum_struct.c*/,
      0x00, 0x00, 0x00, 0x02 /*enum_struct.b*/,
      0x00, 0x00, 0x00, 0x03 /*enum_struct.a*/
      };
  bytes ES_basic_key {
      0x00, 0x00, 0x00, 0x01 /*enum_struct.c*/
      };
  /*xcdr_v1 and xcdr_v2 treat bitbounded enums in the same manner*/
  bytes ES_xcdr_v1_normal {
      0x01 /*enum_struct.c*/,
      0x00 /*padding bytes (1)*/,
      0x00, 0x02 /*enum_struct.b*/,
      0x00, 0x00, 0x00, 0x03 /*enum_struct.a*/
      };

  readwrite_test(ES, ES, ES_basic_normal, ES_basic_key, basic_cdr_stream);
  readwrite_test(ES, ES, ES_xcdr_v1_normal, ES_basic_key, xcdr_v1_stream);
  readwrite_test(ES, ES, ES_xcdr_v1_normal, ES_basic_key, xcdr_v2_stream);
}

/*verifying reads/writes of structs containing optional fields*/

TEST_F(CDRStreamer, cdr_optional)
{
  optional_final_struct OFS(DDSCXX_STD_IMPL_NULLOPT, 'b', 'c');
  optional_appendable_struct OAS(DDSCXX_STD_IMPL_NULLOPT, 'b', 'c');
  optional_mutable_struct OMS(DDSCXX_STD_IMPL_NULLOPT, 'b', 'c');

  /*no basic cdr, since it does not support optional fields*/
  bytes OFS_xcdr_v1_normal {
      0x00, 0x00, 0x00, 0x00 /*optional_final_struct.a.mheader*/,
      'b'/*optional_final_struct.a*/,
      'c'/*optional_final_struct.c*/
      };
  bytes OFS_key {
      'c'/*optional_final_struct.c*/
      };
  bytes OMS_xcdr_v1_normal {
      0x00, 0x01, 0x00, 0x01 /*optional_mutable_struct.b.mheader*/,
      'b'/*optional_mutable_struct.b*/,
      0x00, 0x00, 0x00/*padding bytes (3)*/,
      0x40, 0x02, 0x00, 0x01 /*optional_mutable_struct.c.mheader*/,
      'c'/*optional_final_struct.c*/,
      0x00, 0x00, 0x00/*padding bytes (3)*/,
      0x7F, 0x02, 0x00, 0x00 /*optional_mutable_struct list termination header*/
      };
  bytes OFS_xcdr_v2_normal {
      0x00/*optional_final_struct.a.is_present*/,
      'b'/*optional_final_struct.b*/,
      'c'/*optional_final_struct.c*/
      };
    bytes OAS_xcdr_v2_normal {
      0x00, 0x00, 0x00, 0x03/*dheader*/,
      0x00/*optional_appendable_struct.a.is_present*/,
      'b'/*optional_appendable_struct.b*/,
      'c'/*optional_appendable_struct.c*/
      };
    bytes OMS_xcdr_v2_normal {
      0x00, 0x00, 0x00, 0x15, /*dheader*/
      0x40, 0x00, 0x00, 0x01, /*optional_mutable_struct.b.emheader*/
      0x00, 0x00, 0x00, 0x01, /*optional_mutable_struct.b.emheader.nextint*/
      'b', /*optional_mutable_struct.b*/
      0x00, 0x00, 0x00, /*padding bytes*/
      0xC0, 0x00, 0x00, 0x02, /*optional_mutable_struct.b.emheader*/
      0x00, 0x00, 0x00, 0x01, /*optional_mutable_struct.b.emheader.nextint*/
      'c' /*optional_mutable_struct.c*/
      };

  /* basic cdr does not support optional fields,
     therefore the streamer should enter error status
     when the streamer is asked to write them */
  readwrite_test_fail(OFS, OFS, OFS_key, basic_cdr_stream);

  readwrite_test(OFS, OFS, OFS_xcdr_v1_normal, OFS_key, xcdr_v1_stream)
  readwrite_test(OAS, OAS, OFS_xcdr_v1_normal, OFS_key, xcdr_v1_stream)
  readwrite_test(OMS, OMS, OMS_xcdr_v1_normal, OFS_key, xcdr_v1_stream)

  readwrite_test(OFS, OFS, OFS_xcdr_v2_normal, OFS_key, xcdr_v2_stream)
  readwrite_test(OAS, OAS, OAS_xcdr_v2_normal, OFS_key, xcdr_v2_stream)
  readwrite_test(OMS, OMS, OMS_xcdr_v2_normal, OFS_key, xcdr_v2_stream)

  optional_array_struct ORS('a', DDSCXX_STD_IMPL::optional<std::array<char,5> >({'b','c','d', 'e', 'f'}));
  bytes ORS_v2{
          'a', 1, 'b', 'c', 'd', 'e', 'f'
          },
        ORS_key{
          'a'};

  readwrite_test(ORS, ORS, ORS_v2, ORS_key, xcdr_v2_stream);
}

/*verifying reads/writes of structs containing must_understand field*/

TEST_F(CDRStreamer, cdr_must_understand)
{
  must_understand_struct MU('a','b','c');

  /*only the basic cdr should reject this as it is a mutable struct */
  bytes key {
      'c'
      };
  bytes v1 {
      0x00, 0x01, 0x00, 0x01 /*must_understand_struct.a.mheader*/,
      'a'/*must_understand_struct.a*/,
      0x00, 0x00, 0x00/*padding bytes*/,
      0x40, 0x02, 0x00, 0x01 /*must_understand_struct.b.mheader*/,
      'b'/*must_understand_struct.b*/,
      0x00, 0x00, 0x00/*padding bytes*/,
      0x40, 0x03, 0x00, 0x01 /*must_understand_struct.c.mheader*/,
      'c'/*must_understand_struct.c*/,
      0x00, 0x00, 0x00/*padding bytes*/,
      0x7F, 0x02, 0x00, 0x00 /*optional_mutable_struct list termination header*/
      };
  bytes v2 {
      0x00, 0x00, 0x00, 0x21, /*dheader*/
      0x40, 0x00, 0x00, 0x01, /*must_understand_struct.a.emheader*/
      0x00, 0x00, 0x00, 0x01, /*must_understand_struct.a.emheader.nextint*/
      'a', /*must_understand_struct.a*/
      0x00, 0x00, 0x00, /*padding bytes*/
      0xC0, 0x00, 0x00, 0x02, /*must_understand_struct.b.emheader*/
      0x00, 0x00, 0x00, 0x01, /*must_understand_struct.b.emheader.nextint*/
      'b', /*must_understand_struct.b*/
      0x00, 0x00, 0x00, /*padding bytes*/
      0xC0, 0x00, 0x00, 0x03, /*must_understand_struct.c.emheader*/
      0x00, 0x00, 0x00, 0x01, /*must_understand_struct.c.emheader.nextint*/
      'c', /*must_understand_struct.c*/
      };
  stream_test_fail_basic(MU, v1, v2, key);

  /*these cdr streams for MU do not contain the field b so they must be rejected on read*/
  bytes v1_missing {
      0x00, 0x01, 0x00, 0x01 /*must_understand_struct.a.mheader*/,
      'a'/*must_understand_struct.a*/,
      0x00, 0x00, 0x00/*padding bytes*/,
      0x40, 0x02, 0x00, 0x01 /*must_understand_struct.b.mheader*/,
      'c'/*must_understand_struct.c*/,
      0x00, 0x00, 0x00/*padding bytes*/,
      0x7F, 0x02, 0x00, 0x00 /*optional_mutable_struct list termination header*/
      };
  bytes v2_missing {
      0x00, 0x00, 0x00, 0x15, /*dheader*/
      0x40, 0x00, 0x00, 0x01, /*must_understand_struct.a.emheader*/
      0x00, 0x00, 0x00, 0x01, /*must_understand_struct.a.emheader.nextint*/
      'a', /*must_understand_struct.a*/
      0x00, 0x00, 0x00, /*padding bytes*/
      0xC0, 0x00, 0x00, 0x03, /*must_understand_struct.c.emheader*/
      0x00, 0x00, 0x00, 0x01, /*must_understand_struct.c.emheader.nextint*/
      'c', /*must_understand_struct.c*/
      };
  VerifyRead(v1_missing, MU, xcdr_v1_stream, false, false, true);
  VerifyRead(v2_missing, MU, xcdr_v2_stream, false, false, true);

  /*these cdr streams contain a field with id 0 which is not in the definition of
    must_understand_struct but is set to must_understand, and therefore must
    also be rejected on read*/
  bytes v1_additional {
      0x40, 0x00, 0x00, 0x01 /*must_understand field with id = 0 mheader*/,
      'x'/*must_understand_struct[0]*/,
      0x00, 0x00, 0x00/*padding bytes*/,
      0x00, 0x01, 0x00, 0x01 /*must_understand_struct.a.mheader*/,
      'a'/*must_understand_struct.a*/,
      0x00, 0x00, 0x00/*padding bytes*/,
      0x40, 0x02, 0x00, 0x01 /*must_understand_struct.b.mheader*/,
      'b'/*must_understand_struct.b*/,
      0x00, 0x00, 0x00/*padding bytes*/,
      0x40, 0x03, 0x00, 0x01 /*must_understand_struct.c.mheader*/,
      'c'/*must_understand_struct.c*/,
      0x00, 0x00, 0x00/*padding bytes*/,
      0x7F, 0x02, 0x00, 0x00 /*optional_mutable_struct list termination header*/
      };
  bytes v2_additional {
      0x00, 0x00, 0x00, 0x21, /*dheader*/
      0x40, 0x00, 0x00, 0x00, /*must_understand field with id = 0 emheader*/
      0x00, 0x00, 0x00, 0x01, /*nextint*/
      'x'/*must_understand_struct[0]*/,
      0x40, 0x00, 0x00, 0x01, /*must_understand_struct.a.emheader*/
      0x00, 0x00, 0x00, 0x01, /*must_understand_struct.a.emheader.nextint*/
      'a', /*must_understand_struct.a*/
      0x00, 0x00, 0x00, /*padding bytes*/
      0xC0, 0x00, 0x00, 0x02, /*must_understand_struct.b.emheader*/
      0x00, 0x00, 0x00, 0x01, /*must_understand_struct.b.emheader.nextint*/
      'b', /*must_understand_struct.b*/
      0x00, 0x00, 0x00, /*padding bytes*/
      0xC0, 0x00, 0x00, 0x03, /*must_understand_struct.c.emheader*/
      0x00, 0x00, 0x00, 0x01, /*must_understand_struct.c.emheader.nextint*/
      'c', /*must_understand_struct.c*/
      };
  VerifyRead(v1_missing, MU, xcdr_v1_stream, false, false, true);
  VerifyRead(v2_missing, MU, xcdr_v2_stream, false, false, true);
}

/*verifying correct insertion of d-headers after opening arrays and sequences of non-primitive types*/

TEST_F(CDRStreamer, d_header_insertion)
{
  d_hdr_sequences DS({enum_8::fourth_8, enum_8::third_8, enum_8::second_8, enum_8::first_8},
                     {{enum_8::fourth_8},
                      {enum_8::third_8, enum_8::third_8},
                      {enum_8::second_8, enum_8::second_8, enum_8::second_8},
                      {enum_8::first_8, enum_8::first_8, enum_8::first_8, enum_8::first_8}});

  bytes DS_key {
    0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00 /*d_hdr_sequences.c*/
    };
  bytes DS_basic {
    0x00, 0x00, 0x00, 0x03,
    0x00, 0x00, 0x00, 0x02,
    0x00, 0x00, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, /*d_hdr_sequences.c*/

    0x00, 0x00, 0x00, 0x04, /*d_hdr_sequences.l.length*/
    0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x03, /*d_hdr_sequences.l[0]*/
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x02, /*d_hdr_sequences.l[1]*/
    0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, /*d_hdr_sequences.l[2]*/
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /*d_hdr_sequences.l[3]*/
    };
  bytes DS_v1 {
    0x03, 0x02, 0x01, 0x00, /*d_hdr_sequences.c*/

    0x00, 0x00, 0x00, 0x04, /*d_hdr_sequences.l.length*/
    0x00, 0x00, 0x00, 0x01, 0x03, /*d_hdr_sequences.l[0]*/
    0x00, 0x00, 0x00, /*padding bytes*/
    0x00, 0x00, 0x00, 0x02, 0x02, 0x02, /*d_hdr_sequences.l[1]*/
    0x00, 0x00, /*padding bytes*/
    0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x01, /*d_hdr_sequences.l[2]*/
    0x00, /*padding bytes*/
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, /*d_hdr_sequences.l[3]*/
    };
  bytes DS_v2 {
    0x00, 0x00, 0x00, 0x04, /*d_hdr_sequences.c.d_header*/
    0x03, 0x02, 0x01, 0x00, /*d_hdr_sequences.c*/

    0x00, 0x00, 0x00, 0x34, /*d_hdr_sequences.l.d_header*/
    0x00, 0x00, 0x00, 0x04, /*d_hdr_sequences.l.length*/
    0x00, 0x00, 0x00, 0x05, /*d_hdr_sequences.l[0].d_header*/
    0x00, 0x00, 0x00, 0x01, 0x03, /*d_hdr_sequences.l[0]*/
    0x00, 0x00, 0x00, /*padding bytes*/
    0x00, 0x00, 0x00, 0x06, /*d_hdr_sequences.l[1].d_header*/
    0x00, 0x00, 0x00, 0x02, 0x02, 0x02, /*d_hdr_sequences.l[1]*/
    0x00, 0x00, /*padding bytes*/
    0x00, 0x00, 0x00, 0x07, /*d_hdr_sequences.l[2].d_header*/
    0x00, 0x00, 0x00, 0x03, 0x01, 0x01, 0x01, /*d_hdr_sequences.l[2]*/
    0x00, /*padding bytes*/
    0x00, 0x00, 0x00, 0x08, /*d_hdr_sequences.l[3].d_header*/
    0x00, 0x00, 0x00, 0x04, 0x00, 0x00, 0x00, 0x00, /*d_hdr_sequences.l[3]*/
    };

  readwrite_test(DS, DS, DS_basic, DS_key, basic_cdr_stream);
  readwrite_test(DS, DS, DS_v1, DS_key, xcdr_v1_stream);
  readwrite_test(DS, DS, DS_v2, DS_key, xcdr_v2_stream);
}

/*verifying reads/writes of structs containing bitmasks*/

TEST_F(CDRStreamer, cdr_bitmask)
{
  bitmask_struct BMS(bm1Bits::bm_2 | bm1Bits::bm_5, bm1Bits::bm_3 | bm1Bits::bm_6);

  bytes struct_normal {
    0x00, 0x24, /*bitmask_struct::c*/
    0x00, 0x48  /*bitmask_struct::d*/
    };

  bytes struct_key {
    0x00, 0x24 /*bitmask_struct::c*/
    };

  bitmask_union BMU;
  BMU.c(bm1Bits::bm_3 | bm1Bits::bm_6);

  bitmask_union BMK;
  BMK.c(0);

  bytes union_normal {
    0x00, 0x00, 0x00, 0x01, /*bitmask_union::discriminator*/
    0x00, 0x48              /*bitmask_union::c*/
    };

  bytes union_key {
    0x00, 0x00, 0x00, 0x01 /*bitmask_union::discriminator*/
    };

  stream_test(BMS, struct_normal, struct_key);
  stream_test_union(BMU, BMK, union_normal, union_key);

}
