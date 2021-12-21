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

#include <org/eclipse/cyclonedds/core/cdr/basic_cdr_ser.hpp>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

entity_properties_t& basic_cdr_stream::next_entity(entity_properties_t &props, bool &firstcall)
{
  if (abort_status())
    return m_final;

  m_entity_sizes.push(0);
  auto &prop = next_prop(props, m_key ? member_list_type::key : member_list_type::member_by_seq, firstcall);
  return prop;
}

bool basic_cdr_stream::start_struct(entity_properties_t &props)
{
  if (props.requires_xtypes() && status(unsupported_xtypes))
    return false;

  return cdr_stream::start_struct(props);
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
