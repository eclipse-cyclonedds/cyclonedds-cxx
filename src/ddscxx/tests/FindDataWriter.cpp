// Copyright(c) 2006 to 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <iostream>
#include <gtest/gtest.h>

#include "dds/dds.hpp"
#include "Space.hpp"

#define TOPIC1_NAME    "findwriter_Type1"
#define TOPIC2_NAME    "findwriter_Type2"

/**
 * Fixture for the Topic finding and discovering tests
 */
class FindDataWriter : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant = dds::core::null;

    dds::pub::Publisher publisher = dds::core::null;

    dds::topic::Topic<Space::Type1> topic1 = dds::core::null;
    dds::topic::Topic<Space::Type2> topic2 = dds::core::null;

    dds::pub::DataWriter<Space::Type1> writer1A = dds::core::null;
    dds::pub::DataWriter<Space::Type1> writer1B = dds::core::null;
    dds::pub::DataWriter<Space::Type2> writer2  = dds::core::null;

    void SetUp() {
        this->participant = dds::domain::DomainParticipant(0);
        ASSERT_NE(this->participant, dds::core::null);
        this->publisher = dds::pub::Publisher(this->participant);
        ASSERT_NE(this->publisher, dds::core::null);
    }

    void CreateWriters() {
        this->topic1 = dds::topic::Topic<Space::Type1>(this->participant, TOPIC1_NAME);
        this->topic2 = dds::topic::Topic<Space::Type2>(this->participant, TOPIC2_NAME);
        ASSERT_NE(this->topic1, dds::core::null);
        ASSERT_NE(this->topic2, dds::core::null);

        this->writer1A = dds::pub::DataWriter<Space::Type1>(this->publisher, this->topic1);
        ASSERT_NE(this->writer1A, dds::core::null);
        this->writer1B = dds::pub::DataWriter<Space::Type1>(this->publisher, this->topic1);
        ASSERT_NE(this->writer1B, dds::core::null);
        this->writer2  = dds::pub::DataWriter<Space::Type2>(this->publisher, this->topic2);
        ASSERT_NE(this->writer2,  dds::core::null);
    }
};

/**
 * Tests
 */

TEST_F(FindDataWriter, BinIterator_find_with_empty)
{
    std::vector<dds::pub::DataWriter<Space::Type1> > found;
    uint32_t cnt;

    cnt = dds::pub::find<
                dds::pub::DataWriter<Space::Type1>,
                std::back_insert_iterator<std::vector<dds::pub::DataWriter<Space::Type1> > > >(
            this->publisher,
            TOPIC1_NAME,
            std::back_inserter<std::vector<dds::pub::DataWriter<Space::Type1> > >(found));

    ASSERT_EQ(cnt, 0);
    ASSERT_EQ(found.size(), 0);
}

TEST_F(FindDataWriter, BinIterator_find_with_other_type)
{
    std::vector<dds::pub::DataWriter<Space::Type2> > found;
    uint32_t cnt;

    this->CreateWriters();

    cnt = dds::pub::find<
                dds::pub::DataWriter<Space::Type2>,
                std::back_insert_iterator<std::vector<dds::pub::DataWriter<Space::Type2> > > >(
            this->publisher,
            TOPIC1_NAME,  // <- not matching Space::Type2
            std::back_inserter<std::vector<dds::pub::DataWriter<Space::Type2> > >(found));

    ASSERT_EQ(cnt, 0);
    ASSERT_EQ(found.size(), 0);
}

TEST_F(FindDataWriter, BinIterator_find_nonexisting)
{
    std::vector<dds::pub::DataWriter<Space::Type1> > found;
    uint32_t cnt;

    this->CreateWriters();

    cnt = dds::pub::find<
                dds::pub::DataWriter<Space::Type1>,
                std::back_insert_iterator<std::vector<dds::pub::DataWriter<Space::Type1> > > >(
            this->publisher,
            std::string("non-existing"),
            std::back_inserter<std::vector<dds::pub::DataWriter<Space::Type1> > >(found));

    ASSERT_EQ(cnt, 0);
    ASSERT_EQ(found.size(), 0);
}

TEST_F(FindDataWriter, BinIterator_find)
{
    std::vector<dds::pub::DataWriter<Space::Type1> > found;
    uint32_t cnt;

    /* 2 Type1 writer */
    this->CreateWriters();

    cnt = dds::pub::find<
                dds::pub::DataWriter<Space::Type1>,
                std::back_insert_iterator<std::vector<dds::pub::DataWriter<Space::Type1> > > >(
            this->publisher,
            TOPIC1_NAME,
            std::back_inserter<std::vector<dds::pub::DataWriter<Space::Type1> > >(found));

#if 1
    /* Currently, the find() will only return 1 writer. */
    ASSERT_EQ(cnt, 1);
    ASSERT_EQ(found.size(), 1);
    ASSERT_TRUE((found[0] == this->writer1A) || (found[0] == this->writer1B));
#else
    ASSERT_EQ(cnt, 2);
    ASSERT_EQ(found.size(), 2);
    ASSERT_TRUE((found[0] == this->writer1A) || (found[0] == this->writer1B));
    ASSERT_TRUE((found[1] == this->writer1A) || (found[1] == this->writer1B));
#endif
}

TEST_F(FindDataWriter, BinIterator_find_any)
{
    std::vector<dds::pub::AnyDataWriter> found;
    uint32_t cnt;

    /* 1 Type2 writers */
    this->CreateWriters();

    cnt = dds::pub::find<
                dds::pub::AnyDataWriter,
                std::back_insert_iterator<std::vector<dds::pub::AnyDataWriter> > >(
            this->publisher,
            TOPIC2_NAME,
            std::back_inserter<std::vector<dds::pub::AnyDataWriter> >(found));

    ASSERT_EQ(cnt, 1);
    ASSERT_EQ(found.size(), 1);
    ASSERT_TRUE(found[0] == this->writer2);
}

TEST_F(FindDataWriter, FwdIterator_find_with_empty)
{
    std::vector<dds::pub::DataWriter<Space::Type1> > found(5, dds::core::null);
    std::insert_iterator<std::vector<dds::pub::DataWriter<Space::Type1> > > iter(found, found.begin());
    uint32_t cnt;

    cnt = dds::pub::find<
                dds::pub::DataWriter<Space::Type1>,
                std::vector<dds::pub::DataWriter<Space::Type1> >::iterator>(
            this->publisher,
            TOPIC1_NAME,
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 0);
}

TEST_F(FindDataWriter, FwdIterator_find_with_other_type)
{
    std::vector<dds::pub::DataWriter<Space::Type2> > found(5, dds::core::null);
    uint32_t cnt;

    this->CreateWriters();

    cnt = dds::pub::find<
                dds::pub::DataWriter<Space::Type2>,
                std::vector<dds::pub::DataWriter<Space::Type2> >::iterator>(
            this->publisher,
            TOPIC1_NAME,  // <- not matching Space::Type2
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 0);
}

TEST_F(FindDataWriter, FwdIterator_find_nonexisting)
{
    std::vector<dds::pub::DataWriter<Space::Type1> > found(5, dds::core::null);
    uint32_t cnt;

    this->CreateWriters();

    cnt = dds::pub::find<
                dds::pub::DataWriter<Space::Type1>,
                std::vector<dds::pub::DataWriter<Space::Type1> >::iterator>(
            this->publisher,
            std::string("non-existing"),
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 0);
}

TEST_F(FindDataWriter, FwdIterator_find)
{
    std::vector<dds::pub::DataWriter<Space::Type1> > found(5, dds::core::null);
    uint32_t cnt;

    /* 2 Type1 writer */
    this->CreateWriters();

    cnt = dds::pub::find<
                dds::pub::DataWriter<Space::Type1>,
                std::vector<dds::pub::DataWriter<Space::Type1> >::iterator>(
            this->publisher,
            TOPIC1_NAME,
            found.begin(),
            static_cast<uint32_t>(found.size()));

#if 1
    /* Currently, the find() will only return 1 writer. */
    ASSERT_EQ(cnt, 1);
    ASSERT_TRUE((found[0] == this->writer1A) || (found[0] == this->writer1B));
#else
    ASSERT_EQ(cnt, 2);
    ASSERT_TRUE((found[0] == this->writer1A) || (found[0] == this->writer1B));
    ASSERT_TRUE((found[1] == this->writer1A) || (found[1] == this->writer1B));
#endif
}

TEST_F(FindDataWriter, FwdIterator_find_any)
{
    std::vector<dds::pub::AnyDataWriter> found(5, dds::core::null);
    uint32_t cnt;

    /* 1 Type2 writers */
    this->CreateWriters();

    cnt = dds::pub::find<
                dds::pub::AnyDataWriter,
                std::vector<dds::pub::AnyDataWriter>::iterator>(
            this->publisher,
            TOPIC2_NAME,
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 1);
    ASSERT_TRUE(found[0] == this->writer2);
}

TEST_F(FindDataWriter, ignore)
{
    this->CreateWriters();
    ASSERT_THROW({
        dds::pub::ignore(this->participant, dds::core::InstanceHandle());
    }, dds::core::UnsupportedError);
}
