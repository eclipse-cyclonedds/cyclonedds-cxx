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

/* Various values to check what happens when comparing Durations that
 * have larger seconds, but lower nanoseconds, vice versa or other. */
static dds::core::Duration dur_3s5ns_base(3, 5);
static dds::core::Duration dur_2s4ns(2, 4);
static dds::core::Duration dur_2s5ns(2, 5);
static dds::core::Duration dur_2s6ns(2, 6);
static dds::core::Duration dur_3s4ns(3, 4);
static dds::core::Duration dur_3s5ns(3, 5);
static dds::core::Duration dur_3s6ns(3, 6);
static dds::core::Duration dur_4s4ns(4, 4);
static dds::core::Duration dur_4s5ns(4, 5);
static dds::core::Duration dur_4s6ns(4, 6);

TEST(Duration, plus)
{
    dds::core::Duration d;
    dds::core::Duration d1(10, 10);
    dds::core::Duration d2(10, 10);

    d = d1 + d2;
    ASSERT_EQ(d.sec(), 20);
    ASSERT_EQ(d.nanosec(), 20);
}

TEST(Duration, plus_is)
{
    dds::core::Duration d(10, 10);
    dds::core::Duration d1(10, 10);
    dds::core::Duration d2(1, 999999999);

    d += d1;
    ASSERT_EQ(d.sec(), 20);
    ASSERT_EQ(d.nanosec(), 20);

    d += d2;
    ASSERT_EQ(d.sec(), 22);
    ASSERT_EQ(d.nanosec(), 19);
}

TEST(Duration, minus)
{
    dds::core::Duration d;

    d = dds::core::Duration(10, 10) - dds::core::Duration(10, 10);
    ASSERT_EQ(d.sec(), 0);
    ASSERT_EQ(d.nanosec(), 0);

    d = dds::core::Duration(12, 12) - dds::core::Duration(10, 10);
    ASSERT_EQ(d.sec(), 2);
    ASSERT_EQ(d.nanosec(), 2);

    d = dds::core::Duration(11, 9) - dds::core::Duration(10, 10);
    ASSERT_EQ(d.sec(), 0);
    ASSERT_EQ(d.nanosec(), 999999999);

    ASSERT_THROW({
        /* Negative nsecs. */
        d = dds::core::Duration(10, 9) - dds::core::Duration(10, 10);
    }, dds::core::Error);

    ASSERT_THROW({
        /* Negative nsecs and secs. */
        d = dds::core::Duration(9, 9) - dds::core::Duration(10, 10);
    }, dds::core::Error);

    ASSERT_THROW({
        /* Negative secs. */
        d = dds::core::Duration(9, 12) - dds::core::Duration(10, 10);
    }, dds::core::Error);
}

TEST(Duration, minus_is)
{
    dds::core::Duration d;

    d = dds::core::Duration(10, 10);
    d -= dds::core::Duration(10, 10);
    ASSERT_EQ(d.sec(), 0);
    ASSERT_EQ(d.nanosec(), 0);

    d = dds::core::Duration(12, 12);
    d -= dds::core::Duration(10, 10);
    ASSERT_EQ(d.sec(), 2);
    ASSERT_EQ(d.nanosec(), 2);

    d = dds::core::Duration(11, 9);
    d -= dds::core::Duration(10, 10);
    ASSERT_EQ(d.sec(), 0);
    ASSERT_EQ(d.nanosec(), 999999999);

    ASSERT_THROW({
        /* Negative nsecs. */
        d = dds::core::Duration(10, 9);
        d -= dds::core::Duration(10, 10);
    }, dds::core::Error);

    ASSERT_THROW({
        /* Negative nsecs and secs. */
        d = dds::core::Duration(9, 9);
        d -= dds::core::Duration(10, 10);
    }, dds::core::Error);

    ASSERT_THROW({
        /* Negative secs. */
        d = dds::core::Duration(9, 12);
        d -= dds::core::Duration(10, 10);
    }, dds::core::Error);
}

TEST(Duration, times)
{
    dds::core::Duration d;

    d = 2 * dds::core::Duration(10, 10);
    ASSERT_EQ(d.sec(), 20);
    ASSERT_EQ(d.nanosec(), 20);

    d = dds::core::Duration(1, 999999999) * 10;
    ASSERT_EQ(d.sec(), 19);
    ASSERT_EQ(d.nanosec(), 999999990);
}

TEST(Duration, times_is)
{
    dds::core::Duration d;

    d = dds::core::Duration(10, 10);
    d *= 2;
    ASSERT_EQ(d.sec(), 20);
    ASSERT_EQ(d.nanosec(), 20);

    d = dds::core::Duration(1, 999999999);
    d *= 10;
    ASSERT_EQ(d.sec(), 19);
    ASSERT_EQ(d.nanosec(), 999999990);
}

TEST(Duration, divide)
{
    dds::core::Duration d;

    /* Allow for the round up to microseconds. */
    d = dds::core::Duration(10, 100000000) / 2;
    ASSERT_EQ(d.sec(), 5);
    ASSERT_EQ(d.nanosec(),  50000000);

    ASSERT_THROW({
        d = dds::core::Duration(10, 100000000) / 0;
    }, dds::core::Error);
}

TEST(Duration, nanosec)
{
    dds::core::Duration d;

    d.nanosec(100000000);
    ASSERT_EQ(d.nanosec(), 100000000);
    ASSERT_EQ(d.to_millisecs(), 100);

    ASSERT_THROW({
        /* Invalid nanosecs. */
        d.nanosec(1000000000);
    }, dds::core::Error);
}

TEST(Duration, microsec)
{
    dds::core::Duration d;
    int64_t micros = dds::core::Duration(10,1000).to_microsecs();
    ASSERT_EQ(micros, 10000001);
    d = dds::core::Duration::from_microsecs(micros);
    ASSERT_EQ(d.sec(), 10);
    ASSERT_EQ(d.nanosec(), 1000);
}

TEST(Duration, millisec)
{
    dds::core::Duration d;
    int64_t millis = dds::core::Duration(10,1000000).to_millisecs();
    ASSERT_EQ(millis, 10001);
    d = dds::core::Duration::from_millisecs(millis);
    ASSERT_EQ(d.sec(), 10);
    ASSERT_EQ(d.nanosec(), 1000000);
}

TEST(Duration, sec)
{
    dds::core::Duration d;
    double secs = dds::core::Duration(10,500000000).to_secs();
    ASSERT_EQ(secs, 10.5);
    d = dds::core::Duration::from_secs(secs);
    ASSERT_EQ(d.sec(), 10);
    ASSERT_EQ(d.nanosec(), 500000000);
}

TEST(Duration, greater)
{
    ASSERT_TRUE((dur_2s4ns > dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_2s5ns > dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_2s6ns > dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_3s4ns > dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_3s5ns > dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_3s6ns > dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_4s4ns > dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_4s5ns > dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_4s6ns > dur_3s5ns_base) == true);
}

TEST(Duration, greater_then)
{
    ASSERT_TRUE((dur_2s4ns >= dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_2s5ns >= dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_2s6ns >= dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_3s4ns >= dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_3s5ns >= dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_3s6ns >= dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_4s4ns >= dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_4s5ns >= dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_4s6ns >= dur_3s5ns_base) == true);
}

TEST(Duration, equals)
{
    ASSERT_TRUE((dur_2s4ns == dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_2s5ns == dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_2s6ns == dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_3s4ns == dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_3s5ns == dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_3s6ns == dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_4s4ns == dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_4s5ns == dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_4s6ns == dur_3s5ns_base) == false);
}

TEST(Duration, not_equals)
{
    ASSERT_TRUE((dur_2s4ns != dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_2s5ns != dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_2s6ns != dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_3s4ns != dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_3s5ns != dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_3s6ns != dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_4s4ns != dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_4s5ns != dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_4s6ns != dur_3s5ns_base) == true);
}

TEST(Duration, less_then)
{
    ASSERT_TRUE((dur_2s4ns <= dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_2s5ns <= dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_2s6ns <= dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_3s4ns <= dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_3s5ns <= dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_3s6ns <= dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_4s4ns <= dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_4s5ns <= dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_4s6ns <= dur_3s5ns_base) == false);
}

TEST(Duration, less)
{
    ASSERT_TRUE((dur_2s4ns < dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_2s5ns < dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_2s6ns < dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_3s4ns < dur_3s5ns_base) == true);
    ASSERT_TRUE((dur_3s5ns < dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_3s6ns < dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_4s4ns < dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_4s5ns < dur_3s5ns_base) == false);
    ASSERT_TRUE((dur_4s6ns < dur_3s5ns_base) == false);
}

TEST(Duration, compare)
{
    ASSERT_EQ(dur_3s5ns_base.compare(dur_2s4ns),  1);
    ASSERT_EQ(dur_3s5ns_base.compare(dur_2s5ns),  1);
    ASSERT_EQ(dur_3s5ns_base.compare(dur_2s6ns),  1);
    ASSERT_EQ(dur_3s5ns_base.compare(dur_3s4ns),  1);
    ASSERT_EQ(dur_3s5ns_base.compare(dur_3s5ns),  0);
    ASSERT_EQ(dur_3s5ns_base.compare(dur_3s6ns), -1);
    ASSERT_EQ(dur_3s5ns_base.compare(dur_4s4ns), -1);
    ASSERT_EQ(dur_3s5ns_base.compare(dur_4s5ns), -1);
    ASSERT_EQ(dur_3s5ns_base.compare(dur_4s6ns), -1);
}
