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
          0x00, 0x00, 0x00, 0x01  /*o_3.i_2*/};

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

