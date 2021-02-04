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
#include <stdexcept>
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
  str.align(sizeof(T), false);

  transfer_and_swap(
    *(reinterpret_cast<const T*>(str.get_cursor())),
    toread,
    str.swap_endianness());

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
  str.align(sizeof(T), true);

  transfer_and_swap(
    towrite,
    *(reinterpret_cast<T*>(str.get_cursor())),
    str.swap_endianness());

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

/**
* Stream length field manipulation functions.
* Functions for reading/writing length fields for objects like sequences/strings.
*/

/**
* Bound checked length field write function.
*
* Will check whether a bound is present for this length field, and will throw
* an exception if a length is attempted to be written which exceeds this
* (bound of 0 equals no bound present).
* Writes length to str as a uint32_t type.
*/
static void write_length(basic_cdr_stream& str, size_t length, size_t bound)
{
  //throw an exception if we attempt to write a length field in excess of the supplied bound
  if (bound &&
    length > bound)
    throw 1;  //replace throw

  write(str, uint32_t(length));
}

/**
* Length field read function.
*
* Reads length from the stream.
* This is not bound checked, to prevent reading of spurious data, which the program has
* no control over to result in program failure.
*/
static void read_length(basic_cdr_stream& str, uint32_t& length)
{
  read(str, length);
}

/**
* Bounds checked length field move function.
*
* Will check whether a bound is present for this length field, and will throw
* an exception if a length is attempted to be written which exceeds this
* (bound of 0 equals no bound present).
* Moves the stream cursor by a uint32_t representation of the length.
*/
static void move_length(basic_cdr_stream& str, size_t length, size_t bound)
{
  //throw an exception if we attempt to move the cursor for a length field in excess of the supplied bound
  if (bound &&
    length > bound)
    throw 2;  //replace throw

  move(str, uint32_t(length));
}

/**
* Bounded vector length read and resize combo function.
*
* Reads the length field from str into seq_length, but only increases the size of 
* toread by at most N if this is not 0.
*/
template<typename T, size_t N>
void read_vec_resize(basic_cdr_stream& str, idl_sequence<T, N>& toread, uint32_t& seq_length)
{
  //the length of the entries contained in the stream is read, but...
  read_length(str, seq_length);

  //the container is only enlarged upto its maximum size
  auto read_length = std::min<size_t>(seq_length, N ? N : SIZE_MAX);

  toread.resize(read_length);
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
template<size_t N>
void read(basic_cdr_stream& str, idl_string<N>& toread)
{
  uint32_t string_length = 0;

  read_length(str, string_length);

  auto cursor = str.get_cursor();
  toread.assign(cursor, cursor + std::min<size_t>(string_length - 1, N ? N : SIZE_MAX));  //remove 1 for terminating NULL

  str.incr_position(string_length);
}

/**
* Bounded string write function.
*
* Attempts to write the length of towrite to str, where the bound is checked.
* Then writes the contents of towrite to str.
*/
template<size_t N>
void write(basic_cdr_stream& str, const idl_string<N>& towrite)
{
  size_t string_length = towrite.length() + 1;  //add 1 extra for terminating NULL

  write_length(str, string_length, N);

  //no check on string length necessary after this since it is already checked in write_length

  memcpy(str.get_cursor(), towrite.c_str(), string_length);

  str.incr_position(string_length);
}

/**
* Bounded string cursor move function.
*
* Attempts to move the cursor for the length field, where the bound is checked.
* Then moves the cursor for the length of the string.
*/
template<size_t N>
void move(basic_cdr_stream& str, const idl_string<N>& toincr)
{
  size_t string_length = toincr.length() + 1;  //add 1 extra for terminating NULL

  move_length(str, string_length, N);

  //no check on string length necessary after this since it is already checked in incr_length

  str.incr_position(string_length);
}

/**
* Bounded string cursor max move function.
*
* Similar to the string move function, with the additional checks that no move
* is done if the cursor is already at its maximum position, and that the cursor
* is set to its maximum position if the bound is equal to 0 (unbounded).
*/
template<size_t N>
void max(basic_cdr_stream& str, const idl_string<N>& max_sz)
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
  }
}

/**
* Array type stream manipulation functions.
*/

/**
* Arrays of primitive types.
*
* These are "endpoints" for write functions, since compound
* (sequence/array/constructed type) functions will consist of these
* calls.
*/

/**
* Primitive array read function.
*
* Copies the entire array to toread with a single memcpy, since there
* are no structures to be unpacked.
*/
template<typename T, size_t N, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void read(basic_cdr_stream& str, idl_array<T, N>& toread)
{
  str.align(sizeof(T), false);

  memcpy(toread.data(), str.get_cursor(), N*sizeof(T));

  if (sizeof(T) > 1 &&
    str.swap_endianness())
  {
    for (auto& e : toread)
      byte_swap(e);
  }

  str.incr_position(N * sizeof(T));
}

/**
* Primitive array write function.
*
* Copies the entire array to toread with a single memcpy, since there
* are no structures to be unpacked.
*/
template<typename T, size_t N, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void write(basic_cdr_stream& str, const idl_array<T, N>& towrite)
{
  str.align(sizeof(T), false);

  memcpy(str.get_cursor(), towrite.data(), N * sizeof(T));

  if (sizeof(T) > 1 &&
    str.swap_endianness())
  {
    for (size_t i = 0; i < N; i++)
    {
      byte_swap(*static_cast<T*>(str.get_cursor()));
      str.incr_position(sizeof(T));
    }
  }
  else
  {
    str.incr_position(N * sizeof(T));
  }
}

/**
* Primitive array move function.
*/
template<typename T, size_t N, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void move(basic_cdr_stream& str, const idl_array<T, N>& toincr)
{
  str.align(sizeof(T), false);

  str.incr_position(N * sizeof(T));
}

/**
* Primitive array max move function.
*/
template<typename T, size_t N, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void max(basic_cdr_stream& str, const idl_array<T, N>& max_sz)
{
  (void)max_sz;

  if (str.position() == SIZE_MAX)
    return;

  str.align(sizeof(T), false);

  str.incr_position(N * sizeof(T));
}

/**
* Arrays of complex types.
*
* All these functions do is call more "derived" type functions.
*/

/**
* Complex type array read function.
*/
template<typename T, size_t N>
void read(basic_cdr_stream& str, idl_array<T, N>& toread)
{
  for (auto& e : toread)
    read(str, e);
}

/**
* Complex type array write function.
*/
template<typename T, size_t N>
void write(basic_cdr_stream& str, const idl_array<T, N>& towrite)
{
  for (const auto& e : towrite)
    write(str, e);
}

/**
* Complex type array move function.
*/
template<typename T, size_t N>
void move(basic_cdr_stream& str, const idl_array<T, N>& toincr)
{
  for (const auto& e : toincr)
    move(str, e);
}

/**
* Complex type array max function.
*/
template<typename T, size_t N>
void max(basic_cdr_stream& str, const idl_array<T, N>& max_sz)
{
  if (str.position() == SIZE_MAX)
    return;

  for (const auto & e:max_sz)
    max(str, e);
}

/**
* Sequence type stream manipulation functions.
*
* These attempt to manipulate the sequence length field,
* and then do the appropriate actions for the sequence
* fields.
*/

/**
* Sequences of primitive types.
*
* These are "endpoints" for write functions, since compound
* (sequence/array/constructed type) functions will consist of these
* calls.
*/

/**
* Read function for sequences of bools, since the std::vector<bool> is also specialized, and direct copy will not give a happy result.
*/
template<size_t N>
void read(basic_cdr_stream& str, idl_sequence<bool, N>& toread) {
  uint32_t seq_length = 0;  //this is the sequence length retrieved from the stream, not the number of entities to be written to the sequence object
  read_vec_resize(str, toread, seq_length);

  for (auto& b : toread)
  {
    if (*(str.get_cursor()))
      b = true;
    else
      b = false;
    str.incr_position(1);
  }

  if (N &&
    seq_length > N)
    str.incr_position(seq_length-N);
}

/**
* Primitive sequence read function.
*/
template<typename T, size_t N, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void read(basic_cdr_stream& str, idl_sequence<T, N>& toread) {
  uint32_t seq_length = 0;  //this is the sequence length retrieved from the stream, not the number of entities to be written to the sequence object
  read_vec_resize(str, toread, seq_length);

  str.align(sizeof(T), false);

  memcpy(toread.data(), str.get_cursor(), toread.size() * sizeof(T));

  if (sizeof(T) > 1 &&
    str.swap_endianness())
  {
    for (auto & e:toread)
      byte_swap(e);
  }

  str.incr_position(seq_length * sizeof(T));
}

/**
* Write function for sequences of bools, since the std::vector<bool> is also specialized, and direct copy will not give a happy result.
*/
template<size_t N>
void write(basic_cdr_stream& str, const idl_sequence<bool, N>& towrite) {
  write_length(str, towrite.size(), N);

  //no check on length necessary after this point, it is done in the write_length function

  str.alignment(1);

  for (const auto& b : towrite)
  {
    *(str.get_cursor()) = b ? 0x1 : 0x0;
    str.incr_position(1);
  }
}

/**
* Primitive sequence write function.
*/
template<typename T, size_t N, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void write(basic_cdr_stream& str, const idl_sequence<T, N>& towrite) {
  write_length(str, towrite.size(), N);

  //no check on length necessary after this point, it is done in the write_length function

  str.align(sizeof(T), true);

  memcpy(str.get_cursor(), towrite.data(), towrite.size() * sizeof(T));

  if (sizeof(T) > 1 &&
    str.swap_endianness())
  {
    for (size_t i = 0; i < towrite.size(); i++)
    {
      byte_swap(*static_cast<T*>(str.get_cursor()));
      str.incr_position(sizeof(T));
    }
  }
  else
  {
    str.incr_position(towrite.size() * sizeof(T));
  }
}

/**
* Primitive sequence move function.
*/
template<typename T, size_t N, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void move(basic_cdr_stream& str, const idl_sequence<T, N>& toincr) {
  move_length(str, toincr.size(), N);

  //no check on length necessary after this point, it is done in the incr_length function

  str.align(sizeof(T), false);

  str.incr_position(toincr.size() * sizeof(T));
}

/**
* Primitive sequence max function.
*/
template<typename T, size_t N, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void max(basic_cdr_stream& str, const idl_sequence<T, N>& max_sz) {
  if (str.position() == SIZE_MAX)
    return;

  if (N == 0)
  {
    str.position(SIZE_MAX);
  }
  else
  {
    max(str, uint32_t(0));

    str.align(sizeof(T), false);

    str.incr_position(N * sizeof(T));
  }
}

/*
* Sequences of complex types stream manipulation functions.
*
* These functions just call the more basic implementation functions.
*/

/**
* Complex type sequence read function.
*/
template<typename T, size_t N>
void read(basic_cdr_stream& str, idl_sequence<T, N>& toread) {
  uint32_t seq_length = 0;  //this is the sequence length of entities in the stream, not the number of entities to be read to the sequence
  read_vec_resize(str, toread, seq_length);

  for (auto& e : toread)
    read(str, e);

  //dummy reads
  for (size_t i = N; i < static_cast<size_t>(seq_length); i++)
  {
    T dummy;
    read(str, dummy);
  }
}

/**
* Complex type sequence write function.
*/
template<typename T, size_t N>
void write(basic_cdr_stream& str, const idl_sequence<T, N>& towrite) {
  write_length(str, towrite.size(), N);

  //no check on length necessary after this point, it is done in the write_length function

  for (const auto& e : towrite)
    write(str, e);
}

/**
* Complex type sequence read function.
*/
template<typename T, size_t N>
void move(basic_cdr_stream& str, const idl_sequence<T, N>& toincr) {
  move_length(str, toincr.size(), N);

  //no check on length necessary after this point, it is done in the incr_length function

  for (const auto& e : toincr)
    move(str, e);
}

/**
* Complex type sequence read function.
*/
template<typename T, size_t N>
void max(basic_cdr_stream& str, const idl_sequence<T, N>& max_sz)
{
  (void)max_sz;

  if (str.position() == SIZE_MAX)
    return;

  if (N == 0)
  {
    str.position(SIZE_MAX);
  }
  else
  {
    max(str, uint32_t(0));

    T dummy;
    for (size_t i = 0; i < N; i++)
      max(str, dummy);
  }
}

#endif
