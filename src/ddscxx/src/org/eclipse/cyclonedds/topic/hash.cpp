// Copyright(c) 2006 to 2020 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <org/eclipse/cyclonedds/topic/hash.hpp>
#include "dds/ddsrt/md5.h"
#include <cstring>

namespace org
{
  namespace eclipse
  {
    namespace cyclonedds
    {
      namespace topic
      {
        bool simple_key(const std::vector<unsigned char>& in, ddsi_keyhash_t& out)
        {
          memcpy(&out.value, in.data(), in.size());

          return false;
        }

        bool complex_key(const std::vector<unsigned char>& in, ddsi_keyhash_t& out)
        {
          ddsrt_md5_state_t md5st;
          ddsrt_md5_init(&md5st);
          ddsrt_md5_append(&md5st, reinterpret_cast<const ddsrt_md5_byte_t*>(in.data()), static_cast<unsigned int>(in.size()));
          ddsrt_md5_finish(&md5st, reinterpret_cast<ddsrt_md5_byte_t*>(out.value));

          return true;
        }
      }
    }
  }
}
