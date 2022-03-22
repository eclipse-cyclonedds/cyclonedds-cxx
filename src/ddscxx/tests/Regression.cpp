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

  EXPECT_EQ(TopicTraits<td_1>::getExtensibility(), extensibility::ext_appendable);
  EXPECT_EQ(TopicTraits<td_3>::getExtensibility(), extensibility::ext_final);
}

TEST_F(Regression, union_duplicate_types)
{
  duplicate_types_union d_t;
  d_t.l_1(456789);
  duplicate_types_union_2 d_t_2;
  d_t_2.b_1({1,2,3,4,5});
  duplicate_sequences_union d_s;
  d_s.c_1({1,2,3,4,5,6});
}

TEST_F(Regression, delimiters_bitmask)
{
  bytes s_bm1_bytes =
  {
    0x06, 0x00, 0x00, 0x00,  //s_bm1.dheader(6)
    0x03, 0x00, 0x06, 0x00, 0x05, 0x00 //s_bm1.c
  };

  s_bm1 s;
  s.c({b_0 | b_1, b_1 | b_2, b_2 | b_0});

  readwrite_test(s, s_bm1_bytes, xcdr_v2_stream(endianness::little_endian));
}

TEST_F(Regression, emumerators_properties)
{
  auto props = org::eclipse::cyclonedds::core::cdr::get_type_props<e1>();

  EXPECT_EQ(props.e_ext, extensibility::ext_appendable);
  EXPECT_EQ(props.e_bb, bb_8_bits);
  EXPECT_TRUE(props.xtypes_necessary);
  ASSERT_EQ(props.m_members_by_seq.size(), 1);
  EXPECT_FALSE(bool(props.m_members_by_seq.front()));
  ASSERT_EQ(props.m_members_by_id.size(), 1);
  EXPECT_FALSE(bool(props.m_members_by_id.front()));
  ASSERT_EQ(props.m_keys.size(), 1);
  EXPECT_FALSE(bool(props.m_keys.front()));
}

TEST_F(Regression, delimiters_emumerators)
{
  bytes s_e1_bytes =
  {
    0x03, 0x00, 0x00, 0x00,  //s_e1.dheader(3)
    0x03, 0x01, 0x02,  //s_e1.c
  };

  s_e1 s;
  s.c({e1::e_3, e1::e_1, e1::e_2});

  readwrite_test(s, s_e1_bytes, xcdr_v2_stream(endianness::little_endian));
}

TEST_F(Regression, arrays_in_union_case)
{
  W w_a, w_b;

  w_a.c({'c', 'd'});
  std::array<std::array<char,3>,2> arr = {std::array<char,3>({'j', 'i', 'h'}),std::array<char,3>({'g', 'f', 'e'})};
  w_b.d(arr);

  bytes w_a_bytes =
    {'a',           /*W::discriminator*/
     'c','d'},      /*W::c*/
        w_b_bytes =
    {'b',           /*W::discriminator*/
     'j', 'i', 'h', /*W::d[0]*/
     'g', 'f', 'e'  /*W::d[1]*/
    };

  readwrite_test(w_a, w_a_bytes, xcdr_v2_stream(endianness::little_endian));
  readwrite_test(w_b, w_b_bytes, xcdr_v2_stream(endianness::little_endian));
}

TEST_F(Regression, direct_typedef_of_primitive)
{
  s_td_bool_seq_arr s;

  s.c({true,false,true,false,true});

  bytes s_td_bool_seq_arr_bytes =
    {0x05, 0x00, 0x00, 0x00, /*c.length(5)*/
     0x01, 0x00, 0x01, 0x00, 0x01, /*c.data*/
    };
  readwrite_test(s, s_td_bool_seq_arr_bytes, basic_cdr_stream(endianness::little_endian));
}

TEST_F(Regression, union_array_case)
{
  s_u_struct_arr z;
}

TEST_F(Regression, direct_typedef_of_struct)
{
  u_s_inner u;

  seq_td_s_inner seq({s_inner('b'),s_inner('c')});
  u.c(seq);

  bytes u_s_inner_bytes =
    {'a', /*u.discriminator*/ 0x00, 0x00, 0x00, /*padding(3)*/
      0x02, 0x00, 0x00, 0x00, /*u.c.length(1)*/
     'b',  /*u.c[0]*/
     'c',  /*u.c[1]*/
    };
  readwrite_test(u, u_s_inner_bytes, basic_cdr_stream(endianness::little_endian));
}

TEST_F(Regression, typedef_of_sequence_of_enums)
{
  struct_seq_e1 s(seq_e1({e1::e_3, e1::e_2}));

  bytes struct_seq_e1_bytes =
    { 0x0E, 0x00, 0x00, 0x00, /*u.dheader(6)*/
      0x01, /*s.c.is_present(true)*/ 0x00, 0x00, 0x00, /*s.c.is_present.padding(3)*/
      0x06, 0x00, 0x00, 0x00, /*u.c.dheader(6)*/
      0x02, 0x00, 0x00, 0x00, /*u.c.length(2)*/
      0x03, 0x02              /*u.c.data()*/
    };
  readwrite_test(s, struct_seq_e1_bytes, xcdr_v2_stream(endianness::little_endian));
}
