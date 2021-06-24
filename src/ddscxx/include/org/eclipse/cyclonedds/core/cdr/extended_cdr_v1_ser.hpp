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
#ifndef EXTENDED_CDR_SERIALIZATION_V1_HPP_
#define EXTENDED_CDR_SERIALIZATION_V1_HPP_

#include "cdr_stream.hpp"

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

/**
 * @brief
 * Implementation of the extended cdr version1 stream.
 *
 * This type of cdr stream has a maximum alignment of 8 bytes.
 */
class OMG_DDS_API xcdr_v1_stream : public cdr_stream {
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
  xcdr_v1_stream(endianness end = native_endianness(), uint64_t ignore_faults = 0x0) : cdr_stream(end, 8, ignore_faults) { ; }

  /**
   * @brief
   * Starts a new member.
   *
   * Determines whether a header is necessary for this entity through header_necessary, and if it is, handles the header.
   *
   * @param[in] prop Properties of the member to start.
   * @param[in] present Whether the entity represented by prop is present, if it is an optional entity.
   */
  void start_member(entity_properties_t &prop, bool present);

  /**
   * @brief
   * Finishes a member.
   *
   * Determines whether a header is necessary for this entity through header_necessary, and if it is, completes the previous header.
   *
   * @param[in] prop Properties of the member to finish.
   * @param[in] present Whether the entity represented by prop is present, if it is an optional entity.
   */
  void finish_member(entity_properties_t &prop, bool present);

  /**
   * @brief
   * Skips an entity, bypassing the stack.
   *
   * Used in skipping fields which are not recognized by a class being deserialized.
   *
   * @param[in] props The entity to skip.
   */
  void skip_entity(const entity_properties_t &props);

  /**
   * @brief
   * Returns the next entity to be processed.
   *
   * Depending on the data structure and the streaming mode, either a header is read from the stream, or a
   * properties entry is pulled from the tree.
   *
   * @param[in, out] props The property tree to get the next entity from.
   * @param[in, out] firstcall Whether it is the first time calling the function for props, will store first iterator if true, and then set to false.
   *
   * @return The next entity to be processed, or the final entity if the current tree level does not hold more entities.
   */
  entity_properties_t& next_entity(entity_properties_t &props, bool &firstcall);

  /**
   * @brief
   * Starts a new struct.
   *
   * As the extended cdr v1 stream does not have anything that requires delimiting between entities, this function does nothing.
   */
  void start_struct(entity_properties_t &) {;}

  /**
   * @brief
   * Finishes the current struct.
   *
   * Adds the final parameter list entry if necessary when writing to the stream.
   *
   * @param[in, out] props The property tree to get the next entity from.
   */
  void finish_struct(entity_properties_t &props);

private:

  static const uint16_t pid_mask;                         /**< the mask for non-extended parameter list ids*/
  static const uint16_t pid_extended;                     /**<  indicating an extended entry*/
  static const uint16_t pid_list_end;                     /**<  guardian entry indicating end of parameter list*/
  static const uint16_t pid_ignore;                       /**<  ignore this entry*/
  static const uint16_t pid_flag_impl_extension;          /**<  bit flag indicating implementation specific extension*/
  static const uint16_t pid_flag_must_understand;         /**< bit flag indicating that this entry must be parsed successfully or the entire sample must be discarded*/
  static const uint32_t pl_extended_mask;                 /**<  mask for extended parameter list ids*/
  static const uint32_t pl_extended_flag_impl_extension;  /**< bit flag indicating implementation specific extension*/
  static const uint32_t pl_extended_flag_must_understand; /**< bit flag indicating that this entry must be parsed successfully or the entire sample must be discarded*/

  /**
   * @brief
   * Returns the next entity to be processed.
   *
   * For optional members and members of mutable structures, a parameter list header field is necessary preceding
   * the field contents itself.
   *
   * @param[in] props The properties of the entity.
   *
   * @return Whether a header is necessary for the entity.
   */
  bool header_necessary(const entity_properties_t &props);

  /**
   * @brief
   * Determines whether a parameter list is necessary.
   *
   * @param[in] props The entity whose members might be represented by a parameter list.
   */
  bool list_necessary(const entity_properties_t &props);

  /**
   * @brief
   * Reads a header field from the stream.
   *
   * If header_necessary returns true for a field, then this function needs to be called first to read the
   * header from stream and to allow the streamer to determine what to do with the field.
   *
   * @param[out] props The header to read into.
   */
  void read_header(entity_properties_t &props);

  /**
   * @brief
   * Writes a header field to the stream.
   *
   * If header_necessary returns true for a field, then this function needs to be called first to write the
   * header to the stream before the contents of the field are written.
   *
   * @param[in, out] props The properties of the entity.
   */
  void write_header(entity_properties_t &props);

  /**
   * @brief
   * Finishes a header field in the stream.
   *
   * Goes back to the offset of the length field that was unfinished in 
   *
   * @param[in, out] props The properties of the entity.
   */
  void finish_write_header(entity_properties_t &props);

  /**
   * @brief
   * Writes the terminating entry in a parameter list.
   */
  void write_final_list_entry();

  /**
   * @brief
   * Moves the cursor as if writing the terminating entry in a parameter list.
   */
  void move_final_list_entry();

  /**
   * @brief
   * Moves the stream offset by the amount that would have been written by write_header.
   *
   * This function needs to be called first to move the stream by the same amount the header would
   * have taken up, if it would have been written.
   *
   * @param[in] props The entity to move the cursor by.
   */
  void move_header(const entity_properties_t &props);

  /**
   * @brief
   * Determines whether to use extended format header.
   *
   * An extended header is necessary for entities with a size larger than 65535 bytes or entities with
   * a member id larger than 16128.
   *
   * @param[in] props The entity to check.
   *
   * @return Whether an extended format header is necessary.
   */
  static bool extended_header(const entity_properties_t &props);
};

/**
 * @brief
 * Enumerated type stream manipulation functions.
 * Depending on the number coverage of the enum, it will be written
 * to the stream as an uint8_t, an uint16_t or a uint32_t.
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
void read(xcdr_v1_stream& str, T& toread, size_t N = 1) {
  switch (str.is_key() ? bb_32_bits : str.top_of_stack().e_bb)
  {
    case bb_8_bits:
      read_enum_impl<xcdr_v1_stream,T,uint8_t>(str, toread, N);
      break;
    case bb_16_bits:
      read_enum_impl<xcdr_v1_stream,T,uint16_t>(str, toread, N);
      break;
    case bb_32_bits:
      read_enum_impl<xcdr_v1_stream,T,uint32_t>(str, toread, N);
      break;
    default:
      assert(false);
  }
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
void write(xcdr_v1_stream& str, const T& towrite, size_t N = 1) {
  switch (str.is_key() ? bb_32_bits : str.top_of_stack().e_bb)
  {
    case bb_8_bits:
      write_enum_impl<xcdr_v1_stream,T,uint8_t>(str, towrite, N);
      break;
    case bb_16_bits:
      write_enum_impl<xcdr_v1_stream,T,uint16_t>(str, towrite, N);
      break;
    case bb_32_bits:
      write_enum_impl<xcdr_v1_stream,T,uint32_t>(str, towrite, N);
      break;
    default:
      assert(false);
  }
}

/**
 * @brief
 * Moves the cursor of the stream by the size the enum would take up.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] N The number of entities to move.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void move(xcdr_v1_stream& str, const T&, size_t N = 1) {
  switch (str.is_key() ? bb_32_bits : str.top_of_stack().e_bb)
  {
    case bb_8_bits:
      move(str, int8_t(0), N);
      break;
    case bb_16_bits:
      move(str, int16_t(0), N);
      break;
    case bb_32_bits:
      move(str, int32_t(0), N);
      break;
    default:
      assert(false);
  }
}

/**
 * @brief
 * Moves the cursor of the stream by the size the enum would take up (maximum size version).
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] max_sz The variable to move the cursor by, no contents of this variable are used, it is just used to determine the template.
 * @param[in] N The number of entities at most to move.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void max(xcdr_v1_stream& str, const T& max_sz, size_t N = 1) {
  move(str, max_sz, N);
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
#endif
