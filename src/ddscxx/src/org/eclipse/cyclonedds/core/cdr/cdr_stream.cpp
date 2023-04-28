// Copyright(c) 2020 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <cstring>
#include <assert.h>
#include <algorithm>

#include <org/eclipse/cyclonedds/core/cdr/cdr_stream.hpp>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

void cdr_stream::set_buffer(void* toset, size_t buffer_size)
{
  m_buffer = static_cast<char*>(toset);
  m_buffer_size = buffer_size;
  reset();
}

bool cdr_stream::align(size_t newalignment, bool add_zeroes)
{
  if (newalignment == m_current_alignment)
    return true;

  auto al = alignment(std::min(newalignment, m_max_alignment));

  size_t tomove = (al - position() % al) % al;

  if (tomove) {
    if (m_mode ==  stream_mode::read || m_mode == stream_mode::write) {
      if (!bytes_available(tomove)) {
        return false;
      } else if (add_zeroes) {
        auto cursor = get_cursor();
        assert(cursor);
        memset(cursor, 0, tomove);
      }
    }
    incr_position(tomove);
  }

  return true;
}

bool cdr_stream::finish_struct(const entity_properties_t &props, const member_id_set &member_ids)
{
  switch (m_mode) {
    case stream_mode::read:
      return check_struct_completeness(props, member_ids);
      break;
    default:
      return true;
  }
}

const entity_properties_t *cdr_stream::first_entity(const entity_properties_t *props)
{
  const entity_properties_t *prop = props->first_member;
  while (m_key && prop && !prop->is_key)
    prop = next_entity(prop);

  return prop;
}

const entity_properties_t* cdr_stream::next_entity(const entity_properties_t *prop)
{
  prop = prop->next_on_level;
  if (m_key) {
    while (prop && !prop->is_key)
      prop = prop->next_on_level;
  }
  return prop;
}

const entity_properties_t* cdr_stream::previous_entity(const entity_properties_t *prop)
{
  prop = prop->prev_on_level;
  if (m_key) {
    while (prop && !prop->is_key)
      prop = prop->prev_on_level;
  }
  return prop;
}

bool cdr_stream::bytes_available(size_t N, bool peek)
{
  assert(m_buffer_end.size());
  if (position()+N > m_buffer_end.top()) {
    switch (m_mode) {
      case stream_mode::read:
        return !peek && !status(read_bound_exceeded);
        break;
      case stream_mode::write:
        return !peek && !status(write_bound_exceeded);
        break;
      default:
        break;
    }
  }
  return true;
}

void cdr_stream::reset()
{
  position(0);
  alignment(0);
  m_status = 0;
  m_buffer_end = m_buffer_size;
  m_e_off.reset();
  m_e_sz = 0;
}

bool cdr_stream::check_struct_completeness(const entity_properties_t &props, const member_id_set &member_ids)
{
  if (m_mode != stream_mode::read || abort_status())
    return false;

  const entity_properties_t *ptr = props.first_member;
  while (ptr) {
    if ((ptr->must_understand || ptr->is_key) &&  //if this entity is a must_understand or key member
        member_ids.end() == member_ids.find(ptr->m_id) && //and it was not succesfully deserialized
        status(must_understand_fail)) //and we cannot ignore missing must_understand fields
      return false;
    ptr = cdr_stream::next_entity(ptr);
  }

  return true;
}

bool cdr_stream::finish_member(const entity_properties_t &props, member_id_set &member_ids, bool is_set)
{
  if (is_set)
    member_ids.insert(props.m_id);
  return true;
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
