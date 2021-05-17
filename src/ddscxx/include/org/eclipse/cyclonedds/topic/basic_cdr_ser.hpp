/*
 * Copyright(c) 2020 ADLINK Technology Limited and others
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

/**
* Implementation of the basic cdr stream.
*/
class basic_cdr_stream : public cdr_stream {
public:
  basic_cdr_stream(endianness end = native_endianness()) : cdr_stream(end, 8) { ; }
};

/**
* Primitive type stream manipulation functions.
*
* These are "endpoints" for write functions, since composit
* (sequence/array/constructed type) functions will decay to these
* calls.
*/

/**
* Primitive type read function.
*
* Aligns the stream to the alignment of type T.
* Reads the value from the current position of the stream str into
* toread, will swap bytes if necessary.
* Moves the cursor of the stream by the size of T.
*/
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
void read(basic_cdr_stream &str, T& toread)
{
  char *cursor = str.get_cursor();
  str.align(sizeof(T), false);

  assert(cursor);
  transfer_and_swap(*(reinterpret_cast<const T*>(cursor)), toread, str.swap_endianness());
  str.incr_position(sizeof(T));
}

/**
* Primitive type write function.
*
* Aligns str to the type to be written.
* Writes towrite to str.
* Swaps bytes written to str if the endiannesses do not match up.
* Moves the cursor of str by the size of towrite.
*/
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
void write(basic_cdr_stream& str, const T& towrite)
{
  char *cursor = str.get_cursor();
  str.align(sizeof(T), true);

  assert(cursor);
  transfer_and_swap(towrite, *(reinterpret_cast<T*>(cursor)), str.swap_endianness());
  str.incr_position(sizeof(T));
}

/**
* Primitive type cursor move function.
*
* Used in determining the size of a type when written to the stream.
* Aligns str to the size of toincr.
* Moves the cursor of str by the size of toincr.
*/
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
void move(basic_cdr_stream& str, const T& toincr)
{
  (void)toincr;

  str.align(sizeof(T), false);

  str.incr_position(sizeof(T));
}

/**
* Primitive type max stream move function.
*
* Used in determining the maximum stream size of a constructed type.
* Moves the cursor to the maximum position it could occupy after
* writing max_sz to the stream.
* Is in essence the same as the primitive type cursor move function,
* but additionally checks for whether the cursor it at the "end",
* which may happen if unbounded members (strings/sequences/...)
* are part of the constructed type.
*/
template<typename T, std::enable_if_t<std::is_arithmetic<T>::value && !std::is_enum<T>::value, bool> = true >
void max(basic_cdr_stream& str, const T& max_sz)
{
  if (str.position() == SIZE_MAX)
    return;

  move(str, max_sz);
}

/**
* Enumerated type stream manipulation functions.
* Since enumerated types are represented by a uint32_t in basic CDR streams
* they just loop through to writing uint32_t versions of the enum.
*
* These are "endpoints" for write functions, since compound
* (sequence/array/constructed type) functions will decay to these
* calls.
*/

/**
* Enum read function.
*/
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void read(basic_cdr_stream& str, T& toread) {
  read(str, *reinterpret_cast<uint32_t*>(&toread));
}

/**
* Enum write function.
*/
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void write(basic_cdr_stream& str, const T& towrite) {
  write(str, uint32_t(towrite));
}

/**
* Enum move function.
*/
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void move(basic_cdr_stream& str, const T& toincr) {
  move(str, uint32_t(toincr));
}

/**
* Enum max move function.
*/
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void max(basic_cdr_stream& str, const T& max_sz) {
  max(str, uint32_t(max_sz));
}

/*
* String type stream manipulation functions
*
* These are "endpoints" for write functions, since compound
* (sequence/array/constructed type) functions will decay to these
* calls.
*/

/*
* Bounded string read function.
*
* Reads the length from str, but then initializes toread with at most N characters from it.
* It does move the cursor by length read, since that is the number of characters in the stream.
*/
template<typename T>
void read_string(basic_cdr_stream& str, T& toread, size_t N)
{
  uint32_t string_length = 0;

  read(str, string_length);

  auto cursor = str.get_cursor();
  toread.assign(cursor, cursor + std::min<size_t>(string_length - 1, N ? N : SIZE_MAX));  //remove 1 for terminating NULL

  str.incr_position(string_length);

  //aligned to chars
  str.alignment(1);
}

/**
* Bounded string write function.
*
* Attempts to write the length of towrite to str, where the bound is checked.
* Then writes the contents of towrite to str.
*/
template<typename T>
void write_string(basic_cdr_stream& str, const T& towrite, size_t N)
{
  size_t string_length = towrite.length() + 1;  //add 1 extra for terminating NULL

  //throw an exception if we attempt to write a length field in excess of the supplied bound
  if (N && string_length > N)
    throw dds::core::InvalidDataError("Attempt to write string with length " + std::to_string(string_length) + " exceeding maximum length of " + std::to_string(N) + ".");

  write(str, uint32_t(string_length));

  memcpy(str.get_cursor(), towrite.c_str(), string_length);

  str.incr_position(string_length);

  //aligned to chars
  str.alignment(1);
}

/**
* Bounded string cursor move function.
*
* Attempts to move the cursor for the length field, where the bound is checked.
* Then moves the cursor for the length of the string.
*/
template<typename T>
void move_string(basic_cdr_stream& str, const T& toincr, size_t N)
{
  size_t string_length = toincr.length() + 1;  //add 1 extra for terminating NULL

  //throw an exception if we attempt to move the cursor for a length field in excess of the supplied bound
  if (N && string_length > N)
    throw dds::core::InvalidDataError("Attempt to move cursor for string with length " + std::to_string(string_length) + " exceeding maximum length of " + std::to_string(N) + ".");

  move(str, uint32_t(string_length));

  str.incr_position(string_length);

  //aligned to chars
  str.alignment(1);
}

/**
* Bounded string cursor max move function.
*
* Similar to the string move function, with the additional checks that no move
* is done if the cursor is already at its maximum position, and that the cursor
* is set to its maximum position if the bound is equal to 0 (unbounded).
*/
template<typename T>
void max_string(basic_cdr_stream& str, const T& max_sz, size_t N)
{
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

#endif
