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
#ifndef EXTENDED_CDR_SERIALIZATION_V2_HPP_
#define EXTENDED_CDR_SERIALIZATION_V2_HPP_

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
class OMG_DDS_API xcdr_v2_stream : public cdr_stream {
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
  xcdr_v2_stream(endianness end = native_endianness(), uint64_t ignore_faults = 0x0) : cdr_stream(end, 4, ignore_faults) { ; }

  /**
   * @brief
   * Starts a new member.
   *
   * Determines whether a header is necessary for this entity through em_header_necessary, and if it is, handles the header.
   *
   * @param[in] prop Properties of the member to start.
   * @param[in] present Whether the entity represented by prop is present, if it is an optional entity.
   */
  void start_member(entity_properties_t &prop, bool present);

  /**
   * @brief
   * Finishes a member.
   *
   * Determines whether a header is necessary for this entity through em_header_necessary, and if it is, completes the previous header.
   *
   * @param[in] prop Properties of the member to finish.
   * @param[in] present Whether the entity represented by prop is present, if it is an optional entity.
   */
  void finish_member(entity_properties_t &prop, bool present);

  /**
   * @brief
   * Skips an entity, bypassing the stack.
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
   * Start a new struct.
   *
   * This function is called by the generated streaming functions, and will start a parameter list, if that is relevant for it.
   *
   * @param[in, out] props The entity whose members might be represented by a parameter list.
   */
  void start_struct(entity_properties_t &props);

  /**
   * @brief
   * Finish the current struct.
   *
   * This function is called by the generated streaming functions, and will finish the current parameter list, if that is relevant for it.
   *
   * @param[in, out] props The entity whose members might be represented by a parameter list.
   */
  void finish_struct(entity_properties_t &props);

private:

  static const uint32_t bytes_1;          /**< length field code indicating length is 1 byte*/
  static const uint32_t bytes_2;          /**< length field code indicating length is 2 bytes*/
  static const uint32_t bytes_4;          /**< length field code indicating length is 4 bytes*/
  static const uint32_t bytes_8;          /**< length field code indicating length is 8 bytes*/
  static const uint32_t nextint;          /**< length field code indicating length is the next integer field*/
  static const uint32_t nextint_times_1;  /**< same as nextint*/
  static const uint32_t nextint_times_4;  /**< length field code indicating length is the next integer field times 4*/
  static const uint32_t nextint_times_8;  /**< length field code indicating length is the next integer field times 8*/
  static const uint32_t lc_mask;          /**< mask for length field codes*/
  static const uint32_t id_mask;          /**< mask for member ids*/
  static const uint32_t must_understand;  /**< must understand member field flag*/

  /**
   * @brief
   * Reads a D-header from the stream.
   *
   * @param[out] props The entity to read the D-header into.
   */
  void read_d_header(entity_properties_t &props);

  /**
   * @brief
   * Reads an EM-header from the stream.
   *
   * @param[out] props The entity to read the D-header into.
   */
  void read_em_header(entity_properties_t &props);

  /**
   * @brief
   * Writes a D-header to the stream.
   *
   * @param[out] props The entity to write the D-header for.
   */
  void write_d_header(entity_properties_t &props);

  /**
   * @brief
   * Adds an optional flag to the stream.
   *
   * In the case of an optional field, but not a parameter list, the xcdrv2 spec states that this field should
   * preceded by a single boolean, indicating its presence or absence.
   * This function creates a placeholder for this boolean, which will be filled later with finish_optional_tag.
   *
   * @param[out] props The entity to write the optional flag for.
   * @param[in] present Whether the entity represented by prop is present.
   */
  void write_optional_tag(entity_properties_t &props, bool present);

  /**
   * @brief
   * Moves the stream cursor by the amount of an optional flag.
   *
   * In the case of an optional field, but not a parameter list, the xcdrv2 spec states that this field should
   * preceded by a single boolean, indicating its presence or absence.
   */
  void move_optional_tag();

  /**
   * @brief
   * Writes an EM-header to the stream.
   *
   * @param[in, out] props The entity to write the EM-header for.
   */
  void write_em_header(entity_properties_t &props);

  /**
   * @brief
   * Moves the stream's position by the amount that it would after writing the D-header.
   */
  void move_d_header();

  /**
   * @brief
   * Moves the stream's position by the amount that it would after writing the EM-header.
   *
   * @param[in] props The entity properties, used to determine whether the extended length field is necessary.
   */
  void move_em_header(const entity_properties_t &props);

  /**
   * @brief
   * Finishes the write operation of the D-header.
   *
   * @param[in, out] props The entity whose D-header to finish.
   */
  void finish_d_header(entity_properties_t &props);

  /**
   * @brief
   * Finishes the write operation of the EM-header.
   *
   * @param[in, out] props The entity whose EM-header to finish.
   */
  void finish_em_header(entity_properties_t &props);

  /**
   * @brief
   * Checks whether a D-header is necessary for the indicated entity.
   *
   * @param[in] props The entity whose properties to check.
   *
   * @return Whether the entity props needs a D-header
   */
  bool d_header_necessary(const entity_properties_t &props);

  /**
   * @brief
   * Checks whether a EM-header is necessary for the indicated entity.
   *
   * @param[in] props The entity whose properties to check.
   *
   * @return Whether the entity props needs a EM-header
   */
  bool em_header_necessary(const entity_properties_t &props);

  /**
   * @brief
   * Determines whether a parameter list is necessary.
   *
   * This function is called by the next_entity function, to determine whether or not
   * to read em headers from the stream.
   *
   * @param[in] props The entity whose members might be represented by a parameter list.
   *
   * @return Whether a list is necessary for this entity.
   */
  bool list_necessary(const entity_properties_t &props);

  /**
   * @brief
   * Checks whether a delimited cdr stream is not being read out of bounds.
   *
   * This function will return true if enough bytes are available for reading another header.
   * It will also set the invalid_dl_entry flag if the end of the bounds has been reached.
   *
   * @param[in] props The entity whose members might be represented by a parameter list.
   *
   * @return Whether enough bytes are available for another header.
   */
  bool bytes_available(const entity_properties_t &props);
};

/**
 * @brief
 * Reads the value of the enum from the stream.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read into.
 * @param[in] N The number of entities to read.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void read(xcdr_v2_stream& str, T& toread, size_t N = 1) {
  switch (str.is_key() ? bb_32_bits : str.top_of_stack().e_bb)
  {
    case bb_8_bits:
      read_enum_impl<xcdr_v2_stream,T,uint8_t>(str, toread, N);
      break;
    case bb_16_bits:
      read_enum_impl<xcdr_v2_stream,T,uint16_t>(str, toread, N);
      break;
    case bb_32_bits:
      read_enum_impl<xcdr_v2_stream,T,uint32_t>(str, toread, N);
      break;
    default:
      assert(false);
  }
}

/**
 * @brief
 * Writes the value of the enum to the stream.
 *
 * @param [in, out] str The stream which is written to.
 * @param [in] towrite The variable to write.
 * @param[in] N The number of entities to write.
 */
template<typename T, std::enable_if_t<std::is_enum<T>::value && !std::is_arithmetic<T>::value, bool> = true >
void write(xcdr_v2_stream& str, const T& towrite, size_t N = 1) {
  switch (str.is_key() ? bb_32_bits : str.top_of_stack().e_bb)
  {
    case bb_8_bits:
      write_enum_impl<xcdr_v2_stream,T,uint8_t>(str, towrite, N);
      break;
    case bb_16_bits:
      write_enum_impl<xcdr_v2_stream,T,uint16_t>(str, towrite, N);
      break;
    case bb_32_bits:
      write_enum_impl<xcdr_v2_stream,T,uint32_t>(str, towrite, N);
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
void move(xcdr_v2_stream& str, const T&, size_t N = 1) {
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
void max(xcdr_v2_stream& str, const T& max_sz, size_t N = 1) {
  move(str, max_sz, N);
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
#endif