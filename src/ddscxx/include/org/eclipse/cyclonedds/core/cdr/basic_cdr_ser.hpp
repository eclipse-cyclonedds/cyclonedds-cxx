/*
 * Copyright(c) 2021 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef BASIC_CDR_SERIALIZATION_HPP_
#define BASIC_CDR_SERIALIZATION_HPP_

#include "cdr_stream.hpp"
#include <org/eclipse/cyclonedds/core/type_helpers.hpp>
#include <dds/core/Exception.hpp>
#include <array>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

/**
 * @brief
 * Implementation of the basic cdr stream.
 *
 * This type of cdr stream has a maximum alignment of 8 bytes.
 */
class basic_cdr_stream : public cdr_stream {
public:
  /**
   * @brief
   * Constructor.
   *
   * Basically a pass through for the cdr_stream base class.
   *
   * @param[in] ignore_faults Bitmask for ignoring faults, can be composed of bit fields from the serialization_status enumerator.
   */
  basic_cdr_stream(uint64_t ignore_faults = 0x0) : cdr_stream(8, ignore_faults) { ; }
};

/**
 * @brief
 * Primitive type stream manipulation functions.
 *
 * These are "endpoints" for write functions, since composit
 * (sequence/array/constructed type) functions will decay to these
 * calls.
 */

/**
 * @brief
 * Primitive type read function.
 *
 * Aligns the stream to the alignment of type T.
 * Reads the value from the current position of the stream str into
 * toread.
 * Moves the cursor of the stream by the size of T.
 * This function is only enabled for arithmetic types and enums.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read into.
 */
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void read(basic_cdr_stream &str, T& toread)
{
  if (str.abort_status())
    return;

  str.align(sizeof(T), false);

  toread = *static_cast<T*>(str.get_cursor());

  str.incr_position(sizeof(T));
}

/**
 * @brief
 * Primitive type read function, byteswap version.
 *
 * Same as read(), with an additional byteswap at the end.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read into.
 */
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void read_swapped(basic_cdr_stream &str, T& toread)
{
  read(str, toread);

  byte_swap(toread);
}

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void read_many(basic_cdr_stream &str, T *out, size_t N)
{
  if (str.abort_status())
    return;

  str.align(sizeof(T), false);

  T *in = static_cast<T*>(str.get_cursor());

  memcpy(out, in, sizeof(T)*N);

  str.incr_position(sizeof(T)*N);
}

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void read_many_swapped(basic_cdr_stream &str, T *out, size_t N)
{
  if (str.abort_status())
    return;

  str.align(sizeof(T), false);

  T *in = static_cast<T*>(str.get_cursor());
  for (size_t i = 0; i < N; i++, out++, in++) {
    *out = *in;
    byte_swap(*out);
  }

  str.incr_position(sizeof(T)*N);
}

/**
 * @brief
 * Primitive type write function.
 *
 * Aligns str to the type to be written.
 * Writes towrite to str.
 * Swaps bytes written to str if the endiannesses do not match up.
 * Moves the cursor of str by the size of towrite.
 * This function is only enabled for arithmetic types.
 *
 * @param[in, out] str The stream which is written to.
 * @param[in] towrite The variable to write.
 */
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void write(basic_cdr_stream& str, const T& towrite)
{
  if (str.abort_status())
    return;

  str.align(sizeof(T), true);

  auto out = static_cast<T*>(str.get_cursor());

  *out = towrite;

  str.incr_position(sizeof(T));
}

/**
 * @brief
 * Primitive type write function, byteswap version.
 *
 * Same as write(), with an additional byteswap at the end.
 *
 * @param[in, out] str The stream which is read from.
 * @param[in] towrite The variable to write.
 */
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void write_swapped(basic_cdr_stream& str, const T& towrite)
{
  if (str.abort_status())
    return;

  str.align(sizeof(T), true);

  auto out = static_cast<T*>(str.get_cursor());

  *out = towrite;

  byte_swap(*out);

  str.incr_position(sizeof(T));
}

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void write_many(basic_cdr_stream& str, const T* in, size_t N)
{
  if (str.abort_status())
    return;

  str.align(sizeof(T), true);

  T *out = static_cast<T*>(str.get_cursor());

  memcpy(out, in, sizeof(T)*N);

  str.incr_position(sizeof(T)*N);
}

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void write_many_swapped(basic_cdr_stream& str, const T* in, size_t N)
{
  if (str.abort_status())
    return;

  str.align(sizeof(T), true);

  T *out = static_cast<T*>(str.get_cursor());
  for (size_t i = 0; i < N; i++, out++, in++) {
    *out = *in;
    byte_swap(*out);
  }

  str.incr_position(sizeof(T)*N);
}

/**
 * @brief
 * Primitive type cursor move function.
 *
 * Used in determining the size of a type when written to the stream.
 * Aligns str to the size of toincr.
 * Moves the cursor of str by the size of toincr.
 * This function is only enabled for arithmetic types.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] toincr The variable to move the cursor by, no contents of this variable are used, it is just used to determine the template.
 */
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void move(basic_cdr_stream& str, const T& toincr)
{
  if (str.abort_status())
    return;

  (void)toincr;

  str.align(sizeof(T), false);

  str.incr_position(sizeof(T));
}

/**
 * @brief
 * Primitive type cursor move function, byteswap version.
 *
 * Same as move(), with an additional byteswap at the end.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] toincr The variable to move the cursor by, no contents of this variable are used, it is just used to determine the template.
 */
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void move_swapped(basic_cdr_stream& str, const T& toincr)
{
  move(str, toincr);
}

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void move_many(basic_cdr_stream& str, const T* toincr, size_t N)
{
  if (str.abort_status())
    return;

  (void)toincr;

  str.align(sizeof(T), false);

  str.incr_position(sizeof(T)*N);
}

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void move_many_swapped(basic_cdr_stream& str, const T* toincr, size_t N)
{
  move_many(str, toincr, N);
}

/**
 * @brief
 * Primitive type max stream move function.
 *
 * Used in determining the maximum stream size of a constructed type.
 * Moves the cursor to the maximum position it could occupy after
 * writing max_sz to the stream.
 * Is in essence the same as the primitive type cursor move function,
 * but additionally checks for whether the cursor it at the "end",
 * which may happen if unbounded members (strings/sequences/...)
 * are part of the constructed type.
 * This function is only enabled for arithmetic types.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] max_sz The variable to move the cursor by, no contents of this variable are used, it is just used to determine the template.
 */
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void max(basic_cdr_stream& str, const T& max_sz)
{
  if (str.abort_status())
    return;

  if (str.position() == SIZE_MAX)
    return;

  move(str, max_sz);
}

/**
 * @brief
 * Primitive type max stream move function.
 *
 * Same as max(), with an additional byteswap at the end.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] max_sz The variable to move the cursor by, no contents of this variable are used, it is just used to determine the template.
 */
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void max_swapped(basic_cdr_stream& str, const T& max_sz)
{
  max(str, max_sz);
}

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void max_many(basic_cdr_stream& str, const T* max_sz, size_t N)
{
  if (str.abort_status())
    return;

  if (str.position() == SIZE_MAX)
    return;

  move_many(str, max_sz, N);
}

template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
inline void max_many_swapped(basic_cdr_stream& str, const T* max_sz, size_t N)
{
  max_many(str, max_sz, N);
}


/**
 * @brief
 * Enumerated type stream manipulation functions.
 * Since enumerated types are represented by a uint32_t in basic CDR streams
 * they just loop through to writing uint32_t versions of the enum.
 *
 * These are "endpoints" for write functions, since compound
 * (sequence/array/constructed type) functions will decay to these
 * calls.
 */

/**
 * @brief
 * Reads the value of the enum from the stream.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read into.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void read(basic_cdr_stream& str, T& toread) {
  read(str, *reinterpret_cast<uint32_t*>(&toread));
}

/**
 * @brief
 * Reads the byteswapped value of the enum from the stream.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read into.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void read_swapped(basic_cdr_stream& str, T& toread) {
  read_swapped(str, *reinterpret_cast<uint32_t*>(&toread));
}

template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void read_many(basic_cdr_stream& str, T* toread, size_t N) {
  read_many(str, reinterpret_cast<uint32_t*>(toread), N);
}

template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void read_many_swapped(basic_cdr_stream& str, T* toread, size_t N) {
  read_many_swapped(str, reinterpret_cast<uint32_t*>(toread), N);
}

/**
 * @brief
 * Writes the value of the enum to the stream.
 *
 * @param [in, out] str The stream which is written to.
 * @param [in] towrite The variable to write.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void write(basic_cdr_stream& str, const T& towrite) {
  write(str, uint32_t(towrite));
}

/**
 * @brief
 * Writes the byteswapped value of the enum to the stream.
 *
 * @param [in, out] str The stream which is written to.
 * @param [in] towrite The variable to write.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void write_swapped(basic_cdr_stream& str, const T& towrite) {
  write_swapped(str, uint32_t(towrite));
}

template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void write_many(basic_cdr_stream& str, const T* towrite, size_t N) {
  write_many(str, static_cast<uint32_t*>(towrite), N);
}

template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void write_many_swapped(basic_cdr_stream& str, const T* towrite, size_t N) {
  write_many_swapped(str, static_cast<uint32_t*>(towrite), N);
}

/**
 * @brief
 * Moves the cursor of the stream by the size the enum would take up.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] toincr The variable to move the cursor by, no contents of this variable are used, it is just used to determine the template.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void move(basic_cdr_stream& str, const T& toincr) {
  move(str, uint32_t(toincr));
}

/**
 * @brief
 * Moves the cursor of the stream by the size the enum would take up (byteswapped version).
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] toincr The variable to move the cursor by, no contents of this variable are used, it is just used to determine the template.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void move_swapped(basic_cdr_stream& str, const T& toincr) {
  move_swapped(str, uint32_t(toincr));
}

template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void move_many(basic_cdr_stream& str, const T* toincr, size_t N) {
  move_many(str, static_cast<uint32_t*>(toincr), N);
}

template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void move_many_swapped(basic_cdr_stream& str, const T* toincr, size_t N) {
  move_many_swapped(str, static_cast<uint32_t*>(toincr), N);
}

/**
 * @brief
 * Moves the cursor of the stream by the size the enum would take up (maximum size version).
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] max_sz The variable to move the cursor by, no contents of this variable are used, it is just used to determine the template.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void max(basic_cdr_stream& str, const T& max_sz) {
  max(str, uint32_t(max_sz));
}

/**
 * @brief
 * Moves the cursor of the stream by the size the enum would take up (maximum size and byteswapped version).
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] max_sz The variable to move the cursor by, no contents of this variable are used, it is just used to determine the template.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void max_swapped(basic_cdr_stream& str, const T& max_sz) {
  max_swapped(str, uint32_t(max_sz));
}

template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void max_many(basic_cdr_stream& str, const T* max_sz, size_t N) {
  max_many(str, static_cast<uint32_t*>(max_sz), N);
}

template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
inline void max_many_swapped(basic_cdr_stream& str, const T* max_sz, size_t N) {
  max_many_swapped(str, static_cast<uint32_t*>(max_sz), N);
}

 /**
 * @brief
 * String type stream manipulation functions
 *
 * These are "endpoints" for write functions, since compound
 * (sequence/array/constructed type) functions will decay to these
 * calls.
 */

/**
 * @brief
 * Bounded string read function.
 *
 * Reads the length from str, but then initializes toread with at most N characters from it.
 * It does move the cursor by length read, since that is the number of characters in the stream.
 * If N is 0, then the string is taken to be unbounded.
 *
 * @param[in, out] str The stream to read from.
 * @param[out] toread The string to read to.
 * @param[in] N The maximum number of characters to read from the stream.
 */
template<typename T>
void read_string(basic_cdr_stream& str, T& toread, size_t N)
{
  if (str.abort_status())
    return;

  uint32_t string_length = 0;

  read(str, string_length);

  if (string_length == 0 &&
      str.status(serialization_status::illegal_field_value))
      return;

  //the string length in the CDR stream includes the terminating NULL
  //therefore the checks on the string length decrease it by 1

  if (N &&
      string_length - 1 > N &&
      str.status(serialization_status::read_bound_exceeded))
      return;

  auto cursor = str.get_cursor();
  toread.assign(static_cast<char*>(cursor), std::min<size_t>(string_length - 1, N ? N : SIZE_MAX));  //remove 1 for terminating NULL

  str.incr_position(string_length);

  //aligned to chars
  str.alignment(1);
}

/**
 * @brief
 * Bounded string read function, byteswapped version.
 *
 * Same as read_string(), only that the string length is byte swapped.
 *
 * @param[in, out] str The stream to read from.
 * @param[out] toread The string to read to.
 * @param[in] N The maximum number of characters to read from the stream.
 */
template<typename T>
void read_string_swapped(basic_cdr_stream& str, T& toread, size_t N)
{
  if (str.abort_status())
    return;

  uint32_t string_length = 0;

  read_swapped(str, string_length);

  if (string_length == 0 &&
      str.status(serialization_status::illegal_field_value))
      return;

  //the string length in the CDR stream includes the terminating NULL
  //therefore the checks on the string length decrease it by 1

  if (N &&
      string_length - 1 > N &&
      str.status(serialization_status::read_bound_exceeded))
      return;

  auto cursor = str.get_cursor();
  toread.assign(static_cast<char*>(cursor), std::min<size_t>(string_length - 1, N ? N : SIZE_MAX));  //remove 1 for terminating NULL

  str.incr_position(string_length);

  //aligned to chars
  str.alignment(1);
}

/**
 * @brief
 * Bounded string write function.
 *
 * Attempts to write the length of towrite to str, where the bound is checked.
 * Then writes the contents of towrite to str.
 * If N is 0, then the string is taken to be unbounded.
 *
 * @param[in, out] str The stream to write to.
 * @param[in] towrite The string to write.
 * @param[in] N The maximum number of characters to write to the stream.
 */
template<typename T>
void write_string(basic_cdr_stream& str, const T& towrite, size_t N)
{
  if (str.abort_status())
    return;

  size_t string_length = towrite.length();
  if (N &&
      string_length > N) {
    if (str.status(serialization_status::write_bound_exceeded))
      return;
  }

  //add 1 extra for terminating NULL
  string_length++;

  write(str, uint32_t(string_length));

  memcpy(str.get_cursor(), towrite.c_str(), string_length);

  str.incr_position(string_length);

  //aligned to chars
  str.alignment(1);
}

/**
 * @brief
 * Bounded string write function, byteswapped version.
 *
 * Same as write_string(), only that the string length is byte swapped.
 *
 * @param[in, out] str The stream to write to.
 * @param[in] towrite The string to write.
 * @param[in] N The maximum number of characters to write to the stream.
 */
template<typename T>
void write_string_swapped(basic_cdr_stream& str, const T& towrite, size_t N)
{
  if (str.abort_status())
    return;

  size_t string_length = towrite.length();
  if (N &&
      string_length > N) {
    if (str.status(serialization_status::write_bound_exceeded))
      return;
  }

  //add 1 extra for terminating NULL
  string_length++;

  write_swapped(str, uint32_t(string_length));

  memcpy(str.get_cursor(), towrite.c_str(), string_length);

  str.incr_position(string_length);

  //aligned to chars
  str.alignment(1);
}

/**
 * @brief
 * Bounded string cursor move function.
 *
 * Attempts to move the cursor for the length field, where the bound is checked.
 * Then moves the cursor for the length of the string.
 * If N is 0, then the string is taken to be unbounded.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] toincr The string used to move the cursor.
 * @param[in] N The maximum number of characters in the string which the stream is moved by.
 */
template<typename T>
void move_string(basic_cdr_stream& str, const T& toincr, size_t N)
{
  if (str.abort_status())
    return;

  size_t string_length = toincr.length();

  if (N &&
      string_length > N) {
    if (str.status(serialization_status::move_bound_exceeded))
      return;
  }

  //add 1 extra for terminating NULL
  string_length++;
  move(str, uint32_t(string_length));

  str.incr_position(string_length);

  //aligned to chars
  str.alignment(1);
}

/**
 * @brief
 * Bounded string cursor move function, byteswapped version.
 *
 * Same as move_string().
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] toincr The string used to move the cursor.
 * @param[in] N The maximum number of characters in the string which the stream is moved by.
 */
template<typename T>
inline void move_string_swapped(basic_cdr_stream& str, const T& toincr, size_t N)
{
  move_string(str, toincr, N);
}

/**
 * @brief
 * Bounded string cursor max move function.
 *
 * Similar to the string move function, with the additional checks that no move
 * is done if the cursor is already at its maximum position, and that the cursor
 * is set to its maximum position if the bound is equal to 0 (unbounded).
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] max_sz The string used to move the cursor.
 * @param[in] N The maximum number of characters in the string which the stream is moved by.
 */
template<typename T>
void max_string(basic_cdr_stream& str, const T& max_sz, size_t N)
{
  if (str.abort_status())
    return;

  (void)max_sz;

  if (str.position() == SIZE_MAX)
    return;

  if (N == 0)
  {
    //unbounded string, theoretical length unlimited
    str.position(SIZE_MAX);
  }
  else
  {
    //length field
    max(str, uint32_t(0));

    //bounded string, length maximum N+1 characters
    str.incr_position(N + 1);

    //aligned to chars
    str.alignment(1);
  }
}

/**
 * @brief
 * Bounded string cursor max move function, byteswapped version.
 *
 * Same as max_string().
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] max_sz The string used to move the cursor.
 * @param[in] N The maximum number of characters in the string which the stream is moved by.
 */
template<typename T>
inline void max_string_swapped(basic_cdr_stream& str, const T& max_sz, size_t N)
{
  max_string(str, max_sz, N);
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
#endif
