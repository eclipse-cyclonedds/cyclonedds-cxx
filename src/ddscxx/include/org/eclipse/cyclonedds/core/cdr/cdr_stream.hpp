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
#include <stdint.h>
#include <stdexcept>
#include <dds/core/macros.hpp>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

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
        u.u2 = static_cast<uint16_t>((u.u2 & 0xFF00) >> 8) | static_cast<uint16_t>((u.u2 & 0x00FF) << 8);
        break;
    case 4:
        u.u4 = static_cast<uint32_t>((u.u4 & 0xFFFF0000) >> 16) | static_cast<uint32_t>((u.u4 & 0x0000FFFF) << 16);
        u.u4 = static_cast<uint32_t>((u.u4 & 0xFF00FF00) >> 8) | static_cast<uint32_t>((u.u4 & 0x00FF00FF) << 8);
        break;
    case 8:
        u.u8 = static_cast<uint64_t>((u.u8 & 0xFFFFFFFF00000000) >> 32) | static_cast<uint64_t>((u.u8 & 0x00000000FFFFFFFF) << 32);
        u.u8 = static_cast<uint64_t>((u.u8 & 0xFFFF0000FFFF0000) >> 16) | static_cast<uint64_t>((u.u8 & 0x0000FFFF0000FFFF) << 16);
        u.u8 = static_cast<uint64_t>((u.u8 & 0xFF00FF00FF00FF00) >> 8) | static_cast<uint64_t>((u.u8 & 0x00FF00FF00FF00FF) << 8);
        break;
    default:
        throw std::invalid_argument(std::string("attempted byteswap on variable of invalid size: ") + std::to_string(sizeof(T)));
    }
    DDSCXX_WARNING_MSVC_ON(6326)
    toswap = u.a;
}

/**
 * @brief
 * Transfer and optional byte swapping function.
 *
 * Will copy a primitive type and optionally do a byte swap.
 *
 * @param[in] from The variable to copy from.
 * @param[out] to The variable to copy to.
 * @param[in] sw If true, then the bytes in to will be swapped after copying.
 */
template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void transfer_and_swap(const T& from, T& to, bool sw) {

  to = from;

  if (sw && sizeof(T) > 1)
    byte_swap(to);
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
 */
enum class serialization_status {
  move_bound_exceeded   = 0x1 << 0,
  write_bound_exceeded  = 0x1 << 1,
  read_bound_exceeded   = 0x1 << 2,
  illegal_field_value   = 0x1 << 3
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
     * Resets the current cursor position and alignment to 0.
     */
    void reset_position() { m_position = 0; m_current_alignment = 0; }

    /**
     * @brief
     * Buffer set function.
     *
     * Sets the buffer pointer to toset.
     * As a side effect, the current position and alignment are reset, since these are not associated with the new buffer.
     *
     * @param[in] toset The new pointer of the buffer to set.
     */
    void set_buffer(void* toset);

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
     * Stream endianness getter.
     *
     * This is used to determine whether the data read or written from the stream needs to have their bytes swapped.
     *
     * @return The stream endianness.
     */
    endianness stream_endianness() const { return m_stream_endianness; }

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
     * @return The number of bytes that the cursor was moved.
     */
    size_t align(size_t newalignment, bool add_zeroes);

    /**
     * @brief
     * Returns the current status of serialization.
     *
     * Can be a composition of multiple bit fields from serialization_status.
     *
     * @retval The current status of serialization.
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
protected:

    endianness m_stream_endianness, //the endianness of the stream
        m_local_endianness = native_endianness();  //the local endianness
    size_t m_position = 0,  //the current offset position in the stream
        m_max_alignment,  //the maximum bytes that can be aligned to
        m_current_alignment = 1;  //the current alignment
    char* m_buffer = nullptr;  //the current buffer in use
    uint64_t m_status = 0,  //the current status of streaming
             m_fault_mask;  //the mask for statuses that will causes streaming to be aborted
};

}
}
}
}
} /* namespace org / eclipse / cyclonedds / core / cdr */
#endif
