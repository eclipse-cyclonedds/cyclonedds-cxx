// Copyright(c) 2023 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "StreamingUtility.hpp"
#include "KeyHashModels.hpp"
#include <org/eclipse/cyclonedds/topic/datatopic.hpp>

using namespace Keyhash_testing;

/**
 * Fixture for the KeyHash tests
 */
class KeyHash : public ::testing::Test
{
public:
    const bytes
        s_k_o_h = {
            0x01 /*a*/, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 /*padding*/,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 /*c*/},
        s_k_ih_h = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 /*c*/,
            0x01 /*a*/},
        s_s = {
            0x01 /*a*/, 0x0, 0x0, 0x0 /*padding*/,
            0x00, 0x00, 0x00, 0x04 /*b.length*/, 'a', 'b', 'c', '\0' /*b.c_str*/},
        s_n_h_1 = {
            0x01 /*z.a*/, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 /*padding*/,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 /*z.c*/,
            0x04 /*x*/},
        s_n_h_2 = {
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03 /*z.c*/,
            0x01 /*z.a*/, 0x04 /*x*/},
        n_f_i_h = {
            0x00, 0x00, 0x00, 0x07 /*f*/,
            0x03 /*d.z.a*/, 0x00, 0x00, 0x00 /*padding*/,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05/*d.z.c*/,
            0x02 /*d.y*/,
            0x01 /*d.x*/},
        n_f_i_2_h = {
            0x01 /*a.c.e.x*/,
            0x02 /*a.c.e.y*/,
            0x05 /*a.d.e.x*/,
            0x06 /*a.d.e.y*/},
        n_m_i_h = {
            0x00, 0x00, 0x00, 0x0b /*f*/,
            0x03 /*d.z.a*/, 0x00, 0x00, 0x00 /*padding*/,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05 /*d.z.c*/,
            0x02 /*d.y*/,
            0x01 /*d.x*/},
        s_o_h{
          0x00, 0x00, 0x00, 0x05, /*o_1.i_2*/
          0x00, 0x00, 0x00, 0x02, /*o_3.i_1*/
          0x00, 0x00, 0x00, 0x01  /*o_3.i_2*/},
        s_k_a_h{
          0x01, 0x02, 0x03, 0x04,
          0x05, 0x06, 0x07, 0x08,
          0x09, 0x10, 0x11, 0x12},
        s_k_a_s_b_h {
          0x00, 0x00, 0x00, 0x03 /*length*/, 'a', 'b', '\0',
          0x00/*pad*/,
          0x00, 0x00, 0x00, 0x03 /*length*/, 'c', 'd', '\0'};

    KeyHash()
    {
    }

    void SetUp() { }

    void TearDown() { }

};

#define hash_test(_struct, _bytes) { VerifyWrite(_struct, _bytes, basic_cdr_stream, key_mode::sorted, true, true); }

TEST_F(KeyHash, basic_types)
{
  SerdataKeyOrder s_k_o{1,2,3};
  SerdataKeyOrderId s_k_i{1,2,3};
  SerdataKeyOrderHashId s_k_h{1,2,3};
  SerdataKeyOrderAppendable s_a{1,2,3};
  SerdataKeyOrderMutable s_m{1,2,3};

  hash_test(s_k_o, s_k_o_h);
  hash_test(s_k_i, s_k_ih_h);
  hash_test(s_k_h, s_k_ih_h);
  hash_test(s_a, s_k_ih_h);
  hash_test(s_m, s_k_ih_h);
}

TEST_F(KeyHash, string_types)
{
  SerdataKeyString s{1, "abc"};
  SerdataKeyStringBounded s_b{1, "abc"};
  SerdataKeyStringAppendable s_a{1, "abc"};
  SerdataKeyStringBoundedAppendable s_b_a{1, "abc"};

  hash_test(s, s_s);
  hash_test(s_b, s_s);
  hash_test(s_a, s_s);
  hash_test(s_b_a, s_s);
}

TEST_F(KeyHash, nested_types)
{
  SerdataOuter s_o{SerdataInner1{1,2},SerdataInner1{3,4},SerdataInner2{5,6}};
  SerdataKeyOrderFinalNestedMutable f_n_m{4,5,SerdataKeyOrderMutable{1,2,3}};
  SerdataKeyOrderAppendableNestedMutable a_n_m{4,5,SerdataKeyOrderMutable{1,2,3}};
  SerdataKeyOrderMutableNestedMutable m_n_m{4,5,SerdataKeyOrderMutable{1,2,3}};
  SerdataKeyOrderMutableNestedAppendable m_n_a{4,5,SerdataKeyOrderAppendable{1,2,3}};
  SerdataKeyOrderMutableNestedFinal m_n_f{4,5,SerdataKeyOrder{1,2,3}};


  hash_test(s_o, s_o_h);
  hash_test(f_n_m, s_n_h_2);
  hash_test(a_n_m, s_n_h_2);
  hash_test(m_n_m, s_n_h_2);
  hash_test(m_n_a, s_n_h_2);
  hash_test(m_n_f, s_n_h_1);
}

TEST_F(KeyHash, arrays)
{
  SerdataKeyArr s_k_a{{0x1,0x2,0x3,0x4,0x5,0x6,0x7,0x8,0x9,0x10,0x11,0x12}};
  SerdataKeyArrStrBounded s_k_a_s_b{{"ab", "cd"}};

  hash_test(s_k_a, s_k_a_h);
  hash_test(s_k_a_s_b, s_k_a_s_b_h);
}

TEST_F(KeyHash, nested_types_implicit)
{
  SerdataKeyNestedFinalImplicit n_f_i{
    SerdataKeyNestedFinalImplicitSubtype{1,2,SerdataKeyOrder{3,4,5}},
    SerdataKeyNestedFinalImplicitSubtype{6,7,SerdataKeyOrder{8,9,10}},
    7};

  SerdataKeyNestedFinalImplicit2 n_f_i_2{
    SerdataKeyNestedFinalImplicit2Subtype1{ /*a*/
      SerdataKeyNestedFinalImplicit2Subtype2{ /*a.c*/
        SerdataKeyNestedFinalImplicit2Subtype3{1,2}, /*a.c.e*/
        SerdataKeyNestedFinalImplicit2Subtype3{3,4} /*a.c.f*/},
      SerdataKeyNestedFinalImplicit2Subtype2{ /*a.d*/
        SerdataKeyNestedFinalImplicit2Subtype3{5,6}, /*a.d.e*/
        SerdataKeyNestedFinalImplicit2Subtype3{7,8} /*a.d.f*/}},
    SerdataKeyNestedFinalImplicit2Subtype1{ /*b*/
      SerdataKeyNestedFinalImplicit2Subtype2{ /*b.c*/
        SerdataKeyNestedFinalImplicit2Subtype3{9,10}, /*b.c.e*/
        SerdataKeyNestedFinalImplicit2Subtype3{11,12} /*b.c.f*/},
      SerdataKeyNestedFinalImplicit2Subtype2{ /*b.d*/
        SerdataKeyNestedFinalImplicit2Subtype3{13,14}, /*b.d.e*/
        SerdataKeyNestedFinalImplicit2Subtype3{15,16} /*b.d.f*/}}};

  SerdataKeyNestedMutableImplicit n_m_i{
    SerdataKeyNestedMutableImplicitSubtype{ /*d*/
      1, /*d.x*/
      2, /*d.y*/
      SerdataKeyOrder{3, 4, 5}}, /*d.z*/
    SerdataKeyNestedMutableImplicitSubtype{ /*e*/
      6, /*e.x*/
      7, /*e.y*/
      SerdataKeyOrder{8, 9, 10}}, /*e.z*/
      11 /*f*/};

  hash_test(n_f_i, n_f_i_h);
  hash_test(n_f_i_2, n_f_i_2_h);
  hash_test(n_m_i, n_m_i_h);
}

// These bytes are the big-endian, sorted key serialization, without an
// encapsulation header or struct/member headers. Keep them explicit to catch bugs.
template<typename T, typename S>
static void verify_wire_keyhash(const T& sample, bytes key_bytes, bool hashed)
{
  ddscxx_sertype<T, S> st;
  ddsi_keyhash_t basic_key;
  to_key(sample, basic_key);

  ddsi_keyhash_t expected{};
  if (hashed)
    org::eclipse::cyclonedds::topic::complex_key(key_bytes, expected);
  else
    std::copy(key_bytes.begin(), key_bytes.end(), expected.value);

  for (auto kind : {SDK_DATA, SDK_KEY}) {
    auto sd = serdata_from_sample<T, S>(&st, kind, &sample);
    ASSERT_NE(sd, nullptr);
    // Instance identity must remain independent of the wire representation.
    auto typed_sd = static_cast<ddscxx_serdata<T>*>(sd);
    EXPECT_EQ(0, memcmp(typed_sd->key().value, basic_key.value, 16));
    const auto instance_hash = sd->hash;

    for (bool force_md5 : {false, true}) {
      ddsi_keyhash_t actual, expected_hash = expected;
      if (force_md5 && !hashed)
        org::eclipse::cyclonedds::topic::complex_key(key_bytes, expected_hash);
      // Use the installed callback to check that it selects the writer's stream.
      sd->ops->get_keyhash(sd, &actual, force_md5);
      EXPECT_EQ(0, memcmp(actual.value, expected_hash.value, 16));
    }
    EXPECT_EQ(sd->hash, instance_hash);
    EXPECT_EQ(0, memcmp(typed_sd->key().value, basic_key.value, 16));
    ddsi_serdata_unref(sd);
  }
  ddsrt_atomic_st32(&st.flags_refc, 0);
  ddsi_sertype_fini(&st);
}

TEST_F(KeyHash, wire_keyhash_alignment)
{
  const SerdataKeyAlignment sample{1, 3, 4};
  const bytes cdr_key{
    1, 0, 0, 0, 0, 0, 0, 0, // a and 8-byte alignment
    0, 0, 0, 0, 0, 0, 0, 3, // b
    4};                       // c
  const bytes xcdr2_key{
    1, 0, 0, 0,             // a and 4-byte alignment
    0, 0, 0, 0, 0, 0, 0, 3, // b
    4};                     // c
  verify_wire_keyhash<SerdataKeyAlignment, basic_cdr_stream>(sample, cdr_key, true);
  verify_wire_keyhash<SerdataKeyAlignment, xcdr_v1_stream>(sample, cdr_key, true);
  verify_wire_keyhash<SerdataKeyAlignment, xcdr_v2_stream>(sample, xcdr2_key, false);
}

TEST_F(KeyHash, wire_keyhash_appendable)
{
  const SerdataKeyStringBoundedAppendable sample{1, "abc"};
  const bytes key{
    1, 0, 0, 0,           // a and padding
    0, 0, 0, 4, 'a', 'b', 'c', 0}; // b
  verify_wire_keyhash<SerdataKeyStringBoundedAppendable, xcdr_v2_stream>(sample, key, false);
}

TEST_F(KeyHash, wire_keyhash_mutable)
{
  const SerdataKeyOrderMutable sample{1, 2, 3};
  const bytes key{
    0, 0, 0, 0, 0, 0, 0, 3, // c
    1};                   // a
  verify_wire_keyhash<SerdataKeyOrderMutable, xcdr_v2_stream>(sample, key, false);
}

TEST_F(KeyHash, wire_keyhash_mutable_xcdr1)
{
  const SerdataKeyOrderMutable sample{1, 2, 3};
  const bytes key{
    0, 0, 0, 0, 0, 0, 0, 3, // c
    1};                   // a
  // The final key-holder is nine bytes: mutable parameter headers do not
  // contribute to its maximum size or force MD5. This matches Fast DDS.
  verify_wire_keyhash<SerdataKeyOrderMutable, xcdr_v1_stream>(sample, key, false);
}

TEST_F(KeyHash, wire_keyhash_nested_mutable_xcdr1)
{
  const SerdataKeyOrderFinalNestedMutable sample{4, 5, SerdataKeyOrderMutable{1, 2, 3}};
  const bytes key{
    0, 0, 0, 0, 0, 0, 0, 3, // z.c
    1,                       // z.a
    4};                      // x
  // Nested mutable types also become final in the key-holder.
  verify_wire_keyhash<SerdataKeyOrderFinalNestedMutable, xcdr_v1_stream>(sample, key, false);
  verify_wire_keyhash<SerdataKeyOrderFinalNestedMutable, xcdr_v2_stream>(sample, key, false);
}

TEST_F(KeyHash, wire_keyhash_nested)
{
  const SerdataKeyOrderMutableNestedFinal sample{4, 5, SerdataKeyOrder{1, 2, 3}};
  const bytes key{
    1, 0, 0, 0,              // z.a and padding
    0, 0, 0, 0, 0, 0, 0, 3, // z.c
    4};                      // x
  verify_wire_keyhash<SerdataKeyOrderMutableNestedFinal, xcdr_v2_stream>(sample, key, false);
}

TEST_F(KeyHash, wire_keyhash_collection)
{
  const SerdataKeyArrStrBounded sample{{"ab", "cd"}};
  const bytes key{
    0, 0, 0, 15,             // Collection DHEADER is retained
    0, 0, 0, 3, 'a', 'b', 0,
    0,                       // padding
    0, 0, 0, 3, 'c', 'd', 0};
  verify_wire_keyhash<SerdataKeyArrStrBounded, xcdr_v2_stream>(sample, key, true);
}

TEST_F(KeyHash, wire_keyhash_unbounded)
{
  const SerdataKeyStringAppendable sample{1, "abc"};
  const bytes key{
    1, 0, 0, 0,
    0, 0, 0, 4, 'a', 'b', 'c', 0};
  // Hashing depends on the maximum key size, even for a short string value.
  verify_wire_keyhash<SerdataKeyStringAppendable, xcdr_v2_stream>(sample, key, true);
}
