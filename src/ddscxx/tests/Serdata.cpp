/*
 * Copyright(c) 2006 to 2021 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include <gtest/gtest.h>

#include "dds/dds.hpp"
#include "Serialization.hpp"
#include <org/eclipse/cyclonedds/topic/datatopic.hpp>

using namespace org::eclipse::cyclonedds::core::cdr;

/**
 * Fixture for the tests
 */
class Serdata : public ::testing::Test
{
public:

    ddsi_sertype *m_st = nullptr;

    Serdata()
    {
    }

    void SetUp()
    {
        m_st = org::eclipse::cyclonedds::topic::TopicTraits<Endianness::Msg>::getSerType();
        sd = new ddscxx_serdata<Endianness::Msg>(m_st,SDK_DATA);
        ASSERT_NE(sd,nullptr);

        sd->resize(12);
        ptr = static_cast<unsigned char*>(sd->data());

        ptr[4] = 0x10;
        ptr[5] = 0x19;
        ptr[6] = 0x24;
        ptr[7] = 0x00;
        ptr[8] = 0x01;
        ptr[9] = 0x02;
        ptr[10] = 0x03;
        ptr[11] = 0x04;
    }

    void TearDown()
    {
        delete sd;
        sd = nullptr;
        ddsrt_atomic_st32(&m_st->flags_refc, 0);
        ddsi_sertype_fini(m_st);
        delete m_st;
    }

    Endianness::Msg& getMsg(endianness end)
    {
        ptr[1] = (end == endianness::little_endian ? 0x01 : 0x00);

        return *(sd->getT());
    }

private:

    ddscxx_serdata<Endianness::Msg> *sd = nullptr;
    unsigned char *ptr = nullptr;
};

/*
 * Checking that the cdr stream correctly aligns between single- and multibyte primitives
 */
TEST_F(Serdata, alignment)
{
    Endianness::Msg msg({16,25,36},65535);

    basic_cdr_stream str(endianness::little_endian);
    std::vector<unsigned char> vec(8,0x0);
    str.set_buffer(vec.data());

    write(str,msg);

    ASSERT_EQ(vec, std::vector<unsigned char>({16,25,36,0,255,255,0,0}));
}

/*
 * Checking that ddscxx_serdata uses the correct endianness flags for deserialization
 * of a big endian stream
 */
TEST_F(Serdata, deserialization_big_endianness)
{
    auto msg = getMsg(endianness::big_endian);

    if (native_endianness() == endianness::big_endian)
      ASSERT_EQ(msg.l(), 0x04030201);
    else
      ASSERT_EQ(msg.l(), 0x01020304);
}

/*
 * Checking that ddscxx_serdata uses the correct endianness flags for deserialization
 * of a little endian stream
 */
TEST_F(Serdata, deserialization_little_endianness)
{
    auto msg = getMsg(endianness::little_endian);

    if (native_endianness() == endianness::little_endian)
      ASSERT_EQ(msg.l(), 0x04030201);
    else
      ASSERT_EQ(msg.l(), 0x01020304);
}

/*
 * Checking that ddscxx_serdata uses the correct endianness flags for serialization
 */
TEST_F(Serdata, serialization_big_endianness)
{
    Endianness::Msg msg({0x4,0x5,0x6},
                     4278255360); //(0xFF00FF00 LE)

    auto d = static_cast<const ddscxx_serdata<Endianness::Msg>*>(serdata_from_sample<Endianness::Msg>(
        m_st,
        SDK_DATA,
        static_cast<const void*>(&msg)));

    ASSERT_NE(d, nullptr);

    if (native_endianness() == endianness::big_endian)
      ASSERT_EQ(0,memcmp(d->data(),std::vector<unsigned char>({0x0,0x0,0x0,0x0,0x4,0x5,0x6,0x0,0xFF,0x00,0xFF,0x00}).data(), 12));
    else
      ASSERT_EQ(0,memcmp(d->data(),std::vector<unsigned char>({0x0,0x1,0x0,0x0,0x4,0x5,0x6,0x0,0x00,0xFF,0x00,0xFF}).data(), 12));

    delete d;
}
