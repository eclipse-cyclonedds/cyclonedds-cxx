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
#include <cstring>
#include <assert.h>

#include <org/eclipse/cyclonedds/core/cdr/cdr_stream.hpp>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

entity_properties_t cdr_stream::m_final = final_entry();

void cdr_stream::set_buffer(void* toset) {
  m_buffer = static_cast<char*>(toset);
  m_status = 0;
  reset_position();
}

size_t cdr_stream::align(size_t newalignment, bool add_zeroes)
{
  if (m_current_alignment == newalignment)
    return 0;

  m_current_alignment = std::min(newalignment, m_max_alignment);

  size_t tomove = (m_current_alignment - m_position % m_current_alignment) % m_current_alignment;
  if (tomove && add_zeroes && m_buffer) {
    auto cursor = get_cursor();
    assert(cursor);
    memset(cursor, 0, tomove);
  }

  m_position += tomove;

  return tomove;
}

entity_properties_t& cdr_stream::next_prop(entity_properties_t &props, member_list_type list_type, bool &firstcall)
{
  if (firstcall) {
    std::list<entity_properties_t>::iterator it;
    switch (list_type) {
      case member_list_type::member_by_seq:
        it = props.m_members_by_seq.begin();
        break;
      case member_list_type::member_by_id:
        it = props.m_members_by_id.begin();
        break;
      case member_list_type::key:
        it = props.m_keys.begin();
        break;
      default:
        assert(0);
    }
    m_stack.push(it);
    firstcall = false;
    return *it;
  }

  assert(m_stack.size());

  if (*m_stack.top())  //we have not yet reached the end of the entities in the list, so we can go to the next
    m_stack.top()++;

  entity_properties_t &entity = *m_stack.top();
  if (!entity) //we have reached the end of the list
    m_stack.pop();

  return entity;
}

entity_properties_t& cdr_stream::top_of_stack()
{
  assert(m_stack.size());
  return *(m_stack.top());
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
