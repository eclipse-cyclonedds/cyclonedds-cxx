// Copyright(c) 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "dds/dds.hpp"
#include <dds/core/External.hpp>
#include <thread>
#include "ExternalModels.hpp"

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

TEST(External, reading_writing)
{
  using external_testing::external_struct;

  dds::domain::DomainParticipant participant(org::eclipse::cyclonedds::domain::default_id());

  dds::topic::Topic<external_struct> topic(participant, "external_testing__external_struct");

  dds::pub::Publisher publisher(participant);
  dds::pub::DataWriter<external_struct> writer(publisher, topic);

  dds::sub::Subscriber subscriber(participant);
  dds::sub::DataReader<external_struct> reader(subscriber, topic);

  auto start = std::chrono::high_resolution_clock::now(); //wait for a maximum interval
  bool timedout = false;
  while (!timedout) {
    //wait for reader & writer to have found eachother
    if (writer.publication_matched_status().current_count() > 0 ||
        reader.subscription_matched_status().current_count() > 0)
      break;

    if ((std::chrono::high_resolution_clock::now()-start) > std::chrono::duration<double>(1))
      timedout = true;

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  ASSERT_FALSE(timedout);

  //create message
  external_struct msg;
  //attempt to write with external fields not set
  EXPECT_THROW(writer.write(msg);, dds::core::InvalidArgumentError);

  msg.external_long(new int(123456));
  msg.external_string(std::make_shared<std::string>("i am external string"));
  auto ptr = std::make_shared<std::vector<double> >(std::vector<double>({123.456,234.567,345.678}));
  msg.external_sequence_double(ptr);

  //check that ptr has refcount = 2 (ptr + msg)
  EXPECT_EQ(ptr.use_count(),2);

  //write message
  ASSERT_NO_THROW(writer.write(msg));

  //read message
  size_t samplesread = 0;
  start = std::chrono::high_resolution_clock::now(); //wait for a maximum interval
  while (!samplesread && !timedout) {
    auto samples = reader.take();

    //check for received messages
    for (const auto &sample:samples) {
      if (!sample.info().valid())
        continue;

      const external_struct &data = sample.data();
      EXPECT_NE(msg, data);
      //different pointers
      EXPECT_NE(msg.external_long(), data.external_long());
      EXPECT_NE(msg.external_string(), data.external_string());
      EXPECT_NE(msg.external_sequence_double(), data.external_sequence_double());
      //same values
      EXPECT_EQ(*msg.external_long(), *data.external_long());
      EXPECT_EQ(*msg.external_string(), *data.external_string());
      EXPECT_EQ(*msg.external_sequence_double(), *data.external_sequence_double());

      samplesread++;
    }

    if ((std::chrono::high_resolution_clock::now()-start) > std::chrono::duration<double>(1))
      timedout = true;

    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }

  EXPECT_FALSE(timedout);
  EXPECT_EQ(samplesread,1);
}
