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

    template<typename T>
    void validate(const T &msg, const std::vector<uint8_t> &exp_le, const std::vector<uint8_t> &exp_be) {
        validate_impl(msg, exp_le, endianness::little_endian);
        validate_impl(msg, exp_be, endianness::big_endian);
    }

private:

    template<typename T>
    void validate_impl(const T &msg, const std::vector<uint8_t> &exp, endianness end) {
        basic_cdr_stream str(end);

        move(str, msg, false);

        size_t sz = str.position();
        ASSERT_EQ(sz, exp.size());
        std::vector<uint8_t> buffer(sz, 0x0);
        str.set_buffer(buffer.data(), buffer.size());

        write(str, msg, false);

        ASSERT_EQ(buffer, exp);
    }

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
    str.set_buffer(vec.data(), vec.size());

    write(str, msg, false);

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

/*
 * Checking correct (non-)byteswapping serialization of sequence types.
 */
TEST_F(Serdata, serialization_sequences)
{
    std::vector<bool> b_seq({true,false,true,false});
    std::vector<char> c_seq({'a','b','c','d'});
    std::vector<int16_t> s_seq({1234,2345,3456,4567});
    std::vector<int32_t> l_seq({12345678,23456789,34567890});
    std::vector<int64_t> ll_seq({1234567823456789,2345678934567890});

    std::vector<uint8_t> le =
        {0x04, 0x00, 0x00, 0x00,/* b_seq.length */  0x01, 0x00, 0x01, 0x00, /* b_seq.data */
         0x04, 0x00, 0x00, 0x00,/* c_seq.length */   'a',  'b',  'c',  'd', /* c_seq.data */
         0x04, 0x00, 0x00, 0x00,/* s_seq.length */  0xD2, 0x04,  0x29, 0x09,  0x80, 0x0D,  0xD7, 0x11, /* s_seq.data */
         0x03, 0x00, 0x00, 0x00,/* l_seq.length */  0x4E, 0x61, 0xBC, 0x00,  0x15, 0xEC, 0x65, 0x01,  0xD2, 0x76, 0x0F, 0x02, /* l_seq */
         0x02, 0x00, 0x00, 0x00,/* ll_seq.length */  0x15, 0x7A, 0x91, 0x38, 0xD5, 0x62, 0x04, 0x00,  0xD2, 0xEB, 0xA6, 0xEF, 0x61, 0x55, 0x08, 0x00 /* ll_seq.data */},
        be =
        {0x00, 0x00, 0x00, 0x04,/* b_seq.length */  0x01, 0x00, 0x01, 0x00, /* b_seq.data */
         0x00, 0x00, 0x00, 0x04,/* c_seq.length */   'a',  'b',  'c',  'd', /* c_seq.data */
         0x00, 0x00, 0x00, 0x04,/* s_seq.length */  0x04, 0xD2,  0x09, 0x29,  0x0D, 0x80,  0x11, 0xD7, /* s_seq.data */
         0x00, 0x00, 0x00, 0x03,/* l_seq.length */  0x00, 0xBC, 0x61, 0x4E,  0x01, 0x65, 0xEC, 0x15,  0x02, 0x0F, 0x76, 0xD2, /* l_seq.data */
         0x00, 0x00, 0x00, 0x02,/* ll_seq.length */  0x00, 0x04, 0x62, 0xD5, 0x38, 0x91, 0x7A, 0x15,  0x00, 0x08, 0x55, 0x61, 0xEF, 0xA6, 0xEB, 0xD2 /* ll_seq.data */};

    Endianness::Seqs msg(b_seq, c_seq, s_seq, l_seq, ll_seq);

    validate(msg, le, be);
}

/*
 * Checking correct (non-)byteswapping serialization of enum types.
 */
TEST_F(Serdata, serialization_enums)
{
    typedef Endianness::test_enum test_enum;

    Endianness::Enums msg(
        test_enum::first,
        {test_enum::second, test_enum::third, test_enum::fourth, test_enum::fifth});

    std::vector<uint8_t> le1 =
        {0x01, 0x00, 0x00, 0x00,  /* e_1 */
         0x02, 0x00, 0x00, 0x00,  0x03, 0x00, 0x00, 0x00,  0x04, 0x00, 0x00, 0x00,  0x05, 0x00, 0x00, 0x00} /* e_arr */,
        be1 = {0x00, 0x00, 0x00, 0x01,  /* e_1 */
         0x00, 0x00, 0x00, 0x02,  0x00, 0x00, 0x00, 0x03,  0x00, 0x00, 0x00, 0x04,  0x00, 0x00, 0x00, 0x05} /* e_arr */;

    validate(msg, le1, be1);

    Endianness::SeqEnums msg2({test_enum::zeroth, test_enum::first, test_enum::second, test_enum::third, test_enum::fourth, test_enum::fifth});

    std::vector<uint8_t> le2 =
        {0x06, 0x00, 0x00, 0x00,  /* e_seq.length */
         0x00, 0x00, 0x00, 0x00,  0x01, 0x00, 0x00, 0x00,  0x02, 0x00, 0x00, 0x00,  0x03, 0x00, 0x00, 0x00,  0x04, 0x00, 0x00, 0x00,  0x05, 0x00, 0x00, 0x00 /* e_seq.data */},
        be2 =
        {0x00, 0x00, 0x00, 0x06,  /* e_seq.length */
         0x00, 0x00, 0x00, 0x00,  0x00, 0x00, 0x00, 0x01,  0x00, 0x00, 0x00, 0x02,  0x00, 0x00, 0x00, 0x03,  0x00, 0x00, 0x00, 0x04,  0x00, 0x00, 0x00, 0x05 /* e_seq.data */};

    validate(msg2, le2, be2);
}

/*
 * Checking correct (non-)byteswapping serialization of unions.
 */
TEST_F(Serdata, serialization_unions)
{
    Endianness::UnionStr Ustr;
    Ustr.u().s(1234,5678);

    std::vector<uint8_t> le =
        {0x2E, 0x16, 0x00, 0x00,  /* discriminator */
         0xD2, 0x04} /* s */,
        be = {0x00, 0x00, 0x16, 0x2E,  /* discriminator */
         0x04, 0xD2} /* s */;

    validate(Ustr, le, be);
}
