// Copyright(c) 2006 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

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
        m_st = org::eclipse::cyclonedds::topic::TopicTraits<Endianness::Msg>::getSerType(DDS_DATA_REPRESENTATION_FLAG_XCDR1);
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

    auto d = static_cast<const ddscxx_serdata<Endianness::Msg>*>(serdata_from_sample<Endianness::Msg, basic_cdr_stream>(
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
DDSRT_WARNING_GNUC_OFF(maybe-uninitialized)
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
DDSRT_WARNING_GNUC_ON(maybe-uninitialized)

using kh_t = std::array<unsigned char, 16>;

template<typename T>
static void test_keyhash(const T& sample, const kh_t& expected, const kh_t& expected_md5)
{
    auto st = org::eclipse::cyclonedds::topic::TopicTraits<T>::getSerType(DDS_DATA_REPRESENTATION_FLAG_XCDR1);
    auto sd = serdata_from_sample<T, org::eclipse::cyclonedds::core::cdr::basic_cdr_stream>(st, SDK_DATA, &sample);
    struct ddsi_keyhash khraw, khraw_md5;
    serdata_get_keyhash<T>(sd, &khraw, false);
    serdata_get_keyhash<T>(sd, &khraw_md5, true);
    kh_t kh, kh_md5;
    std::copy(std::begin(khraw.value), std::end(khraw.value), std::begin(kh));
    std::copy(std::begin(khraw_md5.value), std::end(khraw_md5.value), std::begin(kh_md5));
    ASSERT_EQ(kh, expected);
    ASSERT_EQ(kh_md5, expected_md5);
    delete static_cast<ddscxx_serdata<T> *>(sd);
    //hacky and ugly, but we cannot call the sertype_free function, as the C type is referenced nowhere
    dds_free(st->type_name);
    delete static_cast<ddscxx_sertype<T,org::eclipse::cyclonedds::core::cdr::basic_cdr_stream>*>(st);
}

TEST_F(Serdata, keyhash_nokey)
{
    // perl -e 'print pack("N*",0,0,0,0)' | md5 | sed -e 's/\(..\)/0x\1,/g' -e 's/,$//'
    using T = Keyhash::NoKey;
    const T v{0xabcdef01};
    const kh_t kh{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
    const kh_t kh_md5 = kh;
    test_keyhash<T>(v, kh, kh_md5);
}

TEST_F(Serdata, keyhash_smallkey)
{
    // perl -e 'print pack("N*",0x12345678,0,0,0)' | md5 | sed -e 's/\(..\)/0x\1,/g' -e 's/,$//'
    using T = Keyhash::SmallKey;
    const T v{0x12345678, 0xabcdef01};
    const kh_t kh{0x12,0x34,0x56,0x78,0,0,0,0,0,0,0,0,0,0,0,0};
    const kh_t kh_md5{0x85,0x47,0xc6,0x54,0x16,0x3c,0x12,0x43,0x85,0x14,0x91,0x8a,0x1b,0xa8,0x7f,0xeb};
    test_keyhash<T>(v, kh, kh_md5);
}

TEST_F(Serdata, keyhash_largekey)
{
    // perl -e 'print pack("N*",1,2,3,4,5)' | md5 | sed -e 's/\(..\)/0x\1,/g' -e 's/,$//'
    using T = Keyhash::LargeKey;
    const T v{{1,2,3,4,5},0xabcdef01};
    const kh_t kh{0x43,0x21,0xf7,0x28,0x8e,0x52,0x1a,0xa6,0x2a,0xee,0x27,0x45,0xf3,0xf8,0xd9,0x2b};
    const kh_t kh_md5 = kh;
    test_keyhash<T>(v, kh, kh_md5);
}

TEST_F(Serdata, keyhash_stringkey)
{
    // perl -e 'print pack("N/Z*","Ick sie boven uut mijnen throne")' | md5 | sed -e 's/\(..\)/0x\1,/g' -e 's/,$//'
    using T = Keyhash::StringKey;
    const T v{"Ick sie boven uut mijnen throne",0xabcdef01};
    const kh_t kh{0x6f,0x63,0xff,0xe7,0x56,0x29,0x00,0x9b,0x3c,0xbe,0xfa,0xa0,0x6f,0xb3,0x22,0x43};
    const kh_t kh_md5 = kh;
    test_keyhash<T>(v, kh, kh_md5);
}

TEST_F(Serdata, keyhash_bstringkey)
{
    // perl -e '$v=pack("N/Z*x@16","Elckerlijc");print $v' | md5 | sed -e 's/\(..\)/0x\1,/g' -e 's/,$//'
    using T = Keyhash::BStringKey;
    const T v{"Elckerlijc",0xabcdef01};
    const kh_t kh{0x00,0x00,0x00,0x0b,0x45,0x6c,0x63,0x6b,0x65,0x72,0x6c,0x69,0x6a,0x63,0x00,0x00};
    const kh_t kh_md5{0x39, 0x59, 0x90, 0xf2, 0x0f, 0xd2, 0x53, 0x5a, 0x54, 0x07, 0xec, 0xa5, 0x65, 0xcc, 0xd2, 0xd7};
    test_keyhash<T>(v, kh, kh_md5);
}
