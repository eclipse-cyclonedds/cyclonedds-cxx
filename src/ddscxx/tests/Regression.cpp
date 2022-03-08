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
#include "dds/dds.hpp"
#include <gtest/gtest.h>
#include "RegressionModels.hpp"

typedef std::vector<unsigned char> bytes;

using namespace org::eclipse::cyclonedds::core::cdr;
using namespace regression_models;

/**
 * Fixture for the Regression tests
 */
class Regression : public ::testing::Test
{
public:

    Regression() { }

    void SetUp() { }

    void TearDown() { }

};


template<typename T, typename S>
void VerifyWrite(const T& in, const bytes &out, S stream, bool as_key = false, bool write_success = true, bool compare_success = true)
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
void VerifyRead(const bytes &in, const T& out, S stream, bool as_key = false, bool read_success = true, bool compare_success = true)
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

#define readwrite_test(test_struct, test_cdr, streamer)\
VerifyRead(test_cdr, test_struct, streamer);\
VerifyWrite(test_struct, test_cdr, streamer);

TEST_F(Regression, d_header_insertion)
{
  bytes d_hdr_bytes =
  {
    0x25, 0x00, 0x00, 0x00, //c.dheader()
    0x0D, 0x00, 0x00, 0x00, //c[0].d.dheader()
    0x01, 0x00, 0x00, 0x00, //c[0].d[0].length(1)
    0x00, 0x00, 0x00, 0x00, //c[0].d[0].c_str("")
    0x01, 0x00, 0x00, 0x00, //c[0].d[1].length(1)
    0x00, 0x00, 0x00, 0x00, //c[0].d[1].c_str(""), padding(3)
    0x0D, 0x00, 0x00, 0x00, //c[1].d.dheader()
    0x01, 0x00, 0x00, 0x00, //c[1].d[0].length(1)
    0x00, 0x00, 0x00, 0x00, //c[1].d[0].c_str("")
    0x01, 0x00, 0x00, 0x00, //c[1].d[1].length(1)
    0x00                    //c[1].d[1].c_str("")
  };

  s_2 model({s_1({"",""}),s_1({"",""})});

  readwrite_test(model, d_hdr_bytes, xcdr_v2_stream(endianness::little_endian));
}

TEST_F(Regression, member_completeness_unions)
{
  bytes union_bytes =
  {
    'b',  //c.switch('b')
    'b',  //c.u.switch('b')
    0x03, 0x00,  //c.u.s(3)
  };

  U u;
  u.s(3);
  V v;
  v.u(u);
  s_u model(v);

  readwrite_test(model, union_bytes, xcdr_v2_stream(endianness::little_endian));
}

TEST_F(Regression, optional_of_typedef)
{
  bytes s_o_absent_bytes =
  {
    0x00                    //s_o.present(false)
  };
  bytes s_o_present_bytes =
  {
    0x01, 0x00, 0x00, 0x00, //s_o.present(true)
    0x78, 0x56, 0x34, 0x12  //s_o.c(0x12345678)
  };

  s_o     s_1;  //optional of primitive
  s_o_td  s_2;  //optional of typedeffed primitive

  readwrite_test(s_1, s_o_absent_bytes, xcdr_v2_stream(endianness::little_endian));
  readwrite_test(s_2, s_o_absent_bytes, xcdr_v2_stream(endianness::little_endian));

  s_1.c() = 0x12345678;
  s_2.c() = s_1.c();

  readwrite_test(s_1, s_o_present_bytes, xcdr_v2_stream(endianness::little_endian));
  readwrite_test(s_2, s_o_present_bytes, xcdr_v2_stream(endianness::little_endian));
}

TEST_F(Regression, optionals_delimiters_unbalance)
{
  bytes s_o_2_all_absent_bytes =
  {
    0x00, 0x00, 0x00, 0x00,  //s_o_2.dheader(0)
  };
  bytes s_o_2_c_absent_bytes =
  {
    0x0C, 0x00, 0x00, 0x00,  //s_o_2.dheader(12)
    0x01, 0x00, 0x00, 0x40,  //s_o_2.d.emheader(id = 1, lc = nextint)
    0x04, 0x00, 0x00, 0x00,  //s_o_2.d.emheader.nextint(4)
    0xBB, 0x00, 0x00, 0x00   //s_o_2.d(187)
  };
  bytes s_o_2_d_absent_bytes =
  {
    0x0C, 0x00, 0x00, 0x00,  //s_o_2.dheader(12)
    0x00, 0x00, 0x00, 0x40,  //s_o_2.c.emheader(id = 0, lc = nextint)
    0x04, 0x00, 0x00, 0x00,  //s_o_2.c.emheader.nextint(4)
    0xAA, 0x00, 0x00, 0x00,  //s_o_2.c(170)
  };
  bytes s_o_2_all_present_bytes =
  {
    0x18, 0x00, 0x00, 0x00,  //s_o_2.dheader(24)
    0x00, 0x00, 0x00, 0x40,  //s_o_2.c.emheader(id = 0, lc = nextint)
    0x04, 0x00, 0x00, 0x00,  //s_o_2.c.emheader.nextint(4)
    0xAA, 0x00, 0x00, 0x00,  //s_o_2.c(170)
    0x01, 0x00, 0x00, 0x40,  //s_o_2.d.emheader(id = 1, lc = nextint)
    0x04, 0x00, 0x00, 0x00,  //s_o_2.d.emheader.nextint(4)
    0xBB, 0x00, 0x00, 0x00   //s_o_2.d(187)
  };

  s_o_2 s;

  readwrite_test(s, s_o_2_all_absent_bytes, xcdr_v2_stream(endianness::little_endian));

  s.d() = 187;

  readwrite_test(s, s_o_2_c_absent_bytes, xcdr_v2_stream(endianness::little_endian));

  s.d().reset();
  s.c() = 170;

  readwrite_test(s, s_o_2_d_absent_bytes, xcdr_v2_stream(endianness::little_endian));

  s.d() = 187;

  readwrite_test(s, s_o_2_all_present_bytes, xcdr_v2_stream(endianness::little_endian));
}

TEST_F(Regression, propagate_xtypes_version_reqs)
{
  using org::eclipse::cyclonedds::topic::TopicTraits;
  using org::eclipse::cyclonedds::topic::encoding_version;
  using org::eclipse::cyclonedds::topic::extensibility;

  ASSERT_EQ(TopicTraits<td_1>::getExtensibility(), extensibility::ext_appendable);
  ASSERT_EQ(TopicTraits<td_1>::minXCDRVersion(), encoding_version::xcdr_v2);
  ASSERT_EQ(TopicTraits<td_3>::getExtensibility(), extensibility::ext_final);
  ASSERT_EQ(TopicTraits<td_3>::minXCDRVersion(), encoding_version::xcdr_v2);
}

TEST_F(Regression, union_duplicate_types)
{
  duplicate_types_union d_t;
}
