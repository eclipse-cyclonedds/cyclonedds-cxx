// Copyright(c) 2021 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <org/eclipse/cyclonedds/core/cdr/basic_cdr_ser.hpp>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

bool basic_cdr_stream::start_struct(const entity_properties_t &props)
{
  if (!is_key() && props.xtypes_necessary && status(unsupported_xtypes))
    return false;

  return cdr_stream::start_struct(props);
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
