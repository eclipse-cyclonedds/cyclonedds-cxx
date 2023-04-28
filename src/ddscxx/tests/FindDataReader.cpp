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

#define TOPIC1_NAME    "findreader_Type1"
#define TOPIC2_NAME    "findreader_Type2"
#define TOPIC3_NAME    "findreader_Type3"

/**
 * Fixture for the Topic finding and discovering tests
 */
class FindDataReader : public ::testing::Test
{
public:
    dds::domain::DomainParticipant participant = dds::core::null;

    dds::sub::Subscriber subscriber = dds::core::null;

    dds::topic::Topic<Space::Type1> topic1 = dds::core::null;
    dds::topic::Topic<Space::Type2> topic2 = dds::core::null;

    dds::sub::DataReader<Space::Type1> reader1A = dds::core::null;
    dds::sub::DataReader<Space::Type1> reader1B = dds::core::null;
    dds::sub::DataReader<Space::Type2> reader2  = dds::core::null;

    void SetUp() {
        this->participant = dds::domain::DomainParticipant(0);
        ASSERT_NE(this->participant, dds::core::null);
        this->subscriber = dds::sub::Subscriber(this->participant);
        ASSERT_NE(this->subscriber, dds::core::null);
        this->topic1 = dds::topic::Topic<Space::Type1>(this->participant, TOPIC1_NAME);
        this->topic2 = dds::topic::Topic<Space::Type2>(this->participant, TOPIC2_NAME);
        ASSERT_NE(this->topic1, dds::core::null);
        ASSERT_NE(this->topic2, dds::core::null);
    }

    void CreateReaders() {
        this->reader1A = dds::sub::DataReader<Space::Type1>(this->subscriber, this->topic1);
        ASSERT_NE(this->reader1A, dds::core::null);
        this->reader1B = dds::sub::DataReader<Space::Type1>(this->subscriber, this->topic1);
        ASSERT_NE(this->reader1B, dds::core::null);
        this->reader2  = dds::sub::DataReader<Space::Type2>(this->subscriber, this->topic2);
        ASSERT_NE(this->reader2,  dds::core::null);
    }
};

/**
 * Tests
 */

TEST_F(FindDataReader, BinIterator_tname_find_with_empty)
{
    std::vector<dds::sub::DataReader<Space::Type1> > found;
    uint32_t cnt;

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type1>,
                std::back_insert_iterator<std::vector<dds::sub::DataReader<Space::Type1> > > >(
            this->subscriber,
            std::string(TOPIC1_NAME),
            std::back_inserter<std::vector<dds::sub::DataReader<Space::Type1> > >(found));

    ASSERT_EQ(cnt, 0);
    ASSERT_EQ(found.size(), 0);
}

TEST_F(FindDataReader, BinIterator_tname_find_with_other_type)
{
    std::vector<dds::sub::DataReader<Space::Type2> > found;
    uint32_t cnt;

    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type2>,
                std::back_insert_iterator<std::vector<dds::sub::DataReader<Space::Type2> > > >(
            this->subscriber,
            std::string(TOPIC1_NAME),  // <- not matching Space::Type2
            std::back_inserter<std::vector<dds::sub::DataReader<Space::Type2> > >(found));

    ASSERT_EQ(cnt, 0);
    ASSERT_EQ(found.size(), 0);
}

TEST_F(FindDataReader, BinIterator_tname_find_nonexisting)
{
    std::vector<dds::sub::DataReader<Space::Type1> > found;
    uint32_t cnt;

    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type1>,
                std::back_insert_iterator<std::vector<dds::sub::DataReader<Space::Type1> > > >(
            this->subscriber,
            std::string("non-existing"),
            std::back_inserter<std::vector<dds::sub::DataReader<Space::Type1> > >(found));

    ASSERT_EQ(cnt, 0);
    ASSERT_EQ(found.size(), 0);
}

TEST_F(FindDataReader, BinIterator_tname_find)
{
    std::vector<dds::sub::DataReader<Space::Type1> > found;
    uint32_t cnt;

    /* 2 Type1 readers */
    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type1>,
                std::back_insert_iterator<std::vector<dds::sub::DataReader<Space::Type1> > > >(
            this->subscriber,
            std::string(TOPIC1_NAME),
            std::back_inserter<std::vector<dds::sub::DataReader<Space::Type1> > >(found));

    ASSERT_EQ(cnt, 2);
    ASSERT_EQ(found.size(), 2);
    ASSERT_TRUE((found[0] == this->reader1A) || (found[0] == this->reader1B));
    ASSERT_TRUE((found[1] == this->reader1A) || (found[1] == this->reader1B));
}

TEST_F(FindDataReader, BinIterator_tname_find_any)
{
    std::vector<dds::sub::AnyDataReader> found;
    uint32_t cnt;

    /* 1 Type2 reader */
    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::AnyDataReader,
                std::back_insert_iterator<std::vector<dds::sub::AnyDataReader> > >(
            this->subscriber,
            std::string(TOPIC2_NAME),
            std::back_inserter<std::vector<dds::sub::AnyDataReader> >(found));

    ASSERT_EQ(cnt, 1);
    ASSERT_EQ(found.size(), 1);
    ASSERT_TRUE(found[0] == this->reader2);
}

TEST_F(FindDataReader, BinIterator_tdesc_find_with_empty)
{
    std::vector<dds::sub::DataReader<Space::Type1> > found;
    uint32_t cnt;

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type1>,
                std::back_insert_iterator<std::vector<dds::sub::DataReader<Space::Type1> > > >(
            this->subscriber,
            this->topic1,
            std::back_inserter<std::vector<dds::sub::DataReader<Space::Type1> > >(found));

    ASSERT_EQ(cnt, 0);
    ASSERT_EQ(found.size(), 0);
}

TEST_F(FindDataReader, BinIterator_tdesc_find_with_other_type)
{
    std::vector<dds::sub::DataReader<Space::Type2> > found;
    uint32_t cnt;

    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type2>,
                std::back_insert_iterator<std::vector<dds::sub::DataReader<Space::Type2> > > >(
            this->subscriber,
            this->topic1,  // <- not matching Space::Type2
            std::back_inserter<std::vector<dds::sub::DataReader<Space::Type2> > >(found));

    ASSERT_EQ(cnt, 0);
    ASSERT_EQ(found.size(), 0);
}

TEST_F(FindDataReader, BinIterator_tdesc_find)
{
    std::vector<dds::sub::DataReader<Space::Type1> > found;
    uint32_t cnt;

    /* 2 Type1 readers */
    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type1>,
                std::back_insert_iterator<std::vector<dds::sub::DataReader<Space::Type1> > > >(
            this->subscriber,
            this->topic1,
            std::back_inserter<std::vector<dds::sub::DataReader<Space::Type1> > >(found));

    ASSERT_EQ(cnt, 2);
    ASSERT_EQ(found.size(), 2);
    ASSERT_TRUE((found[0] == this->reader1A) || (found[0] == this->reader1B));
    ASSERT_TRUE((found[1] == this->reader1A) || (found[1] == this->reader1B));
}

TEST_F(FindDataReader, BinIterator_tdesc_find_any)
{
    std::vector<dds::sub::AnyDataReader> found;
    uint32_t cnt;

    /* 1 Type2 reader */
    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::AnyDataReader,
                std::back_insert_iterator<std::vector<dds::sub::AnyDataReader> > >(
            this->subscriber,
            this->topic2,
            std::back_inserter<std::vector<dds::sub::AnyDataReader> >(found));

    ASSERT_EQ(cnt, 1);
    ASSERT_EQ(found.size(), 1);
    ASSERT_TRUE(found[0] == this->reader2);
}

TEST_F(FindDataReader, FwdIterator_tname_find_with_empty)
{
    std::vector<dds::sub::DataReader<Space::Type1> > found(5, dds::core::null);
    uint32_t cnt;

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type1>,
                std::vector<dds::sub::DataReader<Space::Type1> >::iterator>(
            this->subscriber,
            std::string(TOPIC1_NAME),
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 0);
}

TEST_F(FindDataReader, FwdIterator_tname_find_with_other_type)
{
    std::vector<dds::sub::DataReader<Space::Type2> > found(5, dds::core::null);
    uint32_t cnt;

    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type2>,
                std::vector<dds::sub::DataReader<Space::Type2> >::iterator>(
            this->subscriber,
            std::string(TOPIC1_NAME),  // <- not matching Space::Type2
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 0);
}

TEST_F(FindDataReader, FwdIterator_tname_find_nonexisting)
{
    std::vector<dds::sub::DataReader<Space::Type1> > found(5, dds::core::null);
    uint32_t cnt;

    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type1>,
                std::vector<dds::sub::DataReader<Space::Type1> >::iterator>(
            this->subscriber,
            std::string("non-existing"),
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 0);
}

TEST_F(FindDataReader, FwdIterator_tname_find)
{
    std::vector<dds::sub::DataReader<Space::Type1> > found(5, dds::core::null);
    uint32_t cnt;

    /* 2 Type1 readers */
    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type1>,
                std::vector<dds::sub::DataReader<Space::Type1> >::iterator>(
            this->subscriber,
            std::string(TOPIC1_NAME),
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 2);
    ASSERT_TRUE((found[0] == this->reader1A) || (found[0] == this->reader1B));
    ASSERT_TRUE((found[1] == this->reader1A) || (found[1] == this->reader1B));
}

TEST_F(FindDataReader, FwdIterator_tname_find_any)
{
    std::vector<dds::sub::AnyDataReader> found(5, dds::core::null);
    uint32_t cnt;

    /* 1 Type2 reader */
    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::AnyDataReader,
                std::vector<dds::sub::AnyDataReader>::iterator>(
            this->subscriber,
            std::string(TOPIC2_NAME),
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 1);
    ASSERT_TRUE(found[0] == this->reader2);
}

TEST_F(FindDataReader, FwdIterator_tdesc_find_with_empty)
{
    std::vector<dds::sub::DataReader<Space::Type1> > found(5, dds::core::null);
    uint32_t cnt;

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type1>,
                std::vector<dds::sub::DataReader<Space::Type1> >::iterator>(
            this->subscriber,
            this->topic1,
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 0);
}

TEST_F(FindDataReader, FwdIterator_tdesc_find_with_other_type)
{
    std::vector<dds::sub::DataReader<Space::Type2> > found(5, dds::core::null);
    uint32_t cnt;

    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type2>,
                std::vector<dds::sub::DataReader<Space::Type2> >::iterator>(
            this->subscriber,
            this->topic1,  // <- not matching Space::Type2
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 0);
}

TEST_F(FindDataReader, FwdIterator_tdesc_find)
{
    std::vector<dds::sub::DataReader<Space::Type1> > found(5, dds::core::null);
    uint32_t cnt;

    /* 2 Type1 readers */
    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::DataReader<Space::Type1>,
                std::vector<dds::sub::DataReader<Space::Type1> >::iterator>(
            this->subscriber,
            this->topic1,
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 2);
    ASSERT_TRUE((found[0] == this->reader1A) || (found[0] == this->reader1B));
    ASSERT_TRUE((found[1] == this->reader1A) || (found[1] == this->reader1B));
}

TEST_F(FindDataReader, FwdIterator_tdesc_find_any)
{
    std::vector<dds::sub::AnyDataReader> found(5, dds::core::null);
    uint32_t cnt;

    /* 1 Type2 reader */
    this->CreateReaders();

    cnt = dds::sub::find<
                dds::sub::AnyDataReader,
                std::vector<dds::sub::AnyDataReader>::iterator>(
            this->subscriber,
            this->topic2,
            found.begin(),
            static_cast<uint32_t>(found.size()));

    ASSERT_EQ(cnt, 1);
    ASSERT_TRUE(found[0] == this->reader2);
}

TEST_F(FindDataReader, ignore)
{
    this->CreateReaders();
    ASSERT_THROW({
        dds::sub::ignore(this->participant, dds::core::InstanceHandle());
    }, dds::core::UnsupportedError);
}
