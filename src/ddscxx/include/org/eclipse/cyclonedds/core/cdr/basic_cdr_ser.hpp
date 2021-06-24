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
class OMG_DDS_API basic_cdr_stream : public cdr_stream {
public:
  /**
   * @brief
   * Constructor.
   *
   * Basically a pass through for the cdr_stream base class.
   *
   * @param[in] end The endianness to set for the data stream, default to the local system endianness.
   * @param[in] ignore_faults Bitmask for ignoring faults, can be composed of bit fields from the serialization_status enumerator.
   */
  basic_cdr_stream(endianness end = native_endianness(), uint64_t ignore_faults = 0x0) : cdr_stream(end, 8, ignore_faults) { ; }

  /**
   * @brief
   * Starts a member.
   *
   * As the basic cdr stream does not have anything that requires delimiting between entities, this function does nothing.
   */
  void start_member(entity_properties_t &, bool) {;}

  /**
   * @brief
   * Finishes a member.
   *
   * As the basic cdr stream does not have anything that requires delimiting between entities, this function does nothing.
   */
  void finish_member(entity_properties_t &, bool) {;}

  /**
   * @brief
   * Starts a new struct.
   *
   * As the basic cdr stream does not have anything that requires delimiting between entities, this function does nothing.
   */
  void start_struct(entity_properties_t &) {;}

  /**
   * @brief
   * Finishes the current struct.
   *
   * As the basic cdr stream does not have anything that requires delimiting between entities, this function does nothing.
   */
  void finish_struct(entity_properties_t &) {;}

  /**
   * @brief
   * Skips an entity, bypassing the stack.
   *
   * As the basic cdr stream does not have anything that requires delimiting between entities, this function does nothing.
   */
  void skip_entity(const entity_properties_t &) {;}

  /**
   * @brief
   * Returns the next entity to be processed.
   *
   * This implementation directly passes through to next_prop, since pops and pushes do nothing.
   *
   * @param[in, out] props The property tree to get the next entity from.
   * @param[in, out] firstcall Whether it is the first time calling the function for props, will store first iterator if true, and then set to false.
   *
   * @return The next entity to be processed, or the final entity if the current tree level does not hold more entities.
   */
  entity_properties_t& next_entity(entity_properties_t &props, bool &firstcall);

};

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
 * @param[in] N The number of entities to read.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void read(basic_cdr_stream& str, T& toread, size_t N = 1) {
  read_enum_impl<basic_cdr_stream,T,uint32_t>(str, toread, N);
}

/**
 * @brief
 * Writes the value of the enum to the stream.
 *
 * @param[in, out] str The stream which is written to.
 * @param[in] towrite The variable to write.
 * @param[in] N The number of entities to write.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void write(basic_cdr_stream& str, const T& towrite, size_t N = 1) {
  write_enum_impl<basic_cdr_stream,T,uint32_t>(str, towrite, N);
}

/**
 * @brief
 * Moves the cursor of the stream by the size the enum would take up.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] N The number of entities to move.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void move(basic_cdr_stream& str, const T&, size_t N = 1) {
  move(str, uint32_t(0), N);
}

/**
 * @brief
 * Moves the cursor of the stream by the size the enum would take up (maximum size version).
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] N The number of entities at most to move.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void max(basic_cdr_stream& str, const T&, size_t N = 1) {
  max(str, uint32_t(0), N);
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
#endif
