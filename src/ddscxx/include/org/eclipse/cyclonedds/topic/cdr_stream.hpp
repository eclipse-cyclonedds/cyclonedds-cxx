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
#ifndef CDR_STREAM_HPP_
#define CDR_STREAM_HPP_

#include "cdr_dummys.hpp"
#include "dds/ddsrt/endian.h"
#include <org/eclipse/cyclonedds/core/type_helpers.hpp>
#include <stdint.h>
#include <stdexcept>
#include <dds/core/macros.hpp>

/**
* Byte swapping function, is only enabled for arithmetic (base) types.
*/
template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void byte_swap(T& toswap) {
    union { T a; uint16_t u2; uint32_t u4; uint64_t u8; } u;
    u.a = toswap;
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
    toswap = u.a;
}

/**
* Sets value of to to from, will thereafter swap the bytes of to, is sw equals true.
*/
template<typename T, typename = std::enable_if_t<std::is_arithmetic<T>::value> >
void transfer_and_swap(const T& from, T& to, bool sw) {

  to = from;

  if (sw &&
    sizeof(T) > 1)
    byte_swap(to);
}

/**
* Endianness types.
*/
enum class endianness {
    little_endian = DDSRT_LITTLE_ENDIAN,
    big_endian = DDSRT_BIG_ENDIAN
};

/**
* Returns the endianness of the local system.
*/
constexpr endianness native_endianness() { return endianness(DDSRT_ENDIAN); }

/**
* Base cdr_stream class, implements the functions which all "real" cdr stream implementations will use.
*/
class OMG_DDS_API cdr_stream {
public:
    /**
    * Constructor.
    * Sets the stream endianness to end, and maximum alignment to max_align.
    * Local endianness is implicitly set to the local endianness value.
    */
    cdr_stream(endianness end, size_t max_align) : m_stream_endianness(end), m_max_alignment(max_align) { ; }

    /**
    * Returns the current stream alignment.
    */
    size_t alignment() const { return m_current_alignment; }

    /**
    * Sets the new stream alignment.
    */
    size_t alignment(size_t newalignment) { return m_current_alignment = newalignment; }

    /**
    * Returns the current cursor offset.
    */
    size_t position() const { return m_position; }

    /**
    * Returns the position value after this operation.
    */
    size_t position(size_t newposition) { return m_position = newposition; }

    /**
    * Moves the current position offset by incr_by if it is not at SIZE_MAX.
    * Returns the position value after this operation.
    */
    size_t incr_position(size_t incr_by) { if (m_position != SIZE_MAX) m_position += incr_by; return m_position; }

    /**
    * Resets the current cursor position and alignment to 0.
    */
    void reset_position() { m_position = 0; m_current_alignment = 0; }

    /**
    * Sets the buffer pointer to toset.
    * As a side effect, the current position and alignment are reset, since these are not associated with the new buffer.
    */
    void set_buffer(void* toset);

    /**
    * Gets the current cursor pointer.
    * If the current position is SIZE_MAX or the buffer pointer is not set, it returns nullptr.
    */
    char* get_cursor() const { return ((m_position != SIZE_MAX && m_buffer != nullptr) ? (m_buffer + m_position) : nullptr); }

    /**
    * Returns the endianness of the local system.
    */
    endianness local_endianness() const { return m_local_endianness; }

    /**
    * Returns the endianness of the stream.
    */
    endianness stream_endianness() const { return m_stream_endianness; }

    /**
    * Returns true if the stream endianness does not match the local endianness.
    */
    bool swap_endianness() const { return m_stream_endianness == m_local_endianness; }

    /**
    * Aligns the current stream to newalignment: moves the cursor be at newalignment;
    * Aligns to maximum m_max_alignment (which is stream-type specific).
    *
    * Zeroes the bytes the cursor is moved if add_zeroes is true.
    *
    * Nothing happens if the stream is already aligned to newalignment.
    */
    size_t align(size_t newalignment, bool add_zeroes);

protected:

    endianness m_stream_endianness, //the endianness of the stream
        m_local_endianness = native_endianness();  //the local endianness
    size_t m_position = 0,  //the current offset position in the stream
        m_max_alignment,  //the maximum bytes that can be aligned to
        m_current_alignment = 1;  //the current alignment
    char* m_buffer = nullptr;  //the current buffer in use
};

#endif
