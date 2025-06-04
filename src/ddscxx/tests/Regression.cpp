// Copyright(c) 2006 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "Util.hpp"
#include "dds/ddsrt/misc.h"

DDSRT_WARNING_GNUC_OFF(maybe-uninitialized)

#include "dds/dds.hpp"
#include <gtest/gtest.h>
#include <type_traits>
#include "RegressionModels.hpp"
#include "RegressionModels_pragma.hpp"

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
void VerifyWrite_impl(const T& in, const bytes &out, S &stream, bool as_key = false, bool write_success = true, bool compare_success = true)
{
  bytes buffer;
  bool result = move(stream, in, as_key ? key_mode::unsorted : key_mode::not_key);
  ASSERT_EQ(result, write_success);

  if (!result)
    return;

  buffer.resize(stream.position());
  stream.set_buffer(buffer.data(), buffer.size());
  ASSERT_TRUE(write(stream, in, as_key ? key_mode::unsorted : key_mode::not_key));

  result = (buffer == out);
  ASSERT_EQ(result, compare_success);
}

template<typename T, typename S>
void VerifyRead_impl(const bytes &in, const T& out, S &stream, bool as_key = false, bool read_success = true, bool compare_success = true)
{
  bytes incopy(in);
  T buffer;
  stream.set_buffer(incopy.data(), incopy.size());
  bool result = read(stream, buffer, as_key ? key_mode::unsorted : key_mode::not_key);
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
{\
streamer streamer_1(endianness::little_endian);\
VerifyRead_impl(test_cdr, test_struct, streamer_1);\
streamer streamer_2(endianness::little_endian);\
VerifyWrite_impl(test_struct, test_cdr, streamer_2);\
}

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

  readwrite_test(model, d_hdr_bytes, xcdr_v2_stream);
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

  readwrite_test(model, union_bytes, xcdr_v2_stream);
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

  readwrite_test(s_1, s_o_absent_bytes, xcdr_v2_stream);
  readwrite_test(s_2, s_o_absent_bytes, xcdr_v2_stream);

  s_1.c() = 0x12345678;
  s_2.c() = s_1.c();

  readwrite_test(s_1, s_o_present_bytes, xcdr_v2_stream);
  readwrite_test(s_2, s_o_present_bytes, xcdr_v2_stream);
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

  readwrite_test(s, s_o_2_all_absent_bytes, xcdr_v2_stream);

  s.d() = 187;

  readwrite_test(s, s_o_2_c_absent_bytes, xcdr_v2_stream);

  s.d().reset();
  s.c() = 170;

  readwrite_test(s, s_o_2_d_absent_bytes, xcdr_v2_stream);

  s.d() = 187;

  readwrite_test(s, s_o_2_all_present_bytes, xcdr_v2_stream);
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

TEST_F(Regression, union_duplicate_string_types)
{
  duplicate_string_types_union d_t;
}

TEST_F(Regression, union_duplicate_array_types)
{
  duplicate_array_types_union d_t;
}

TEST_F(Regression, union_duplicate_bitmask_types)
{
  duplicate_bitmask_types_union d_t;
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

  readwrite_test(s, s_bm1_bytes, xcdr_v2_stream);
}

TEST_F(Regression, emumerators_properties)
{
  EXPECT_EQ(org::eclipse::cyclonedds::core::cdr::get_bit_bound<e1>(), bit_bound::bb_8_bits);
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

  readwrite_test(s, s_e1_bytes, xcdr_v2_stream);
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

  readwrite_test(w_a, w_a_bytes, xcdr_v2_stream);
  readwrite_test(w_b, w_b_bytes, xcdr_v2_stream);
}

TEST_F(Regression, direct_typedef_of_primitive)
{
  s_td_bool_seq_arr s;

  s.c({true,false,true,false,true});

  bytes s_td_bool_seq_arr_bytes =
    {0x05, 0x00, 0x00, 0x00, /*c.length(5)*/
     0x01, 0x00, 0x01, 0x00, 0x01, /*c.data*/
    };
  readwrite_test(s, s_td_bool_seq_arr_bytes, basic_cdr_stream);
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
  readwrite_test(u, u_s_inner_bytes, basic_cdr_stream);
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
  readwrite_test(s, struct_seq_e1_bytes, xcdr_v2_stream);
}

TEST_F(Regression, key_value_of_appendables)
{
  s_final s_f;
  s_appendable s_a;

  s_f.s() = "abcdef";
  s_a.s() = "abcdef";

  ddsi_keyhash_t kh_f, kh_a;

  unsigned char k[] = {0xcd, 0x34, 0x4d, 0x59, 0xec, 0x90, 0xb9, 0x62, 0xca, 0xb1, 0x41, 0xcf, 0x2a, 0x5d, 0xa6, 0xcf};

  ASSERT_TRUE(to_key<s_final>(s_f, kh_f));
  EXPECT_EQ(0, memcmp(k, kh_f.value, 16));
  ASSERT_TRUE(to_key<s_appendable>(s_a, kh_a));
  EXPECT_EQ(0, memcmp(k, kh_a.value, 16));
}

TEST_F(Regression, memberless_struct)
{
  const auto &props = get_type_props<s_memberless>();

  xcdr_v1_stream v1(endianness::big_endian);
  v1.set_mode(cdr_stream::stream_mode::read, key_mode::not_key);
  auto ptr1 = v1.first_entity(props.data());
  EXPECT_EQ(ptr1, nullptr);


  xcdr_v2_stream v2(endianness::big_endian);
  v2.set_mode(cdr_stream::stream_mode::read, key_mode::not_key);
  auto ptr2 = v2.first_entity(props.data());
  EXPECT_EQ(ptr2, nullptr);
}

TEST_F(Regression, unaligned_access)
{
  uint64_t buffer[3];  //assures alignment at 8 bytes

  uint8_t *correct = reinterpret_cast<uint8_t*>(buffer);
  uint8_t *incorrect = correct+4;
  for (uint8_t i = 0; i < sizeof(buffer); i++)
    *(correct+i) = i;

  //make sure byte swaps occur
  const endianness swap_end = native_endianness() == endianness::big_endian ? endianness::little_endian : endianness::big_endian;

  s_unaligned_access s;

  basic_cdr_stream b(swap_end);
  b.set_buffer(correct, 16);
  ASSERT_TRUE(org::eclipse::cyclonedds::core::cdr::read(b, s, key_mode::not_key));
  ASSERT_EQ(s.l(), int32_t(0x00010203));  //size 4 reads should be done at 4 byte offsets in stream
  ASSERT_EQ(s.ll(), int64_t(0x08090A0B0C0D0E0F));  //size 8 reads should be done at 8 byte offsets in stream

  b.set_buffer(incorrect, 16);
  ASSERT_TRUE(org::eclipse::cyclonedds::core::cdr::read(b, s, key_mode::not_key));
  ASSERT_EQ(s.l(), int32_t(0x04050607));  //size 4 reads should be done at 4 byte offsets in stream
  ASSERT_EQ(s.ll(), int64_t(0x0C0D0E0F10111213));  //size 8 reads should be done at 8 byte offsets in stream

  xcdr_v1_stream v1(swap_end);

  v1.set_buffer(correct, 16);
  ASSERT_TRUE(org::eclipse::cyclonedds::core::cdr::read(v1, s, key_mode::not_key));
  ASSERT_EQ(s.l(), int32_t(0x00010203));  //size 4 reads should be done at 4 byte offsets in stream
  ASSERT_EQ(s.ll(), int64_t(0x08090A0B0C0D0E0F));  //size 8 reads should be done at 8 byte offsets in stream

  v1.set_buffer(incorrect, 16);
  ASSERT_TRUE(org::eclipse::cyclonedds::core::cdr::read(v1, s, key_mode::not_key));
  ASSERT_EQ(s.l(), int32_t(0x04050607));  //size 4 reads should be done at 4 byte offsets in stream
  ASSERT_EQ(s.ll(), int64_t(0x0C0D0E0F10111213));  //size 8 reads should be done at 8 byte offsets in stream

  xcdr_v2_stream v2(swap_end);

  v2.set_buffer(correct, 16);
  ASSERT_TRUE(org::eclipse::cyclonedds::core::cdr::read(v2, s, key_mode::not_key));
  ASSERT_EQ(s.l(), int32_t(0x00010203));  //size 4 reads should be done at 4 byte offsets in stream
  ASSERT_EQ(s.ll(), int64_t(0x0405060708090A0B));  //size 4 reads should be done at 4 byte offsets in stream

  v2.set_buffer(incorrect, 16);
  ASSERT_TRUE(org::eclipse::cyclonedds::core::cdr::read(v2, s, key_mode::not_key));
  ASSERT_EQ(s.l(), int32_t(0x04050607));  //size 4 reads should be done at 4 byte offsets in stream
  ASSERT_EQ(s.ll(), int64_t(0x08090A0B0C0D0E0F));  //size 4 reads should be done at 4 byte offsets in stream
}

TEST_F(Regression, union_comparisons)
{
  regression_models::union_without_default u_1, u_2, u_3, u_4;
  u_1.s("abcdef", regression_enum::case_1);
  u_2.s("fedcba", regression_enum::case_1);
  u_3.s("abcdef", regression_enum::case_2);
  u_4._d(regression_enum::case_3);

  regression_models::union_with_default w_1, w_2, w_3, w_4, w_5;
  w_1.s("abcdef", regression_enum::case_1);
  w_2.s("fedcba", regression_enum::case_1);
  w_3.s("abcdef", regression_enum::case_2);
  w_4.i(123);
  w_5.i(456);

  EXPECT_EQ(u_1, u_1);
  EXPECT_NE(u_1, u_2);
  EXPECT_NE(u_1, u_3);
  EXPECT_NE(u_1, u_4);
  EXPECT_NE(u_2, u_1);
  EXPECT_EQ(u_2, u_2);
  EXPECT_NE(u_2, u_3);
  EXPECT_NE(u_2, u_4);
  EXPECT_NE(u_3, u_1);
  EXPECT_NE(u_3, u_2);
  EXPECT_EQ(u_3, u_3);
  EXPECT_NE(u_3, u_4);
  EXPECT_NE(u_4, u_1);
  EXPECT_NE(u_4, u_2);
  EXPECT_NE(u_4, u_3);
  EXPECT_EQ(u_4, u_4);

  EXPECT_EQ(w_1, w_1);
  EXPECT_NE(w_1, w_2);
  EXPECT_NE(w_1, w_3);
  EXPECT_NE(w_1, w_4);
  EXPECT_NE(w_1, w_5);
  EXPECT_NE(w_2, w_1);
  EXPECT_EQ(w_2, w_2);
  EXPECT_NE(w_2, w_3);
  EXPECT_NE(w_2, w_4);
  EXPECT_NE(w_2, w_5);
  EXPECT_NE(w_3, w_1);
  EXPECT_NE(w_3, w_2);
  EXPECT_EQ(w_3, w_3);
  EXPECT_NE(w_3, w_4);
  EXPECT_NE(w_3, w_5);
  EXPECT_NE(w_4, w_1);
  EXPECT_NE(w_4, w_2);
  EXPECT_NE(w_4, w_3);
  EXPECT_EQ(w_4, w_4);
  EXPECT_NE(w_4, w_5);
  EXPECT_NE(w_5, w_1);
  EXPECT_NE(w_5, w_2);
  EXPECT_NE(w_5, w_3);
  EXPECT_NE(w_5, w_4);
  EXPECT_EQ(w_5, w_5);
}

//functions to check whether a member of a type exists
struct Has_a_helper { int a_; };

template <typename T>
struct Has_a : public T, Has_a_helper
 {
   template <typename U = Has_a, typename = decltype(U::a_)>
   static constexpr std::false_type check (int);
   static constexpr std::true_type check (long);
   using type = decltype(check(0));
   static constexpr auto value = type::value;
 };

struct Has_b_helper { int b_; };

template <typename T>
struct Has_b : public T, Has_b_helper
 {
   template <typename U = Has_b, typename = decltype(U::b_)>
   static constexpr std::false_type check (int);
   static constexpr std::true_type check (long);
   using type = decltype(check(0));
   static constexpr auto value = type::value;
 };

struct Has_c_helper { int c_; };

template <typename T>
struct Has_c : public T, Has_c_helper
 {
   template <typename U = Has_c, typename = decltype(U::c_)>
   static constexpr std::false_type check (int);
   static constexpr std::true_type check (long);
   using type = decltype(check(0));
   static constexpr auto value = type::value;
 };

//functions to populate the member (if it exists) with random data

template<typename T, std::enable_if_t<Has_a<T>::value, bool> = true > void populate_a(T &in, const int val) { in.a(val); }
template<typename T, std::enable_if_t<!Has_a<T>::value, bool> = true > void populate_a(T &, const int) { ; }

template<typename T, std::enable_if_t<Has_b<T>::value, bool> = true > void populate_b(T &in, const int val) { in.b(val); }
template<typename T, std::enable_if_t<!Has_b<T>::value, bool> = true > void populate_b(T &, const int) { ; }

template<typename T, std::enable_if_t<Has_c<T>::value, bool> = true > void populate_c(T &in, const int val) { in.c(val); }
template<typename T, std::enable_if_t<!Has_c<T>::value, bool> = true > void populate_c(T &, const int) { ; }

//functions to check whether both types have a specific member

template <typename T, typename U> constexpr bool BothHave_a() { return Has_a<T>::value && Has_a<U>::value; }

template <typename T, typename U> constexpr bool BothHave_b() { return Has_b<T>::value && Has_b<U>::value; }

template <typename T, typename U> constexpr bool BothHave_c() { return Has_c<T>::value && Has_c<U>::value; }

//functions to compare the contents of the member on both types (if it exists)

template<typename T, typename U, std::enable_if_t<BothHave_a<T,U>(), bool> = true >
bool compare_a(const T &left, const U &right) { return left.a() == right.a(); }
template<typename T, typename U, std::enable_if_t<!BothHave_a<T,U>(), bool> = true >
bool compare_a(const T &, const U &) { return true; }

template<typename T, typename U, std::enable_if_t<BothHave_b<T,U>(), bool> = true >
bool compare_b(const T &left, const U &right) { return left.b() == right.b(); }
template<typename T, typename U, std::enable_if_t<!BothHave_b<T,U>(), bool> = true >
bool compare_b(const T &, const U &) { return true; }

template<typename T, typename U, std::enable_if_t<BothHave_c<T,U>(), bool> = true >
bool compare_c(const T &left, const U &right) { return left.c() == right.c(); }
template<typename T, typename U, std::enable_if_t<!BothHave_c<T,U>(), bool> = true >
bool compare_c(const T &, const U &) { return true; }

//function to test writing one struct and attempting to read it back as another
template<typename S, typename T, typename U>
bool test_appendable_mutable()
{
  bytes buffer;
  T towrite;

  populate_a(towrite, int(0x12345678));
  populate_b(towrite, int(0x55555555));
  populate_c(towrite, int(0x87654321));

  S serializer;
  if (!move(serializer, towrite, key_mode::not_key))
    return false;

  buffer.resize(serializer.position());
  serializer.reset();
  serializer.set_buffer(buffer.data(), buffer.size());

  if (!write(serializer, towrite, key_mode::not_key))
    return false;

  serializer.reset();
  serializer.set_buffer(buffer.data(), buffer.size());

  U toread;
  if (!read(serializer, toread, key_mode::not_key))
    return false;

  //compare fields
  return compare_a(towrite, toread) &&
         compare_b(towrite, toread) &&
         compare_c(towrite, toread);
}

//test naked appendable structs
template<typename S>
void appendable_tests ()
{
  //appendable can skip/truncate members that have not been serialized...
  auto result = test_appendable_mutable<S, appendable_extended, appendable_base>();
  EXPECT_TRUE(result);

  result = test_appendable_mutable<S, appendable_extended_k, appendable_base>();
  EXPECT_TRUE(result);

  result = test_appendable_mutable<S, appendable_extended_optional, appendable_base>();
  EXPECT_TRUE(result);

  result = test_appendable_mutable<S, appendable_base, appendable_extended>();
  EXPECT_TRUE(result);

  result = test_appendable_mutable<S, appendable_extended_k, appendable_extended>();
  EXPECT_TRUE(result);

  result = test_appendable_mutable<S, appendable_extended, appendable_extended_k>();
  EXPECT_TRUE(result);

  result = test_appendable_mutable<S, appendable_base, appendable_extended_optional>();
  EXPECT_TRUE(result);

  // ... except for key fields
  result = test_appendable_mutable<S, appendable_base, appendable_extended_k>();
  EXPECT_FALSE(result);
}

//test naked mutable structs
template<typename S>
void mutable_tests ()
{
  //mutable types can always be extended/truncated when no must understand fields are dropped
  auto result = test_appendable_mutable<S, mutable_base, mutable_extended>();
  EXPECT_TRUE(result);

  result = test_appendable_mutable<S, mutable_extended, mutable_base>();
  EXPECT_TRUE(result);

  //must_understand fields must be deserialized in the sending and receiving stream
  result = test_appendable_mutable<S, mutable_base, mutable_extended_mu>();
  EXPECT_FALSE(result);

  result = test_appendable_mutable<S, mutable_extended_mu, mutable_base>();
  EXPECT_FALSE(result);

  //the field "c" is present and we understand it, so no problem here
  result = test_appendable_mutable<S, mutable_extended, mutable_extended_mu>();
  EXPECT_TRUE(result);

  result = test_appendable_mutable<S, mutable_extended_mu, mutable_extended>();
  EXPECT_TRUE(result);

  //truncated mutables can be received as everything...
  result = test_appendable_mutable<S, mutable_truncated, mutable_base>();
  EXPECT_TRUE(result);

  result = test_appendable_mutable<S, mutable_truncated, mutable_extended>();
  EXPECT_TRUE(result);

  result = test_appendable_mutable<S, mutable_truncated, mutable_extended_optional>();
  EXPECT_TRUE(result);

  //... except for types with a must_understand field
  result = test_appendable_mutable<S, mutable_truncated, mutable_extended_mu>();
  EXPECT_FALSE(result);
}

//testing truncation of appendable/mutable types
TEST_F(Regression, truncation)
{
  appendable_tests<xcdr_v1_stream>();
  appendable_tests<xcdr_v2_stream>();
  mutable_tests<xcdr_v1_stream>();
  mutable_tests<xcdr_v2_stream>();
}

//function to test writing one struct containing sequential structs and attempting to read it back as another
template<typename S, typename T, typename U>
bool test_sequential()
{
  bytes buffer;
  T towrite;

  int i = int(0x12345678),
      j = int(0x55555555),
      k = int(0x87654321);
  for (auto & e:towrite.m()) {
    populate_a(e, i++);
    populate_b(e, j++);
    populate_c(e, k++);
  }

  S serializer;
  if (!move(serializer, towrite, key_mode::not_key))
    return false;

  buffer.resize(serializer.position());
  serializer.reset();
  serializer.set_buffer(buffer.data(), buffer.size());

  if (!write(serializer, towrite, key_mode::not_key))
    return false;

  serializer.reset();
  serializer.set_buffer(buffer.data(), buffer.size());

  U toread;
  if (!read(serializer, toread, key_mode::not_key))
    return false;

  auto it1 = towrite.m().begin();
  auto it2 = toread.m().begin();
  while (it1 != towrite.m().end() &&
         it2 != toread.m().end()) {
    if (compare_a(*it1, *it2) &&
        compare_b(*it1, *it2) &&
        compare_c(*it1, *it2)) {
      it1++;
      it2++;
    } else {
      return false;
    }
  }

  return true;
}

//test sequential appendable structs
template <typename S>
void sequential_appendable_tests()
{
  //appendable can skip/truncate members that have not been serialized ...
  auto result = test_sequential<S, appendable_extended_seq, appendable_base_seq>();
  EXPECT_TRUE(result);

  result = test_sequential<S, appendable_extended_k_seq, appendable_base_seq>();
  EXPECT_TRUE(result);

  result = test_sequential<S, appendable_extended_optional_seq, appendable_base_seq>();
  EXPECT_TRUE(result);

  result = test_sequential<S, appendable_base_seq, appendable_extended_seq>();
  EXPECT_TRUE(result);

  result = test_sequential<S, appendable_extended_k_seq, appendable_extended_seq>();
  EXPECT_TRUE(result);

  result = test_sequential<S, appendable_extended_seq, appendable_extended_k_seq>();
  EXPECT_TRUE(result);

  result = test_sequential<S, appendable_base_seq, appendable_extended_optional_seq>();
  EXPECT_TRUE(result);

  // ... except for key fields
  result = test_sequential<S, appendable_base_seq, appendable_extended_k_seq>();
  EXPECT_FALSE(result);
}

//test sequential mutable structs
template <typename S>
void sequential_mutable_tests()
{
  //mutable types can always be extended/truncated when no must understand fields are dropped
  auto result = test_sequential<S, mutable_base_seq, mutable_extended_seq>();
  EXPECT_TRUE(result);

  result = test_sequential<S, mutable_extended_seq, mutable_base_seq>();
  EXPECT_TRUE(result);

  //must_understand fields must be deserialized in the sending and receiving stream
  result = test_sequential<S, mutable_base_seq, mutable_extended_mu_seq>();
  EXPECT_FALSE(result);

  result = test_sequential<S, mutable_extended_mu_seq, mutable_base_seq>();
  EXPECT_FALSE(result);

  //the field "c" is present and we understand it, so no problem here
  result = test_sequential<S, mutable_extended_seq, mutable_extended_mu_seq>();
  EXPECT_TRUE(result);

  result = test_sequential<S, mutable_extended_mu_seq, mutable_extended_seq>();
  EXPECT_TRUE(result);

  //truncated mutables can be received as everything...
  result = test_sequential<S, mutable_truncated_seq, mutable_base_seq>();
  EXPECT_TRUE(result);

  result = test_sequential<S, mutable_truncated_seq, mutable_extended_seq>();
  EXPECT_TRUE(result);

  result = test_sequential<S, mutable_truncated_seq, mutable_extended_optional_seq>();
  EXPECT_TRUE(result);

  //... except for types with a must_understand field
  result = test_sequential<S, mutable_truncated_seq, mutable_extended_mu_seq>();
  EXPECT_FALSE(result);

}

//testing truncation of sequences of appendable/mutable types
TEST_F(Regression, sequential_truncation)
{
  //appendable is only for outer level structs in xcdrv1
  sequential_mutable_tests<xcdr_v1_stream>();
  sequential_appendable_tests<xcdr_v2_stream>();
  sequential_mutable_tests<xcdr_v2_stream>();
}

DDSRT_WARNING_GNUC_ON(maybe-uninitialized)
