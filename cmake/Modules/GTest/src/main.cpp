/*
 * Copyright(c) 2006 to 2020 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include "gtest/gtest.h"

// Valid is true if at least 1 test was run (or if no tests exist)
class HasRunAnyTests : public ::testing::EmptyTestEventListener {
public:
  bool valid;
  HasRunAnyTests() {}

protected:
  virtual void OnTestProgramEnd(const ::testing::UnitTest& unit_test) {
    valid = unit_test.total_test_case_count() == 0 || unit_test.test_case_to_run_count();
  }
};

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);

  // Add handler to check if any test has run
  HasRunAnyTests* hasRunAnyTests = new HasRunAnyTests();
  ::testing::TestEventListeners& listeners =
        ::testing::UnitTest::GetInstance()->listeners();
  listeners.Append(hasRunAnyTests);

  int result = RUN_ALL_TESTS();

  // Require at lease 1 test
  int ret = hasRunAnyTests->valid ? result : 1;

  return ret;
}
