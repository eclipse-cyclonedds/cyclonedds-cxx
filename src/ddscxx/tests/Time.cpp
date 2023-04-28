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

/* Various values to check what happens when comparing Times that have
 * larger seconds, but lower nanoseconds or vice versa or other. */
static dds::core::Time time_3s5ns_base(3, 5);
static dds::core::Time time_2s4ns(2, 4);
static dds::core::Time time_2s5ns(2, 5);
static dds::core::Time time_2s6ns(2, 6);
static dds::core::Time time_3s4ns(3, 4);
static dds::core::Time time_3s5ns(3, 5);
static dds::core::Time time_3s6ns(3, 6);
static dds::core::Time time_4s4ns(4, 4);
static dds::core::Time time_4s5ns(4, 5);
static dds::core::Time time_4s6ns(4, 6);

TEST(Time, plus)
{
    dds::core::Time t;
    dds::core::Time t1(10, 10);
    dds::core::Duration d1(10, 10);

    t = t1 + d1;
    ASSERT_EQ(t.sec(), 20);
    ASSERT_EQ(t.nanosec(), 20);

    t = d1 + t;
    ASSERT_EQ(t.sec(), 30);
    ASSERT_EQ(t.nanosec(), 30);
}

TEST(Time, plus_is)
{
    dds::core::Time t(10, 10);
    dds::core::Duration d1(10, 10);
    dds::core::Duration d2(1, 999999999);

    t += d1;
    ASSERT_EQ(t.sec(), 20);
    ASSERT_EQ(t.nanosec(), 20);

    t += d2;
    ASSERT_EQ(t.sec(), 22);
    ASSERT_EQ(t.nanosec(), 19);
}

TEST(Time, minus)
{
    dds::core::Time t;

    t = dds::core::Time(10, 10) - dds::core::Duration(10, 10);
    ASSERT_EQ(t.sec(), 0);
    ASSERT_EQ(t.nanosec(), 0);

    t = dds::core::Time(12, 12) - dds::core::Duration(10, 10);
    ASSERT_EQ(t.sec(), 2);
    ASSERT_EQ(t.nanosec(), 2);

    t = dds::core::Time(11, 9) - dds::core::Duration(10, 10);
    ASSERT_EQ(t.sec(), 0);
    ASSERT_EQ(t.nanosec(), 999999999);

    ASSERT_THROW({
        /* Negative nsecs. */
        t = dds::core::Time(10, 9) - dds::core::Duration(10, 10);
    }, dds::core::Error);

    ASSERT_THROW({
        /* Negative nsecs and secs. */
        t = dds::core::Time(9, 9) - dds::core::Duration(10, 10);
    }, dds::core::Error);

    ASSERT_THROW({
        /* Negative secs. */
        t = dds::core::Time(9, 12) - dds::core::Duration(10, 10);
    }, dds::core::Error);
}

TEST(Time, minus_is)
{
    dds::core::Time t;

    t = dds::core::Time(10, 10);
    t -= dds::core::Duration(10, 10);
    ASSERT_EQ(t.sec(), 0);
    ASSERT_EQ(t.nanosec(), 0);

    t = dds::core::Time(12, 12);
    t -= dds::core::Duration(10, 10);
    ASSERT_EQ(t.sec(), 2);
    ASSERT_EQ(t.nanosec(), 2);

    t = dds::core::Time(11, 9);
    t -= dds::core::Duration(10, 10);
    ASSERT_EQ(t.sec(), 0);
    ASSERT_EQ(t.nanosec(), 999999999);

    ASSERT_THROW({
        /* Negative nsecs. */
        t = dds::core::Time(10, 9);
        t -= dds::core::Duration(10, 10);
    }, dds::core::Error);

    ASSERT_THROW({
        /* Negative nsecs and secs. */
        t = dds::core::Time(9, 9);
        t -= dds::core::Duration(10, 10);
    }, dds::core::Error);

    ASSERT_THROW({
        /* Negative secs. */
        t = dds::core::Time(9, 12);
        t -= dds::core::Duration(10, 10);
    }, dds::core::Error);
}

TEST(Time, nanosec)
{
    dds::core::Time t;

    t.nanosec(100000000);
    ASSERT_EQ(t.nanosec(), 100000000);
    ASSERT_EQ(t.to_millisecs(), 100);

    ASSERT_THROW({
        /* Invalid nanosecs. */
        t.nanosec(1000000000);
    }, dds::core::Error);
}

TEST(Time, microsec)
{
    dds::core::Time t;
    int64_t micros = dds::core::Time(10,1000).to_microsecs();
    ASSERT_EQ(micros, 10000001);
    t = dds::core::Time::from_microsecs(micros);
    ASSERT_EQ(t.sec(), 10);
    ASSERT_EQ(t.nanosec(), 1000);
}

TEST(Time, millisec)
{
    dds::core::Time t;
    int64_t millis = dds::core::Time(10,1000000).to_millisecs();
    ASSERT_EQ(millis, 10001);
    t = dds::core::Time::from_millisecs(millis);
    ASSERT_EQ(t.sec(), 10);
    ASSERT_EQ(t.nanosec(), 1000000);
}

TEST(Time, sec)
{
    dds::core::Time t;
    double secs = dds::core::Time(10,500000000).to_secs();
    ASSERT_EQ(secs, 10.5);
    t = dds::core::Time::from_secs(secs);
    ASSERT_EQ(t.sec(), 10);
    ASSERT_EQ(t.nanosec(), 500000000);

    ASSERT_THROW({
        /* Negative secs. */
        t.sec(-2);
    }, dds::core::Error);
}

TEST(Time, greater)
{
    ASSERT_TRUE((time_2s4ns > time_3s5ns_base) == false);
    ASSERT_TRUE((time_2s5ns > time_3s5ns_base) == false);
    ASSERT_TRUE((time_2s6ns > time_3s5ns_base) == false);
    ASSERT_TRUE((time_3s4ns > time_3s5ns_base) == false);
    ASSERT_TRUE((time_3s5ns > time_3s5ns_base) == false);
    ASSERT_TRUE((time_3s6ns > time_3s5ns_base) == true);
    ASSERT_TRUE((time_4s4ns > time_3s5ns_base) == true);
    ASSERT_TRUE((time_4s5ns > time_3s5ns_base) == true);
    ASSERT_TRUE((time_4s6ns > time_3s5ns_base) == true);
}

TEST(Time, greater_then)
{
    ASSERT_TRUE((time_2s4ns >= time_3s5ns_base) == false);
    ASSERT_TRUE((time_2s5ns >= time_3s5ns_base) == false);
    ASSERT_TRUE((time_2s6ns >= time_3s5ns_base) == false);
    ASSERT_TRUE((time_3s4ns >= time_3s5ns_base) == false);
    ASSERT_TRUE((time_3s5ns >= time_3s5ns_base) == true);
    ASSERT_TRUE((time_3s6ns >= time_3s5ns_base) == true);
    ASSERT_TRUE((time_4s4ns >= time_3s5ns_base) == true);
    ASSERT_TRUE((time_4s5ns >= time_3s5ns_base) == true);
    ASSERT_TRUE((time_4s6ns >= time_3s5ns_base) == true);
}

TEST(Time, equals)
{
    ASSERT_TRUE((time_2s4ns == time_3s5ns_base) == false);
    ASSERT_TRUE((time_2s5ns == time_3s5ns_base) == false);
    ASSERT_TRUE((time_2s6ns == time_3s5ns_base) == false);
    ASSERT_TRUE((time_3s4ns == time_3s5ns_base) == false);
    ASSERT_TRUE((time_3s5ns == time_3s5ns_base) == true);
    ASSERT_TRUE((time_3s6ns == time_3s5ns_base) == false);
    ASSERT_TRUE((time_4s4ns == time_3s5ns_base) == false);
    ASSERT_TRUE((time_4s5ns == time_3s5ns_base) == false);
    ASSERT_TRUE((time_4s6ns == time_3s5ns_base) == false);
}

TEST(Time, not_equals)
{
    ASSERT_TRUE((time_2s4ns != time_3s5ns_base) == true);
    ASSERT_TRUE((time_2s5ns != time_3s5ns_base) == true);
    ASSERT_TRUE((time_2s6ns != time_3s5ns_base) == true);
    ASSERT_TRUE((time_3s4ns != time_3s5ns_base) == true);
    ASSERT_TRUE((time_3s5ns != time_3s5ns_base) == false);
    ASSERT_TRUE((time_3s6ns != time_3s5ns_base) == true);
    ASSERT_TRUE((time_4s4ns != time_3s5ns_base) == true);
    ASSERT_TRUE((time_4s5ns != time_3s5ns_base) == true);
    ASSERT_TRUE((time_4s6ns != time_3s5ns_base) == true);
}

TEST(Time, less_then)
{
    ASSERT_TRUE((time_2s4ns <= time_3s5ns_base) == true);
    ASSERT_TRUE((time_2s5ns <= time_3s5ns_base) == true);
    ASSERT_TRUE((time_2s6ns <= time_3s5ns_base) == true);
    ASSERT_TRUE((time_3s4ns <= time_3s5ns_base) == true);
    ASSERT_TRUE((time_3s5ns <= time_3s5ns_base) == true);
    ASSERT_TRUE((time_3s6ns <= time_3s5ns_base) == false);
    ASSERT_TRUE((time_4s4ns <= time_3s5ns_base) == false);
    ASSERT_TRUE((time_4s5ns <= time_3s5ns_base) == false);
    ASSERT_TRUE((time_4s6ns <= time_3s5ns_base) == false);
}

TEST(Time, less)
{
    ASSERT_TRUE((time_2s4ns < time_3s5ns_base) == true);
    ASSERT_TRUE((time_2s5ns < time_3s5ns_base) == true);
    ASSERT_TRUE((time_2s6ns < time_3s5ns_base) == true);
    ASSERT_TRUE((time_3s4ns < time_3s5ns_base) == true);
    ASSERT_TRUE((time_3s5ns < time_3s5ns_base) == false);
    ASSERT_TRUE((time_3s6ns < time_3s5ns_base) == false);
    ASSERT_TRUE((time_4s4ns < time_3s5ns_base) == false);
    ASSERT_TRUE((time_4s5ns < time_3s5ns_base) == false);
    ASSERT_TRUE((time_4s6ns < time_3s5ns_base) == false);
}

TEST(Time, compare)
{
    ASSERT_EQ(time_3s5ns_base.compare(time_2s4ns),  1);
    ASSERT_EQ(time_3s5ns_base.compare(time_2s5ns),  1);
    ASSERT_EQ(time_3s5ns_base.compare(time_2s6ns),  1);
    ASSERT_EQ(time_3s5ns_base.compare(time_3s4ns),  1);
    ASSERT_EQ(time_3s5ns_base.compare(time_3s5ns),  0);
    ASSERT_EQ(time_3s5ns_base.compare(time_3s6ns), -1);
    ASSERT_EQ(time_3s5ns_base.compare(time_4s4ns), -1);
    ASSERT_EQ(time_3s5ns_base.compare(time_4s5ns), -1);
    ASSERT_EQ(time_3s5ns_base.compare(time_4s6ns), -1);
}
