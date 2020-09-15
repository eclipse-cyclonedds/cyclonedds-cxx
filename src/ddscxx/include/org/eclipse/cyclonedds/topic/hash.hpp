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


 /**
  * @file
  */

#ifndef IDLCXX_HASH_HPP_
#define IDLCXX_HASH_HPP_

#include "dds/ddsi/ddsi_keyhash.h"
#include "dds/ddsrt/md5.h"
#include <vector>
#include <cstring>

namespace org
{
  namespace eclipse
  {
    namespace cyclonedds
    {
      namespace topic
      {
        ddsi_keyhash_t simple_key(const std::vector<unsigned char> & in)
        {
          ddsi_keyhash_t returnval;
          memcpy(&returnval.value, in.data(), in.size());
          memset((&returnval.value) + in.size(), 0x0, 16 - in.size());
          return returnval;
        }

        ddsi_keyhash_t complex_key(const std::vector<unsigned char> & in)
        {
          ddsi_keyhash_t returnval;

          ddsrt_md5_state_t md5st;
          ddsrt_md5_init(&md5st);
          ddsrt_md5_append(&md5st, in.data(), in.size());
          ddsrt_md5_finish(&md5st, returnval.value);

          return returnval;
        }
      }
    }
  }
}

#endif /* IDLCXX_HASH_HPP_ */
