
#include "dds/dds.hpp"
#include <gtest/gtest.h>
#include <vector>

#if DDSCXX_USE_BOOST
#include <boost/optional.hpp>
#include <boost/none.hpp>
#define DDSCXX_STD_IMPL boost
#define DDSCXX_STD_IMPL_NULLOPT boost::none
#else
#define DDSCXX_STD_IMPL std
#define DDSCXX_STD_IMPL_NULLOPT std::nullopt
#endif

typedef std::vector<unsigned char> bytes;

using namespace org::eclipse::cyclonedds::core::cdr;

template<typename T, typename S>
void VerifyWrite_Impl(const T& in, const bytes &out, S &stream, key_mode _key, bool write_success = true, bool compare_success = true)
{
  bytes buffer;
  bool result = move(stream, in, _key);
  ASSERT_EQ(result, write_success);

  if (!result)
    return;

  buffer.resize(stream.position());
  stream.set_buffer(buffer.data(), buffer.size());
  ASSERT_TRUE(write(stream, in, _key));

  result = (buffer == out);
  ASSERT_EQ(result, compare_success);
}

template<typename T, typename S>
void VerifyRead_Impl(const bytes &in, const T& out, S &stream, key_mode _key, bool read_success = true, bool compare_success = true)
{
  bytes incopy(in);
  T buffer;
  stream.set_buffer(incopy.data(), incopy.size());
  bool result = read(stream, buffer, _key);
  ASSERT_EQ(result, read_success);

  if (!result)
    return;

  if (_key == key_mode::sorted || _key == key_mode::unsorted)
    result = (buffer.c() == out.c());
  else
    result = (buffer == out);

  ASSERT_EQ(result, compare_success);
}

template<typename T, typename S>
void VerifyReadOneDeeper_Impl(const bytes &in, const T& out, S &stream, key_mode _key)
{
  bytes incopy(in);
  T buffer;

  stream.set_buffer(incopy.data(), incopy.size());
  ASSERT_TRUE(read(stream, buffer, _key));

  if (_key == key_mode::sorted || _key == key_mode::unsorted) {
    ASSERT_EQ(buffer.c().size(), out.c().size());
    for (size_t i = 0; i < buffer.c().size() && i < out.c().size(); i++)
      ASSERT_EQ(buffer.c()[i].c(), out.c()[i].c());
  } else {
    ASSERT_EQ(buffer, out);
  }
}

#define VerifyRead(_bytes, _struct, _streamer, _key, _read_success, _compare_success)\
{\
_streamer streamer_1(endianness::big_endian);\
VerifyRead_Impl(_bytes, _struct, streamer_1, _key, _read_success, _compare_success);\
}

#define VerifyReadOneDeeper(_bytes, _struct, _streamer, _key)\
{\
_streamer streamer_1(endianness::big_endian);\
VerifyReadOneDeeper_Impl(_bytes, _struct, streamer_1, _key);\
}

#define read_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
{\
VerifyRead(normal_bytes, test_struct, streamer, key_mode::not_key, true, true);\
VerifyRead(key_bytes, key_struct, streamer, key_mode::unsorted, true, true);\
}

#define read_test_fail(test_struct, key_struct, streamer)\
{\
VerifyRead(bytes(256, 0x0), test_struct, streamer, key_mode::not_key, false, false);\
VerifyRead(bytes(256, 0x0), key_struct, streamer, key_mode::unsorted, false, false);\
}

#define read_deeper_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
{\
VerifyRead(normal_bytes, test_struct, streamer, key_mode::not_key, true, true);\
VerifyReadOneDeeper(key_bytes, key_struct, streamer, key_mode::unsorted);\
}

#define VerifyWrite(_struct, _bytes, _streamer, _key, _write_success, _compare_success)\
{\
_streamer streamer_1(endianness::big_endian);\
VerifyWrite_Impl(_struct, _bytes, streamer_1, _key, _write_success, _compare_success);\
}

#define write_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
{\
VerifyWrite(test_struct, normal_bytes, streamer, key_mode::not_key, true, true);\
VerifyWrite(key_struct, key_bytes, streamer, key_mode::unsorted, true, true);\
}

#define write_test_fail(test_struct, key_struct, streamer)\
{\
VerifyWrite(test_struct, bytes(256, 0x0), streamer, key_mode::not_key, false, false);\
VerifyWrite(key_struct, bytes(256, 0x0), streamer, key_mode::unsorted, false, false);\
}

#define readwrite_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
read_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
write_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)

#define readwrite_test_fail(test_struct, key_struct, streamer)\
read_test_fail(test_struct, key_struct, streamer)\
write_test_fail(test_struct, key_struct, streamer)

#define readwrite_deeper_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
read_deeper_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)\
write_test(test_struct, key_struct, normal_bytes, key_bytes, streamer)

#define stream_test(test_struct, cdr_normal_bytes, key_bytes)\
readwrite_test(test_struct, test_struct, cdr_normal_bytes, key_bytes, basic_cdr_stream)\
readwrite_test(test_struct, test_struct, cdr_normal_bytes, key_bytes, xcdr_v1_stream)\
readwrite_test(test_struct, test_struct, cdr_normal_bytes, key_bytes, xcdr_v2_stream)

#define stream_test_union(test_struct, key_struct, cdr_normal_bytes, key_bytes)\
readwrite_test(test_struct, key_struct, cdr_normal_bytes, key_bytes, basic_cdr_stream)\
readwrite_test(test_struct, key_struct, cdr_normal_bytes, key_bytes, xcdr_v1_stream)\
readwrite_test(test_struct, key_struct, cdr_normal_bytes, key_bytes, xcdr_v2_stream)
