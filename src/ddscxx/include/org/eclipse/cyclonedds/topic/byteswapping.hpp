/*
 * Copyright(c) 2006 to 2020 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#ifndef IDL_BYTESWAPPING_HPP
#define IDL_BYTESWAPPING_HPP

namespace org
{
  namespace eclipse
  {
    namespace cyclonedds
    {
      namespace topic
      {

        inline void bswap_16(void* in)
        {
          uint16_t& tmp = *(static_cast<uint16_t*>(in));
          //cast to 16 bit type done here to suppress warnings of conversion to 32 bit type in bit shift
          tmp = static_cast<uint16_t>(tmp >> 8) | static_cast<uint16_t>(tmp << 8);
        }

        inline void bswap_32(void* in)
        {
          uint32_t& tmp = *(static_cast<uint32_t*>(in));
          // swap adjacent 16-bit blocks
          tmp = ((tmp & 0xFFFF0000) >> 16) | ((tmp & 0x0000FFFF) << 16);
          // swap adjacent 8-bit blocks
          tmp = ((tmp & 0xFF00FF00) >> 8) | ((tmp & 0x00FF00FF) << 8);
        }

        inline void bswap_64(void* in)
        {
          uint64_t& tmp = *(static_cast<uint64_t*>(in));
          // swap adjacent 32-bit blocks
          tmp = (tmp >> 32) | (tmp << 32);
          // swap adjacent 16-bit blocks
          tmp = ((tmp & 0xFFFF0000FFFF0000) >> 16) | ((tmp & 0x0000FFFF0000FFFF) << 16);
          // swap adjacent 8-bit blocks
          tmp = ((tmp & 0xFF00FF00FF00FF00) >> 8) | ((tmp & 0x00FF00FF00FF00FF) << 8);
        }

      }
    }
  }
}

#endif
