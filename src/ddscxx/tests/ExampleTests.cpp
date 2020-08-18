/*
 * Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include <chrono>
#include <thread>
#include "dds/ddscxx/test.h"

DDSCXX_TEST(ddscxx_example_test, assert)
{
  ASSERT_TRUE(true);
}

DDSCXX_TEST(ddscxx_example_test, custom_message)
{
  ASSERT_TRUE(true) << "Should be true";
}

DDSCXX_TEST(ddscxx_example_test, time_out, .timeout = 1.0)
{
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
}

DDSCXX_TEST(ddscxx_example_test, exception)
{
  ASSERT_THROW({
    throw std::exception();
  }, std::exception);
}

DDSCXX_TEST(ddscxx_example_test, no_throw)
{
  ASSERT_NO_THROW({
    return;
  });
}

bool pred1(int n) { return (n % 2) == 0; };
bool pred2(int a, int b) { return (a + b) == 15; };

DDSCXX_TEST(ddscxx_example_test, assert_pred)
{
  ASSERT_PRED1(pred1, 8);
  ASSERT_PRED1(pred1, 22);
  ASSERT_PRED2(pred2, 5, 10);
}

class ddscxx_example_test_fixture : public ::testing::Test
{
protected:
  static int test_suite_val;
  int test_val;

  ddscxx_example_test_fixture() : test_val(0) { }

  static void SetUpTestCase() { test_suite_val = 10; }
  static void TearDownTestCase() { }
  void SetUp() { test_val++; }
  void TearDown() { }
};
int ddscxx_example_test_fixture::test_suite_val;

DDSCXX_TEST_F(ddscxx_example_test_fixture, test1)
{
  ASSERT_EQ(test_suite_val, 10);
  ASSERT_EQ(test_val, 1);
}

DDSCXX_TEST_F(ddscxx_example_test_fixture, test2)
{
  ASSERT_EQ(test_suite_val, 10);
  ASSERT_EQ(test_val, 1);
}

class ddscxx_example_test_params : public ::testing::TestWithParam<int> {
public:
  int val;

  ddscxx_example_test_params() : val(0) { }

  void setVal(int v) { val = v; }
  bool check1() { return val % 2 == 0 && val <= 10; }
  bool check2() { return val >= 0; }
};

DDSCXX_TEST_P(ddscxx_example_test_params, check_1) {
  setVal(GetParam());
  EXPECT_TRUE(check1());
}

DDSCXX_TEST_P(ddscxx_example_test_params, check_2) {
  setVal(GetParam());
  EXPECT_TRUE(check2());
}

INSTANTIATE_TEST_CASE_P(increasing, ddscxx_example_test_params,
                                ::testing::Range(0, 10, 2));

INSTANTIATE_TEST_CASE_P(decreasing, ddscxx_example_test_params,
                                ::testing::Values(10, 8, 6, 4, 2));
