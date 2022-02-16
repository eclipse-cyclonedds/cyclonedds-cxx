/*
 * Copyright(c) 2022 ZettaScale Technology and others
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
#include <dds/core/External.hpp>

#include <gtest/gtest.h>

using dds::core::external;

TEST(External, constructing)
{
  external<int> ei1;
  EXPECT_EQ(ei1.get(), nullptr);
  EXPECT_FALSE(ei1);
  EXPECT_FALSE(ei1.is_locked());

  external<int> ei2(new int(123), true);
  EXPECT_NE(ei2.get(), nullptr);
  EXPECT_TRUE(ei2);
  EXPECT_TRUE(ei2.is_locked());
  EXPECT_EQ(*ei2, 123);

  external<int> ei3(std::make_shared<int>(456));
  EXPECT_NE(ei3.get(),nullptr);
  EXPECT_TRUE(ei3);
  EXPECT_FALSE(ei3.is_locked());
  EXPECT_EQ(*ei3, 456);
}

TEST(External, assigning)
{
  external<int> ei1, ei2(new int(123), true), ei3;

  ASSERT_NO_THROW(ei1 = ei2;);  //deep copy of ei2

  EXPECT_NE(ei1.get(),nullptr);
  EXPECT_FALSE(ei1.is_locked());
  EXPECT_EQ(*ei1, *ei2);
  EXPECT_NE(ei1, ei2);

  ASSERT_NO_THROW(ei3 = ei1;);  //shallow copy of ei1
  EXPECT_EQ(ei1, ei3);

  EXPECT_THROW(ei2 = ei1;, dds::core::InvalidDataError);
}

TEST(External, accessing)
{
  external<int> ei1;

  EXPECT_EQ(ei1.get(), nullptr);
  EXPECT_EQ(ei1.get_shared_ptr(), nullptr);
  EXPECT_EQ(ei1.operator->(), nullptr);
  EXPECT_THROW(*ei1 = 123;, dds::core::NullReferenceError);

  ei1 = new int(456);

  EXPECT_NE(ei1.get(), nullptr);
  EXPECT_NE(ei1.get_shared_ptr(), nullptr);
  EXPECT_NE(ei1.operator->(), nullptr);
  EXPECT_EQ(*ei1, 456);
  EXPECT_NO_THROW(*ei1 = 567;);
  EXPECT_EQ(*ei1, 567);
}

TEST(External, locking)
{
  external<int> ei1;

  EXPECT_FALSE(ei1.is_locked());

  EXPECT_THROW(ei1.lock();, dds::core::NullReferenceError);

  ei1 = new int(123);

  EXPECT_NO_THROW(ei1.lock(););

  EXPECT_TRUE(ei1.is_locked());
}
