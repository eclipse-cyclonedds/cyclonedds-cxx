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
#ifndef CDR_STREAM_HPP_
#define CDR_STREAM_HPP_

#include "dds/ddsrt/endian.h"
#include <org/eclipse/cyclonedds/core/type_helpers.hpp>
#include <org/eclipse/cyclonedds/core/cdr/entity_properties.hpp>
#include <stdint.h>
#include <stdexcept>
#include <stack>
#include <cassert>
#include <string>
#include <dds/core/macros.hpp>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

/**
 * @brief
 * Enum conversion and validation function template forward declaration.
 *
 * This function is generated for each enumerated class encountered in the parsed .idl files.
 * Converts an integer value to the corresponding enum class value, or its default value
 * if there is no enum equivalent to the int.
 *
 * @param[in] in The integer to convert to the enumerated class.
 *
 * @return The enumerator representation of in.
 */
template<typename E>
E enum_conversion(uint32_t in);

/**
 * @brief
 * Byte swapping function, is only enabled for arithmetic (base) types.
 *
 * Determines the number of bytes to swap by the size of the template parameter.
 *
 * @param[in, out] toswap The entity whose bytes will be swapped.
 */
template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void byte_swap(T& toswap) {
    union { T a; uint16_t u2; uint32_t u4; uint64_t u8; } u;
    u.a = toswap;
    DDSCXX_WARNING_MSVC_OFF(6326)
    switch (sizeof(T)) {
    case 1:
        break;
    case 2:
        u.u2 = static_cast<uint16_t>((u.u2 & 0xFF00) >> 8)
             | static_cast<uint16_t>((u.u2 & 0x00FF) << 8);
        break;
    case 4:
        u.u4 = static_cast<uint32_t>((u.u4 & 0xFFFF0000) >> 16)
             | static_cast<uint32_t>((u.u4 & 0x0000FFFF) << 16);
        u.u4 = static_cast<uint32_t>((u.u4 & 0xFF00FF00) >> 8)
             | static_cast<uint32_t>((u.u4 & 0x00FF00FF) << 8);
        break;
    case 8:
        u.u8 = static_cast<uint64_t>((u.u8 & 0xFFFFFFFF00000000) >> 32)
             | static_cast<uint64_t>((u.u8 & 0x00000000FFFFFFFF) << 32);
        u.u8 = static_cast<uint64_t>((u.u8 & 0xFFFF0000FFFF0000) >> 16)
             | static_cast<uint64_t>((u.u8 & 0x0000FFFF0000FFFF) << 16);
        u.u8 = static_cast<uint64_t>((u.u8 & 0xFF00FF00FF00FF00) >> 8)
             | static_cast<uint64_t>((u.u8 & 0x00FF00FF00FF00FF) << 8);
        break;
    default:
        throw std::invalid_argument(std::string("attempted byteswap on variable of invalid size: ") + std::to_string(sizeof(T)));
    }
    DDSCXX_WARNING_MSVC_ON(6326)
    toswap = u.a;
}

/**
 * @brief
 * Endianness types.
 *
 * @enum endianness C++ implementation of cyclonedds's DDSRT_ENDIAN endianness defines
 *
 * @var endianness::little_endian Little endianness.
 * @var endianness::big_endian Big endianness.
 */
enum class endianness {
    little_endian = DDSRT_LITTLE_ENDIAN,
    big_endian = DDSRT_BIG_ENDIAN
};

/**
 * @brief
 * Returns the endianness of the local system.
 *
 * Takes the value from the DDSRT_ENDIAN definition and converts it to the c++ enum class value.
 *
 * @retval little_endian If the system is little endian.
 * @retval big_endian If the system is big endian.
 */
constexpr endianness native_endianness() { return endianness(DDSRT_ENDIAN); }

/**
 * @brief
 * Serialization status bitmasks.
 *
 * @enum serialization_status Describes the serialization status of a cdr stream.
 *
 * These are stored as an bitfields in an int in cdr streams, since more than one serialization fault can be encountered.
 *
 * @var serialization_status::move_bound_exceeded The serialization has encountered a field which has exceeded the bounds set for it.
 * @var serialization_status::write_bound_exceeded The serialization has encountered a field which has exceeded the bounds set for it.
 * @var serialization_status::read_bound_exceeded The serialization has encountered a field which has exceeded the bounds set for it.
 * @var serialization_status::illegal_field_value The serialization has encountered a field with a value which should never occur in a valid CDR stream.
 * @var serialization_status::invalid_pl_entry The serialization has encountered a parameter list id which is illegal (not extended id in reserved for OMG space).
 * @var serialization_status::unsupported_xtypes A streamer has attempted to stream a struct requiring xtypes but not supporting it itself.
 * @var serialization_status::must_understand_fail A struct being read contains a field that must be understood but does not recognize or have.
 */
enum serialization_status : uint64_t {
  move_bound_exceeded   = 0x1 << 0,
  write_bound_exceeded  = 0x1 << 1,
  read_bound_exceeded   = 0x1 << 2,
  invalid_pl_entry      = 0x1 << 3,
  illegal_field_value   = 0x1 << 4,
  unsupported_xtypes    = 0x1 << 5,
  must_understand_fail  = 0x1 << 6
};

/**
 * @brief
 * Base cdr_stream class.
 *
 * This class implements the base functions which all "real" cdr stream implementations will use.
 */
class OMG_DDS_API cdr_stream {
public:
    /**
     * @brief
     * Constructor.
     *
     * Sets the stream endianness to end, and maximum alignment to max_align.
     *
     * @param[in] end The endianness to set for the data stream, default to the local system endianness.
     * @param[in] max_align The maximum size that the stream will align CDR primitives to.
     * @param[in] ignore_faults Bitmask for ignoring faults, can be composed of bit fields from the serialization_status enumerator.
     */
    cdr_stream(endianness end, size_t max_align, uint64_t ignore_faults = 0x0) : m_stream_endianness(end), m_max_alignment(max_align), m_fault_mask(~ignore_faults) { ; }

    /**
     * @brief
     * Returns the current stream alignment.
     *
     * @return The current stream alignment.
     */
    size_t alignment() const { return m_current_alignment; }

    /**
     * @brief
     * Sets the new stream alignment.
     *
     * Also returns the value the alignment has been set to.
     *
     * @param[in] newalignment The new alignment to set.
     *
     * @return The value the alignment has been set to.
     */
    size_t alignment(size_t newalignment) { return m_current_alignment = newalignment; }

    /**
     * @brief
     * Checks whether a delimited cdr stream is not being read out of bounds.
     *
     * This function will return true if N bytes can be read from the stream.
     *
     * @param[in] N The number of bytes requested.
     * @param[in] peek Whether this is true access, or just a "peek".
     *
     * @return Whether enough bytes are available for another header.
     */
    bool bytes_available(size_t N = 1, bool peek = false);

    /**
     * @brief
     * Returns the current cursor offset.
     *
     * @retval SIZE_MAX In this case, a maximum size calculation was being done, and the maximum size was determined to be unbounded.
     * @return The current cursor offset.
     */
    size_t position() const { return m_position; }

    /**
     * @brief
     * Sets the new cursor offset.
     *
     * Also returs the value the offset has been set to.
     *
     * @param[in] newposition The new offset to set.
     *
     * @return The value the offset has been set to.
     */
    size_t position(size_t newposition) { return m_position = newposition; }

    /**
     * @brief
     * Cursor move function.
     *
     * Moves the current position offset by incr_by if it is not at SIZE_MAX.
     * Returns the position value after this operation.
     *
     * @param[in] incr_by The amount to move the cursor position by.
     *
     * @return The cursor position after this operation.
     */
    size_t incr_position(size_t incr_by) { if (m_position != SIZE_MAX) m_position += incr_by; return m_position; }

    /**
     * @brief
     * Resets the state of the stream as before streaming began.
     *
     * Will set the current offset, alignment to 0, clear the stack and fault status.
     * Will retain the buffer pointer and size.
     */
    virtual void reset();

    /**
     * @brief
     * Buffer set function.
     *
     * Sets the buffer pointer to toset.
     * As a side effect, the current position and alignment are reset, since these are not associated with the new buffer.
     *
     * @param[in] toset The new pointer of the buffer to set.
     * @param[in] buffer_size The size of the buffer being set.
     */
    void set_buffer(void* toset, size_t buffer_size = SIZE_MAX);

    /**
     * @brief
     * Gets the current cursor pointer.
     *
     * If the current position is SIZE_MAX or the buffer pointer is not set, it returns nullptr.
     *
     * @retval nullptr If the current buffer is not set, or if the cursor offset is not valid.
     * @return The current cursor pointer.
     */
    char* get_cursor() const { return ((m_position != SIZE_MAX && m_buffer != nullptr) ? (m_buffer + m_position) : nullptr); }

    /**
     * @brief
     * Local system endianness getter.
     *
     * This is used to determine whether the data read or written from the stream needs to have their bytes swapped.
     *
     * @return The local endianness.
     */
    endianness local_endianness() const { return m_local_endianness; }

    /**
     * @brief
     * Const stream endianness getter (const).
     *
     * This is used to determine whether the data read or written from the stream needs to have their bytes swapped.
     *
     * @return The stream endianness.
     */
    const endianness& stream_endianness() const { return m_stream_endianness; }

    /**
     * @brief
     * Stream endianness getter.
     *
     * This is used to determine whether the data read or written from the stream needs to have their bytes swapped.
     *
     * @return The stream endianness.
     */
    endianness& stream_endianness() { return m_stream_endianness; }

    /**
     * @brief
     * Determines whether the local and stream endianness are the same.
     *
     * This is used to determine whether the data read or written from the stream needs to have their bytes swapped.
     *
     * @retval false If the stream endianness DOES match the local endianness.
     * @retval true If the stream endianness DOES NOT match the local endianness.
     */
    bool swap_endianness() const { return m_stream_endianness != m_local_endianness; }

    /**
     * @brief
     * Aligns the current stream to a new alignment.
     *
     * Aligns the current stream to newalignment, moves the cursor be at newalignment.
     * Aligns to maximum m_max_alignment (which is stream-type specific).
     * Zeroes the bytes the cursor is moved if add_zeroes is true.
     * Nothing happens if the stream is already aligned to newalignment.
     *
     * @param[in] newalignment The new alignment to align the stream to.
     * @param[in] add_zeroes Whether the bytes that the cursor moves need to be zeroed.
     *
     * @return Whether the cursor could be moved by the required amount.
     */
    bool align(size_t newalignment, bool add_zeroes);

    /**
     * @brief
     * Returns the current status of serialization.
     *
     * Can be a composition of multiple bit fields from serialization_status.
     *
     * @return The current status of serialization.
     */
    uint64_t status() const { return m_status; }

    /**
     * @brief
     * Serialization status update function.
     *
     * Adds to the current status of serialization and returns whether abort status has been reached.
     *
     * @param[in] toadd The serialization status error to add.
     *
     * @retval false If the serialization status of the stream HAS NOT YET reached one of the serialization errors which it is not set to ignore.
     * @retval true If the serialization status of the stream HAS reached one of the serialization errors which it is not set to ignore.
     */
    bool status(serialization_status toadd) { m_status |= static_cast<uint64_t>(toadd); return abort_status(); }

    /**
     * @brief
     * Returns true when the stream has encountered an error which it is not set to ignore.
     *
     * All streaming functions should become NOOPs after this status is encountered.
     *
     * @retval false If the serialization status of the stream HAS NOT YET reached one of the serialization errors which it is not set to ignore.
     * @retval true If the serialization status of the stream HAS reached one of the serialization errors which it is not set to ignore.
     */
    bool abort_status() const { return m_status & m_fault_mask; }

    /**
     * @brief
     * Type of streaming operation to be done.
     *
     * @var stream_mode::unset The stream mode is not set.
     * @var stream_mode::read Reads from the stream into an instance.
     * @var stream_mode::write Writes from the instance to the stream.
     * @var stream_mode::move Moves the cursor by the same amount as would has been done through stream_mode::write, without copying any data to the stream.
     * @var stream_mode::max Same as stream_mode::move, but by the maximum amount possible for an entity of that type.
     */
    enum class stream_mode {
      unset,
      read,
      write,
      move,
      max
    };

    /**
     * @brief
     * Returns whether the streaming is done only over the key values.
     *
     * @return Whether the streaming is done only over the key values.
     */
    bool is_key() const {return m_key;}

    /**
     * @brief
     * Function which sets the current streaming mode.
     *
     * This will impact which entities will be retrieved from the entity properties list.
     * This will also reset the current cursor position.
     *
     * @param[in] mode The streaming mode to set for the stream.
     * @param[in] key The key mode to set for the stream.
     */
    void set_mode(stream_mode mode, bool key) {m_mode = mode; m_key = key; reset();}

    /**
     * @brief
     * Function declaration for starting a new member.
     *
     * This function is called by next_entity for each entity which is iterated over.
     * Depending on the implementation and mode headers may be read from/written to the stream.
     * This function can be overridden in cdr streaming implementations.
     *
     * @param[in] prop Properties of the entity to start.
     * @param[in] is_set Whether the entity represented by prop is present, if it is an optional entity.
     *
     * @return Whether the operation was completed succesfully.
     */
    virtual bool start_member(entity_properties_t &prop, bool is_set = true);

    /**
     * @brief
     * Function declaration for finishing an existing member.
     *
     * This function is called by next_entity for each entity which is iterated over.
     * Depending on the implementation and mode header length fields may be completed.
     * This function can be overridden in cdr streaming implementations.
     *
     * @param[in] prop Properties of the entity to finish.
     * @param[in] is_set Whether the entity represented by prop is present, if it is an optional entity.
     *
     * @return Whether the operation was completed succesfully.
     */
    virtual bool finish_member(entity_properties_t &prop, bool is_set = true);

    /**
     * @brief
     * Function declaration for skipping entities without involving them on the stack.
     *
     * This function is called by the instance implementation switchbox, when it encounters an id which
     * does not resolve to an id pointing to a member it knows. It will move to the next entity in the
     * stream.
     */
    virtual void skip_entity();

    /**
     * @brief
     * Function declaration for retrieving the next entity to be operated on by the streamer.
     *
     * This function is called by the instance implementation switchbox and will return the next entity to operate on by calling next_prop.
     * This will also call the implementation specific push/pop entity functions to write/finish headers where necessary.
     *
     * @param[in, out] props The property tree to get the next entity from.
     * @param[in, out] firstcall Whether it is the first time calling the function for props, will store first iterator if true, and then set to false.
     *
     * @return The next entity to be processed, or the final entity if the current tree level does not hold more entities.
     */
    virtual entity_properties_t& next_entity(entity_properties_t &props, bool &firstcall) = 0;

    /**
     * @brief
     * Function declaration for starting a parameter list.
     *
     * This function is called by the generated functions for the entity, and will trigger the necessary actions on starting a new struct.
     * I.E. starting a new parameter list, writing headers.
     *
     * @param[in, out] props The entity whose members might be represented by a parameter list.
     *
     * @return Whether the operation was completed succesfully.
     */
    virtual bool start_struct(entity_properties_t &props);

    /**
     * @brief
     * Function declaration for finishing a parameter list.
     *
     * This function is called by the generated functions for the entity, and will trigger the necessary actions on finishing the current struct.
     * I.E. finishing headers, writing length fields.
     *
     * @param[in, out] props The entity whose members might be represented by a parameter list.
     *
     * @return Whether the struct is complete and correct.
     */
    virtual bool finish_struct(entity_properties_t &props);

    /**
     * @brief
     * Function declaration for starting an array or sequence of non-primitive types.
     *
     * This function is used to keep track of whether delimiters need to be and have been written to the stream.
     * This function is an effective no-op for all streamers except xcdr_v2.
     *
     * @param[in] is_array True when the consecutive entries is an array, false when it is a sequence.
     * @param[in] primitive Whether the consecutive entities are primitives (base types, not enums, strings, typedefs and arrays are resolved though)
     *
     * @return Always true.
     */
    virtual bool start_consecutive(bool is_array, bool primitive) { (void) is_array; (void) primitive; return true;}

    /**
     * @brief
     * Function declaration for finishing an array or sequence of non-primitive types.
     *
     * This function is an effective no-op for all streamers except xcdr_v2.
     *
     * @return Always true.
     */
    virtual bool finish_consecutive() {return true;}

protected:

    /**
     * @brief
     * Member list types
     *
     * @enum member_list_type Which type of list of entries is to be iterated over,
     * used in calls to cdr_stream::next_prop.
     *
     * @var member_list_type::member_by_seq Member entries in order of declaration.
     * @var member_list_type::member_by_id Member entries sorted by member id.
     * @var member_list_type::key Key entries sorted by member id.
     */
    enum class member_list_type {
      member_by_seq,
      member_by_id,
      key
    };

    /**
     * @brief
     * Records the start of a member entry.
     *
     * Will record the member start and set the member present flag to true.
     *
     * @param[in, out] prop The member whose start is recorded.
     */
    void record_member_start(entity_properties_t &prop);

    /**
     * @brief
     * Skips a member entry.
     *
     * In the case a read has failed, this will go to the next member.
     */
    void go_to_next_member();

    /**
     * @brief
     * Checks the struct for completeness.
     *
     * Checks whether all fields which must be understood are present.
     *
     * @param[in, out] props The struct whose start is recorded.
     * @param[in] list_type Which list must be checked for entries.
     */
    void check_struct_completeness(entity_properties_t &props, member_list_type list_type);

    /**
     * @brief
     * Function for retrieving the next entity to be operated on by the streamer.
     *
     * When it is called the first time, the iterator of the (member/key)entities to be iterated over is stored on the stack.
     * This iterator is then used in successive calls, until the end of the valid entities is reached, at which point the iterator is popped off the stack.
     * This function is to be implemented in cdr streaming implementations.
     *
     * @param[in, out] props The property tree to get the next entity from.
     * @param[in] list_type Which list to take the next property from
     * @param[in, out] firstcall Whether it is the first time calling the function for props, will store first iterator if true, and then set to false.
     *
     * @return The next entity to be processed, or the final entity if the current tree level does not hold more entities.
     */
    entity_properties_t& next_prop(entity_properties_t &props, member_list_type list_type, bool &firstcall);

    endianness m_stream_endianness,               /**< the endianness of the stream*/
        m_local_endianness = native_endianness(); /**< the local endianness*/
    size_t m_position = 0,                        /**< the current offset position in the stream*/
        m_max_alignment,                          /**< the maximum bytes that can be aligned to*/
        m_current_alignment = 1,                  /**< the current alignment*/
        m_buffer_size = 0;                        /**< the size of the current buffer*/
    char* m_buffer = nullptr;                     /**< the current buffer in use*/
    uint64_t m_status = 0,                        /**< the current status of streaming*/
             m_fault_mask;                        /**< the mask for statuses that will cause streaming
                                                       to be aborted*/
    stream_mode m_mode = stream_mode::unset;      /**< the current streaming mode*/
    bool m_key = false;                           /**< the current key mode*/

    entity_properties_t m_final = final_entry();  /**< A placeholder for the final entry to be returned
                                                       from next_prop if we are reading from a stream*/
    entity_properties_t m_current_header;         /**< Container for header information being read from a stream*/

    DDSCXX_WARNING_MSVC_OFF(4251)
    std::stack<size_t> m_buffer_end;              /**< the end of reading at the current level*/
    std::stack<uint32_t> m_entity_offsets;        /**< the entity offsets at the current level*/
    std::stack<uint32_t> m_entity_sizes;          /**< the entity sizes at the current level*/
    std::stack<proplist::iterator> m_stack;       /**< Stack of iterators currently being handled*/
    DDSCXX_WARNING_MSVC_ON(4251)
};

 /**
 * @brief
 * Sequence type stream manipulation functions
 *
 * These will "unwrap" the sequence type. Bounds checking and length field streaming
 * are done before calling the corresponding streaming function on the contained
 * entities.
 */

/**
 * @brief
 * Sequence type read function.
 *
 * Reads the sequence length from the stream and checks whether the
 * maximum length does not exceed any bounds of the sequence.
 * It will resize the container and call reads for all entities in
 * the sequence.
 * This function is only enabled for non-arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read into.
 * @param[in, out] props The properties of the variable being read.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, typename> class V,
          typename T,
          typename A,
          std::enable_if_t<!std::is_arithmetic<T>::value, bool> = true >
bool read_sequence(S &str, V<T, A>& toread, entity_properties_t &props, const size_t *max_sz)
{
  uint32_t vec_length = 0;
  if (!str.start_consecutive(false, false) ||
      !read(str,vec_length))
    return false;

  //do length checks (if necessary)
  uint32_t read_length = vec_length;
  if (max_sz && *max_sz)
    vec_length = std::min<uint32_t>(static_cast<uint32_t>(*max_sz),vec_length);

  //do read for entries in vector
  toread.resize(vec_length);
  for (auto &e:toread) {
    if (!read(str, e, props, max_sz ? max_sz+1 : nullptr))
      return false;
  }

  //dummy reads for entries beyond the sequence's maximum size
  for (uint32_t i = vec_length; i < read_length; i++) {
    T dummy;
    if (!read(str, dummy, props, max_sz ? max_sz+1 : nullptr))
      return false;
  }

  return str.finish_consecutive();
}

/**
 * @brief
 * Sequence type read function.
 *
 * Reads the sequence length from the stream and checks whether the
 * maximum length does not exceed any bounds of the sequence.
 * It will resize the container and call the primitive type read
 * function as there a bulk copy is done.
 * This function is only enabled for arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read into.
 * @param[in, out] props The properties of the variable being read.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, typename> class V,
          typename T,
          typename A,
          std::enable_if_t<std::is_arithmetic<T>::value, bool> = true >
bool read_sequence(S &str, V<T, A>& toread, entity_properties_t &props, const size_t *max_sz)
{
  uint32_t vec_length = 0;
  if (!str.start_consecutive(false, props.is_primitive_type) ||
      !read(str,vec_length))
    return false;

  //do length checks (if necessary)
  uint32_t read_length = vec_length;
  if (max_sz && *max_sz)
    vec_length = std::min<uint32_t>(static_cast<uint32_t>(*max_sz),vec_length);

  //do read for entries in vector
  toread.resize(vec_length);
  if (read_length && !read(str, toread[0], props, max_sz, vec_length))
    return false;

  //dummy reads for entries beyond the sequence's maximum size
  if (read_length > vec_length &&
      !move(str, T(), props, max_sz, read_length-vec_length))
    return false;

  return str.finish_consecutive();
}

/**
 * @brief
 * Sequence type write function.
 *
 * Checks whether the maximum length does not exceed any bounds of
 * the sequence and then writes the sequence length to the stream.
 * It will then call reads for all entities in the sequence.
 * This function is only enabled for non-arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream which is written to.
 * @param[in] towrite The variable to write.
 * @param[in, out] props The properties of the variable being written.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, typename> class V,
          typename T,
          typename A,
          std::enable_if_t<!std::is_arithmetic<T>::value, bool> = true >
bool write_sequence(S &str, const V<T, A>& towrite, entity_properties_t &props, const size_t *max_sz)
{
  if (!str.start_consecutive(false, false) ||
      (max_sz && *max_sz && towrite.size() > *max_sz) ||
      !write(str,static_cast<uint32_t>(towrite.size())))
    return false;

  //do write for entries in vector
  for (const auto & w:towrite) {
    if (!write(str, w, props, max_sz ? max_sz+1 : nullptr))
      return false;
  }

  return str.finish_consecutive();
}

/**
 * @brief
 * Sequence type write function.
 *
 * Checks whether the maximum length does not exceed any bounds of
 * the sequence and then writes the sequence length to the stream.
 * It will then call the primitive type write function as there the
 * bulk copy is done.
 * This function is only enabled for arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream which is written to.
 * @param[in] towrite The variable to write.
 * @param[in, out] props The properties of the variable being written.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, typename> class V,
          typename T,
          typename A,
          std::enable_if_t<std::is_arithmetic<T>::value, bool> = true >
bool write_sequence(S &str, const V<T, A>& towrite, entity_properties_t &props, const size_t *max_sz)
{
  if (!str.start_consecutive(false, props.is_primitive_type) ||
      (max_sz && *max_sz && towrite.size() > *max_sz) ||
      !write(str,static_cast<uint32_t>(towrite.size())))
    return false;

  //do write for entries in vector
  return (0 == towrite.size() || write(str, towrite[0], props, max_sz, towrite.size())) &&
         str.finish_consecutive();
}

/**
 * @brief
 * Sequence type move function.
 *
 * Checks whether the maximum length does not exceed any bounds of
 * the sequence and then moves the stream by the sequence number.
 * It will then call the move for all entities in the sequence.
 * This function is only enabled for non-arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] tomove The variable the cursor is moved by.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, typename> class V,
          typename T,
          typename A,
          std::enable_if_t<!std::is_arithmetic<T>::value, bool> = true >
bool move_sequence(S &str, const V<T, A>& tomove, entity_properties_t &props, const size_t *max_sz)
{
  if (!str.start_consecutive(false, false) ||
      (max_sz && *max_sz && tomove.size() > *max_sz) ||
      !move(str,uint32_t()))
    return false;

  //do move for entries in vector
  for (const auto & w:tomove) {
    if (!move(str, w, props, max_sz ? max_sz+1 : nullptr))
      return false;
  }

  return str.finish_consecutive();
}

/**
 * @brief
 * Sequence type move function.
 *
 * Checks whether the maximum length does not exceed any bounds of
 * the sequence and then moves the stream for the length field.
 * It will then call the primitive type move function as there the
 * simple move is done.
 * This function is only enabled for arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] tomove The variable the cursor is moved by.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, typename> class V,
          typename T,
          typename A,
          std::enable_if_t<std::is_arithmetic<T>::value, bool> = true >
bool move_sequence(S &str, const V<T, A>& tomove, entity_properties_t &props, const size_t *max_sz)
{
  if (!str.start_consecutive(false, props.is_primitive_type) ||
      (max_sz && *max_sz && tomove.size() > *max_sz) ||
      !move(str,uint32_t()))
    return false;

  //do move for entries in vector
  return (0 == tomove.size() || move(str, tomove[0], props, max_sz, tomove.size())) &&
         str.finish_consecutive();
}

/**
 * @brief
 * Sequence type max function.
 *
 * This will either set the cursor to SIZE_MAX if no max_sz is provided
 * or max_sz points to 0, otherwise it will call a max operation for the
 * maximum number of entries in the bounded function.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, typename> class V,
          typename T,
          typename A>
bool max_sequence(S &str, const V<T, A>&, entity_properties_t &props, const size_t *max_sz)
{
  T dummy;
  for (size_t i = 0; max_sz && i < *max_sz; i++) {
    if (!max(str, dummy, props, max_sz))
      return false;
  }

  if (!max_sz || !*max_sz)
    str.position(SIZE_MAX);

  return true;
}

 /**
 * @brief
 * Array type stream manipulation functions
 *
 * These will "unwrap" the array type and call the corresponding streaming
 * function on the contained entities.
 */

/**
 * @brief
 * Array type read function.
 *
 * It will call reads for all entities in the array.
 * This function is only enabled for non-arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read into.
 * @param[in, out] props The properties of the variable being read.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, size_t> class A,
          typename T,
          size_t N,
          std::enable_if_t<!std::is_arithmetic<T>::value, bool> = true >
bool read_array(S &str, A<T, N>& toread, entity_properties_t &props, const size_t *max_sz)
{
  if (!str.start_consecutive(true, props.is_primitive_type))
    return false;

  //do read for entries in array
  for (auto & r:toread) {
    if (!read(str, r, props, max_sz))
      return false;
  }

  return str.finish_consecutive();
}

/**
 * @brief
 * Array type read function.
 *
 * It will call the primitive type read function as there a bulk copy is done.
 * This function is only enabled for arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read into.
 * @param[in, out] props The properties of the variable being read.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, size_t> class A,
          typename T,
          size_t N,
          std::enable_if_t<std::is_arithmetic<T>::value, bool> = true >
bool read_array(S &str, A<T,N>& toread, entity_properties_t &props, const size_t *max_sz)
{
  return str.start_consecutive(true, props.is_primitive_type) &&
         read(str, toread[0], props, max_sz, N) &&
         str.finish_consecutive();
}

/**
 * @brief
 * Array type write function.
 *
 * It will call writes for all entities in the array.
 * This function is only enabled for non-arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream which is written to.
 * @param[out] towrite The variable to write.
 * @param[in, out] props The properties of the variable being written.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, size_t> class A,
          typename T,
          size_t N,
          std::enable_if_t<!std::is_arithmetic<T>::value, bool> = true >
bool write_array(S &str, const A<T, N>& towrite, entity_properties_t &props, const size_t *max_sz)
{
  if (!str.start_consecutive(true, props.is_primitive_type))
    return false;

  //do write for entries in array
  for (const auto & w:towrite) {
    if (!write(str, w, props, max_sz))
      return false;
  }

  return str.finish_consecutive();
}

/**
 * @brief
 * Array type write function.
 *
 * It will call the primitive type write function as there a bulk copy is done.
 * This function is only enabled for arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream which is written to.
 * @param[out] towrite The variable to write.
 * @param[in, out] props The properties of the variable being written.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, size_t> class A,
          typename T,
          size_t N,
          std::enable_if_t<std::is_arithmetic<T>::value, bool> = true >
bool write_array(S &str, const A<T,N>& towrite, entity_properties_t &props, const size_t *max_sz)
{
  return str.start_consecutive(true, props.is_primitive_type) &&
         write(str, towrite[0], props, max_sz, N) &&
         str.finish_consecutive();
}

/**
 * @brief
 * Array type move function.
 *
 * It will call the move for all entities in the sequence.
 * This function is only enabled for non-arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] tomove The variable the cursor is moved by.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, size_t> class A,
          typename T,
          size_t N,
          std::enable_if_t<!std::is_arithmetic<T>::value, bool> = true >
bool move_array(S &str, const A<T, N>& tomove, entity_properties_t &props, const size_t *max_sz)
{
  if (!str.start_consecutive(true, props.is_primitive_type))
    return false;

  //do move for entries in array
  for (const auto & w:tomove) {
    if (!move(str, w, props, max_sz))
      return false;
  }

  return str.finish_consecutive();
}

/**
 * @brief
 * Array type move function.
 *
 * It will call the primitive type move function as there the simple move is done.
 * This function is only enabled for arithmetic types.
 * This function's container template will be specialized in the
 * generated code, as there its type is known.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] tomove The variable the cursor is moved by.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename, size_t> class A,
          typename T,
          size_t N,
          std::enable_if_t<std::is_arithmetic<T>::value, bool> = true >
bool move_array(S &str, const A<T,N>& tomove, entity_properties_t &props, const size_t *max_sz)
{
  return str.start_consecutive(true, props.is_primitive_type) &&
         move(str, tomove[0], props, max_sz, N) &&
         str.finish_consecutive();
}

/**
 * @brief
 * Array type max function.
 *
 * It will call the max function for each entity in the array.
 *
 * @param[in, out] str The stream whose cursor is moved.
 * @param[in] tomax The variable the cursor is moved by.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename,size_t> class A,
          typename T,
          size_t N >
bool max_array(S &str, const A<T, N>& tomax, entity_properties_t &props, const size_t *max_sz)
{
  if (!str.start_consecutive(true, props.is_primitive_type))
    return false;

  //do max for entries in array
  for (const auto & w:tomax) {
    if (!max(str, w, props, max_sz))
      return false;
  }

  return str.finish_consecutive();
}

 /**
 * @brief
 * Optional type stream manipulation functions
 *
 * These are "endpoints" for streaming functions, since compound
 * (sequence/array/constructed type) functions will decay to these
 * calls.
 */

/**
 * @brief
 * Optional type read function.
 *
 * It will initialize the optional container with a default entity
 * if it does not yet contain anything.
 * Then call a read on that entity. As this operation will be
 * skipped if the stream indicates it is not populated, this will not
 * cause extraneous reads.
 *
 * @param[in, out] str The stream being read from.
 * @param[out] toread The variable being read to.
 * @param[in, out] props The properties of the variable being read.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename> class O,
          typename T >
bool read_optional(S &str, O<T> &toread, entity_properties_t &props, const size_t *max_sz)
{
  if (!toread)
    toread = T();

  return read(str, toread.value(), props, max_sz);
}

/**
 * @brief
 * Optional type write function.
 *
 * It will check the container for the presence of a value, and not write if there
 * are no contents.
 *
 * @param[in, out] str The stream being written to.
 * @param[in] towrite The variable being written from.
 * @param[in, out] props The properties of the variable being written.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename> class O,
          typename T >
bool write_optional(S &str, const O<T> &towrite, entity_properties_t &props, const size_t *max_sz)
{
  if (!towrite)
    return true;

  return write(str, towrite.value(), props, max_sz);
}

/**
 * @brief
 * Optional type move function.
 *
 * It will check the container for the presence of a value, and not move if there
 * are no contents in it.
 *
 * @param[in, out] str The stream being moved.
 * @param[in] tomove The variable being moved by.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename> class O,
          typename T >
bool move_optional(S &str, const O<T> &tomove, entity_properties_t &props, const size_t *max_sz)
{
  if (!tomove)
    return true;

  return move(str, tomove.value(), props, max_sz);
}

/**
 * @brief
 * Optional type max function.
 *
 * It will move the stream by the entity in the container.
 *
 * @param[in, out] str The stream being moved.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template< typename S,
          template<typename> class O,
          typename T >
bool max_optional(S &str, const O<T> &, entity_properties_t &props, const size_t *max_sz)
{
  return max(str, T(), props, max_sz);
}

//externals

template< typename S,
          template<typename> class E,
          typename T >
bool read_external(S &str, E<T> &toread, entity_properties_t &props, const size_t *max_sz)
{
  if (!toread)
    toread = new T();

  return read(str, *toread, props, max_sz);
}

template< typename S,
          template<typename> class E,
          typename T >
bool write_external(S &str, const E<T> &towrite, entity_properties_t &props, const size_t *max_sz)
{
  if (!towrite)
    return false;

  return write(str, *towrite, props, max_sz);
}

template< typename S,
          template<typename> class E,
          typename T >
bool move_external(S &str, const E<T> &tomove, entity_properties_t &props, const size_t *max_sz)
{
  if (!tomove)
    return false;

  return move(str, *tomove, props, max_sz);
}

template< typename S,
          template<typename> class E,
          typename T >
bool max_external(S &str, const E<T> &, entity_properties_t &props, const size_t *max_sz)
{
  return max(str, T(), props, max_sz);
}

/**
 * @brief
 * Primitive type stream manipulation functions.
 *
 * These are "endpoints" for streaming functions, since composit
 * (sequence/array/constructed type) functions will decay to these
 * calls.
 */

/**
 * @brief
 * Primitive type read function.
 *
 * Aligns the stream to the alignment of type T.
 * Reads the value from the current position of the stream str into
 * toread, will swap bytes if necessary.
 * Moves the cursor of the stream by the size of T.
 * This function is only enabled for arithmetic types.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read into.
 * @param[in, out] props The properties of the variable being read.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 * @param[in] N The number of entities to read.
 *
 * @return Whether the operation was completed succesfully.
 */
template<typename S, typename T, std::enable_if_t<std::is_arithmetic<T>::value
                                               && std::is_base_of<cdr_stream, S>::value, bool> = true >
bool read(S &str, T& toread, entity_properties_t &props, const size_t *max_sz = nullptr, size_t N = 1)
{
  (void) max_sz;
  assert(N);

  props.is_present = false;
  if (str.position() == SIZE_MAX
   || !str.align(sizeof(T), false)
   || !str.bytes_available(sizeof(T)*N))
    return false;

  auto from = str.get_cursor();
  T *to = &toread;

  assert(from);

  memcpy(to,from,sizeof(T)*N);

  if (str.swap_endianness()) {
    for (size_t i = 0; i < N; i++, to++)
      byte_swap(*to);
  }

  str.incr_position(sizeof(T)*N);

  props.is_present = true;
  return true;
}

/**
 * @brief
 * Primitive type read function (short version).
 *
 * Same as the long version, only no feedback is given through an entity properties parameter.
 * Used as subroutines in larger entity write functions, e.g.: delimiters between members, or
 * length fields for sequences and strings.
 */
template<typename S, typename T, std::enable_if_t<std::is_arithmetic<T>::value
                                               && std::is_base_of<cdr_stream, S>::value, bool> = true >
bool read(S &str, T& toread)
{
  auto props = get_type_props<T>();
  return read(str, toread, props);
}

/**
 * @brief
 * Enum type read function implementation.
 *
 * Uses the template parameter I to determine the stream-end read type,
 * this type is determined by the stream implementation.
 * Reads the enums as type I from the stream.
 * Each read entity is verified by the enum's conversion version.
 * This function is only enabled for enum types.
 *
 * @param[in, out] str The stream which is read from.
 * @param[out] toread The variable to read.
 * @param[in, out] props The properties of the variable being read.
 * @param[in] N The number of entities to read.
 *
 * @return Whether the operation was completed succesfully.
 */
template<typename S, typename T, typename I, std::enable_if_t<std::is_integral<I>::value
                                               && std::is_enum<T>::value
                                               && std::is_base_of<cdr_stream, S>::value, bool> = true>
bool read_enum_impl(S& str, T& toread, entity_properties_t &props, const size_t *, size_t N)
{
  assert(N);
  T *ptr = &toread;
  I holder = 0;
  for (size_t i = 0; i < N; i++, ptr++)
  {
    if (!read(str, holder))
      return false;
    *ptr = enum_conversion<T>(holder);
  }
  props.is_present = true;
  return true;
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
 * @param[in, out] props The properties of the variable being written.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 * @param[in] N The number of entities to write.
 *
 * @return Whether the operation was completed succesfully.
 */
template<typename S, typename T, std::enable_if_t<std::is_arithmetic<T>::value
                                               && std::is_base_of<cdr_stream, S>::value, bool> = true >
bool write(S& str, const T& towrite, entity_properties_t &props, const size_t *max_sz = nullptr, size_t N = 1)
{
  assert(N);
  (void) max_sz;

  props.is_present = false;
  if (str.position() == SIZE_MAX
   || !str.align(sizeof(T), true)
   || !str.bytes_available(sizeof(T)*N))
    return false;

  auto to = reinterpret_cast<T*>(str.get_cursor());
  const T *from = &towrite;

  assert(to);

  memcpy(to,from,sizeof(T)*N);

  if (str.swap_endianness()) {
    for (size_t i = 0; i < N; i++, to++)
      byte_swap(*to);
  }

  str.incr_position(sizeof(T)*N);

  props.is_present = true;
  return true;
}

/**
 * @brief
 * Primitive type write function (short version).
 *
 * Same as the long version, only no feedback is given through an entity properties parameter.
 * Used as subroutines in larger entity write functions, e.g.: delimiters between members, or
 * length fields for sequences and strings.
 */
template<typename S, typename T, std::enable_if_t<std::is_arithmetic<T>::value
                                               && std::is_base_of<cdr_stream, S>::value, bool> = true >
bool write(S& str, const T& towrite)
{
  auto props = get_type_props<T>();
  return write(str, towrite, props);
}

/**
 * @brief
 * Enum type write function implementation.
 *
 * Uses the template parameter I to determine the stream-end write type,
 * this type is determined by the stream implementation.
 * Writes the enums as type I to the stream.
 * If the enums have the same size as the integer stream type, they are written
 * as a block, otherwise they are copied one by one.
 * This function is only enabled for enum types.
 *
 * @param[in, out] str The stream which is written to.
 * @param[in] towrite The variable to write.
 * @param[in] N The number of entities to write.
 *
 * @return Whether the operation was completed succesfully.
 */
template<typename S, typename T, typename I, std::enable_if_t<std::is_integral<I>::value
                                               && std::is_enum<T>::value
                                               && std::is_base_of<cdr_stream, S>::value, bool> = true>
bool write_enum_impl(S& str, const T& towrite, entity_properties_t &props, const size_t *max_sz, size_t N)
{
  assert(N);
  const T *ptr = &towrite;
  props.is_present = false;
  if (sizeof(T) == sizeof(I)) {
      if (!write(str, *reinterpret_cast<const I*>(ptr), props, max_sz, N))
        return false;
  } else {
    for (size_t i = 0; i < N; i++, ptr++)
      if (!write(str, *reinterpret_cast<const I*>(ptr)))
        return false;
  }
  props.is_present = true;
  return true;
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
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 * @param[in] N The number of entities to move.
 *
 * @return Whether the operation was completed succesfully.
 */
template<typename S, typename T, std::enable_if_t<std::is_arithmetic<T>::value
                                               && std::is_base_of<cdr_stream, S>::value, bool> = true >
bool move(S& str, const T&, entity_properties_t &props, const size_t *max_sz = nullptr, size_t N = 1)
{
  assert(N);
  (void) max_sz;

  props.is_present = false;
  if (str.position() != SIZE_MAX) {
    if (!str.align(sizeof(T), false))
      return false;

    str.incr_position(sizeof(T)*N);
  }

  props.is_present = true;
  return true;
}

/**
 * @brief
 * Primitive type move function (short version).
 *
 * Same as the long version, only no feedback is given through an entity properties parameter.
 * Used as subroutines in larger entity write functions, e.g.: delimiters between members, or
 * length fields for sequences and strings.
 */
template<typename S, typename T, std::enable_if_t<std::is_arithmetic<T>::value
                                               && std::is_base_of<cdr_stream, S>::value, bool> = true >
bool move(S& str, const T& tomove)
{
  auto props = get_type_props<T>();
  return move(str, tomove, props);
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
 * @param[in] tomax The variable to move the cursor by, no contents of this variable are used, it is just used to determine the template.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 * @param[in] N The number of entities at most to move.
 *
 * @return Whether the operation was completed succesfully.
 */
template<typename S, typename T, std::enable_if_t<std::is_arithmetic<T>::value
                                               && std::is_base_of<cdr_stream, S>::value, bool> = true >
bool max(S& str, const T& tomax, entity_properties_t &props, const size_t *max_sz = nullptr, size_t N = 1)
{
  assert(N);
  return move(str, tomax, props, max_sz, N);
}

 /**
 * @brief
 * String type stream manipulation functions
 *
 * These are "endpoints" for streaming functions, since compound
 * (sequence/array/constructed type) functions will decay to these
 * calls.
 */

/**
 * @brief
 * Bounded string read function.
 *
 * Reads the length from str, but then initializes toread with at most N characters from it.
 * It does move the cursor by length read, since that is the number of characters in the stream.
 * If max_sz is null or points to 0, then the string is taken to be unbounded.
 * This template function will be specialized for the string class being used.
 *
 * @param[in, out] str The stream to read from.
 * @param[out] toread The string to read to.
 * @param[in, out] props The properties of the variable being read.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template<typename S, typename T >
bool read_string(S& str, T& toread, entity_properties_t &props, const size_t *max_sz)
{
  props.is_present = false;
  if (str.position() == SIZE_MAX)
    return false;

  uint32_t string_length = 0;

  if (!read(str, string_length)
   || !str.bytes_available(string_length))
    return false;

  if (string_length == 0
   && str.status(serialization_status::illegal_field_value))
    return false;

  if (max_sz && *max_sz && string_length > *max_sz + 1)
    return false;

  auto cursor = str.get_cursor();
  toread.assign(cursor, cursor + std::min<size_t>(string_length - 1, (max_sz && *max_sz) ? *max_sz : SIZE_MAX));  //remove 1 for terminating NULL

  str.incr_position(string_length);

  //aligned to chars
  str.alignment(1);

  props.is_present = true;
  return true;
}

/**
 * @brief
 * Bounded string write function.
 *
 * Attempts to write the length of towrite to str, where the bound is checked.
 * Then writes the contents of towrite to str.
 * If max_sz is null or points to 0, then the string is taken to be unbounded.
 * This template function will be specialized for the string class being used.
 *
 * @param[in, out] str The stream to write to.
 * @param[in] towrite The string to write.
 * @param[in, out] props The properties of the variable being written.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template<typename S, typename T >
bool write_string(S& str, const T& towrite, entity_properties_t &props, const size_t *max_sz)
{
  props.is_present = false;
  if (str.position() == SIZE_MAX)
    return false;

  size_t string_length = towrite.length() + 1;  //add 1 extra for terminating NULL

  if (max_sz
   && *max_sz
   && string_length > *max_sz + 1
   && str.status(serialization_status::write_bound_exceeded))
      return false;

  if (!write(str, uint32_t(string_length))
   || !str.bytes_available(string_length))
    return false;

  memcpy(str.get_cursor(), towrite.c_str(), string_length);

  str.incr_position(string_length);

  //aligned to chars
  str.alignment(1);

  props.is_present = true;
  return true;
}

/**
 * @brief
 * Bounded string cursor move function.
 *
 * Attempts to move the cursor for the length field, where the bound is checked.
 * Then moves the cursor for the length of the string.
 * If max_sz is null or points to 0, then the string is taken to be unbounded.
 * This template function will be specialized for the string class being used.
 *
 * @param[in, out] str The stream to move its cursor.
 * @param[in] toincr The string to move the cursor by.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template<typename S, typename T >
bool move_string(S& str, const T &toincr, entity_properties_t &props, const size_t *max_sz)
{
  props.is_present = false;
  if (str.position() != SIZE_MAX) {
    size_t string_length = toincr.length() + 1;  //add 1 extra for terminating NULL

    if (max_sz
     && *max_sz
     && string_length > *max_sz + 1
     && str.status(serialization_status::move_bound_exceeded))
        return false;

    if (!move(str, uint32_t()))
      return false;

    str.incr_position(string_length);

    //aligned to chars
    str.alignment(1);
  }

  props.is_present = true;
  return true;
}

/**
 * @brief
 * Bounded string cursor max move function.
 *
 * Similar to the string move function, with the additional checks that no move
 * is done if the cursor is already at its maximum position, and that the cursor
 * is set to its maximum position if the bound is equal to 0 (unbounded).
 * If max_sz is null or points to 0, then the string is taken to be unbounded.
 * This template function will be specialized for the string class being used.
 *
 * @param[in, out] str The stream to move its cursor.
 * @param[in, out] props The properties of the variable being moved.
 * @param[in] max_sz The array of max sizes used for bounded sequences/strings.
 *
 * @return Whether the operation was completed succesfully.
 */
template<typename S, typename T >
bool max_string(S& str, const T&, entity_properties_t &props, const size_t *max_sz)
{
  props.is_present = false;
  if (!max_sz || !*max_sz) {
    str.position(SIZE_MAX); //unbounded string, theoretical length unlimited
    props.is_present = true;
    return true;
  } else {
    T dummy;
    dummy.resize(*max_sz);
    return move_string(str, dummy, props, max_sz);
  }
}

}
}
}
}
} /* namespace org / eclipse / cyclonedds / core / cdr */
#endif
