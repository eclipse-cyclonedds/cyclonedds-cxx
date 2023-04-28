// Copyright(c) 2006 to 2021 ZettaScale Technology and others
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
#include "ExtendedTypesModels.hpp"

using namespace org::eclipse::cyclonedds::core::cdr;
using namespace ExtendedTypes_testing;

typedef std::vector<char> bytes;

/**
 * Fixture for the ExtendedTypes tests
 */
class ExtendedTypes : public ::testing::Test
{
public:
  ExtendedTypes()
  {
  }

  void SetUp() { }

  void TearDown() { }

  template<class MSGIN, class MSGOUT, class S>
  void validate(
    const MSGIN& in,
    bool exp_write_result = true,
    bool exp_read_result = true,
    bool exp_contents = true)
  {
    S str;
    bool move_result = move(str, in, false);
    ASSERT_EQ(move_result, exp_write_result);

    if (!move_result)
      return;

    bytes buffer(str.position(), 0x0);
    str.set_buffer(buffer.data(), buffer.size());

    bool write_result = write(str, in, false);
    ASSERT_EQ(write_result, exp_write_result);

    if (!write_result)
      return;

    MSGOUT out;
    str.reset();
    bool read_result = read(str, out, false);
    ASSERT_EQ(read_result, exp_read_result);

    if (!read_result || !exp_read_result)
      return;

    ASSERT_EQ(exp_contents, (in.c() == out.c()));
  }

  template<class MSGIN, class MSGOUT, class S>
  void validate_deeper(
    const MSGIN& in,
    bool exp_write_result = true,
    bool exp_read_result = true,
    bool exp_contents = true)
  {
    S str;
    bool move_result = move(str, in, false);
    ASSERT_EQ(move_result, exp_write_result);

    if (!move_result)
      return;

    bytes buffer(str.position(), 0x0);
    str.set_buffer(buffer.data(), buffer.size());

    bool write_result = write(str, in, false);
    ASSERT_EQ(write_result, exp_write_result);

    if (!write_result)
      return;

    MSGOUT out;
    str.reset();
    bool read_result = read(str, out, false);
    ASSERT_EQ(read_result, exp_read_result);

    if (!read_result || !exp_read_result)
      return;

    ASSERT_EQ(in.c().size(), out.c().size());
    auto it1 = in.c().begin();
    auto it2 = out.c().begin();
    bool contents = true;
    while (it1 != in.c().end() && it2 != out.c().end()) {
      contents &= (it1->c() == it2->c());
      it1++;
      it2++;
    }
    ASSERT_EQ(exp_contents, contents);
  }
};

TEST_F(ExtendedTypes, final)
{
  finalstruct_smaller smaller('c');
  finalstruct_larger larger('c', 'd');

  /* write smaller, read larger, read should fail,
     as there are not enough bytes to populate larger*/
  validate<finalstruct_smaller, finalstruct_larger, basic_cdr_stream>(smaller, true, false);
  validate<finalstruct_smaller, finalstruct_larger, xcdr_v1_stream>(smaller, true, false);
  validate<finalstruct_smaller, finalstruct_larger, xcdr_v2_stream>(smaller, true, false);

  /* write smaller, read larger, this should be okay,
     as the excess bytes containing 'd' will be ignored*/
  validate<finalstruct_larger, finalstruct_smaller, basic_cdr_stream>(larger);
  validate<finalstruct_larger, finalstruct_smaller, xcdr_v1_stream>(larger);
  validate<finalstruct_larger, finalstruct_smaller, xcdr_v2_stream>(larger);
}

TEST_F(ExtendedTypes, appendable)
{
  appendablestruct_smaller smaller('c');
  appendablestruct_larger larger('c', 'd');

  /* read and write will fail for both smaller to larger and reverse,
     as basic cdr serialization does not support appendable structs*/
  validate<appendablestruct_smaller, appendablestruct_larger, basic_cdr_stream>(smaller, false, false);
  validate<appendablestruct_larger, appendablestruct_smaller, basic_cdr_stream>(larger, false, false);

  /* write smaller, read larger, this should be okay,
     as the read should see that there are not enough
     bytes to populate the member 'd'*/
  validate<appendablestruct_smaller, appendablestruct_larger, xcdr_v1_stream>(smaller);
  validate<appendablestruct_smaller, appendablestruct_larger, xcdr_v2_stream>(smaller);

  /* write smaller, read larger, this should be okay,
     as the excess bytes containing 'd' will be ignored*/
  validate<appendablestruct_larger, appendablestruct_smaller, xcdr_v1_stream>(larger);
  validate<appendablestruct_larger, appendablestruct_smaller, xcdr_v2_stream>(larger);
}

TEST_F(ExtendedTypes, mutable)
{
  mutablestruct_a a('a', 'c', 'e');
  mutablestruct_b b('b', 'c', 'd');

  /* read and write will fail for both smaller to larger and reverse,
     as basic cdr serialization does not support appendable structs*/
  validate<mutablestruct_a, mutablestruct_b, basic_cdr_stream>(a, false, false);
  validate<mutablestruct_b, mutablestruct_a, basic_cdr_stream>(b, false, false);

  /* write a, read b, should be okay, as member
     c is present in both representations*/
  validate<mutablestruct_a, mutablestruct_b, xcdr_v1_stream>(a);
  validate<mutablestruct_a, mutablestruct_b, xcdr_v2_stream>(a);

  /* write b, read a, should be okay, as members
     c is present in both representations*/
  validate<mutablestruct_b, mutablestruct_a, xcdr_v1_stream>(b);
  validate<mutablestruct_b, mutablestruct_a, xcdr_v2_stream>(b);
}

TEST_F(ExtendedTypes, sequences_final)
{
  sequences_of_final_smaller smaller({finalstruct_smaller('a'), finalstruct_smaller('b')});
  sequences_of_final_larger larger({finalstruct_larger('a', 'z'), finalstruct_larger('b', 'y')});

  //this should read and write correctly, but the end result is corrupt
  validate_deeper<sequences_of_final_larger, sequences_of_final_smaller, basic_cdr_stream>(larger, true, true, false);
  //this should fail reading
  validate_deeper<sequences_of_final_smaller, sequences_of_final_larger, basic_cdr_stream>(smaller, true, false, false);

  //this should read and write correctly, but the end result is corrupt
  validate_deeper<sequences_of_final_larger, sequences_of_final_smaller, xcdr_v1_stream>(larger, true, true, false);
  //this should fail reading
  validate_deeper<sequences_of_final_smaller, sequences_of_final_larger, xcdr_v1_stream>(smaller, true, false, false);

  validate_deeper<sequences_of_final_larger, sequences_of_final_smaller, xcdr_v2_stream>(larger, true, true, false);
  validate_deeper<sequences_of_final_smaller, sequences_of_final_larger, xcdr_v2_stream>(smaller, true, false, false);
}
