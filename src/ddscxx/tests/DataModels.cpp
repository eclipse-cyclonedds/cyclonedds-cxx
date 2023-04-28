// Copyright(c) 2022 ZettaScale Technologies
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

/**
 * Fixture for the DataModels tests
 */
class DataModels : public ::testing::Test
{
public:
    DataModels() { }

    void SetUp() { }

    void TearDown() { }
};

/*testing implicit defaults of structs*/
TEST_F(DataModels, implicit_defaults)
{
  using namespace DataModels_testing;

  implicit_defaults_struct ds;

  EXPECT_EQ(ds.l(), 0);
  EXPECT_EQ(ds.d(), 0);
  EXPECT_EQ(ds.c(), '\0');
  EXPECT_EQ(ds.s(), "");
  EXPECT_EQ(ds.e(), test_enum::e_2);
  EXPECT_EQ(ds.b(), 0);
}

/*testing explicit defaults of structs*/
TEST_F(DataModels, explicit_defaults)
{
  using namespace DataModels_testing;

  explicit_defaults_struct ds;

  EXPECT_EQ(ds.l(), 123);
  EXPECT_EQ(ds.d(), .456);
  EXPECT_EQ(ds.c(), 'a');
  EXPECT_EQ(ds.s(), "def");
  /*
  this test will fail until we support enum values as annotation parameters
  EXPECT_EQ(ds.e(), test_enum::e_1);
  */
  EXPECT_EQ(ds.b(), 5);
  EXPECT_EQ(ds.b(), f_0|f_2);
}
