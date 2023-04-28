// Copyright(c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "org/eclipse/cyclonedds/core/cdr/fragchain.hpp"
#include "dds/core/Exception.hpp"
#include <memory>
#include <cstring>
#include <assert.h>
#include "dds/ddsi/ddsi_radmin.h"

namespace org { namespace eclipse { namespace cyclone { namespace core { namespace cdr {

void serdata_from_ser_copyin_fragchain (unsigned char * __restrict cursor, const struct ddsi_rdata* fragchain, size_t size)
{
  uint32_t off = 0;
  assert(fragchain->min == 0);
  assert(fragchain->maxp1 >= off);    //CDR header must be in first fragment
  while (fragchain) {
    if (fragchain->maxp1 > off) {
      //only copy if this fragment adds data
      const unsigned char* payload =
        DDSI_RMSG_PAYLOADOFF(fragchain->rmsg, DDSI_RDATA_PAYLOAD_OFF(fragchain));
      auto src = payload + off - fragchain->min;
      auto n_bytes = fragchain->maxp1 - off;
      memcpy(cursor, src, n_bytes);
      cursor += n_bytes;
      off = fragchain->maxp1;
      if (off > size) {
        assert(false);
        throw dds::core::InvalidDataError("Fragchain contained fewer bytes than indicated size");
      }
    }
    fragchain = fragchain->nextfrag;
  }
}

} } } } }
