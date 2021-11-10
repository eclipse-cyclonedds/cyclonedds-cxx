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

void cdr_stream::set_buffer(void* toset, size_t buffer_size)
{
  m_buffer = static_cast<char*>(toset);
  m_buffer_size = buffer_size;
  reset();
}

bool cdr_stream::align(size_t newalignment, bool add_zeroes)
{
  auto al = alignment(std::min(newalignment, m_max_alignment));

  size_t tomove = (al - position() % al) % al;

  if (tomove &&
      (m_mode == stream_mode::read || m_mode == stream_mode::write)) {
    if (!bytes_available(tomove))
      return false;
    if (add_zeroes) {
      auto cursor = get_cursor();
      assert(cursor);
      memset(cursor, 0, tomove);
    }
  }

  incr_position(tomove);

  return true;
}

bool cdr_stream::finish_member(entity_properties_t &prop, bool)
{
  if (abort_status())
    return false;

  if (!prop.is_present) {
    if (m_mode == stream_mode::read)
      go_to_next_member(prop);
    else
      return false;
  }

  return true;
}

bool cdr_stream::finish_struct(entity_properties_t &props)
{
  check_struct_completeness(props, m_key ? member_list_type::key : member_list_type::member_by_seq);

  return !abort_status() && props.is_present;
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
  return !abort_status();
}

void cdr_stream::reset()
{
  position(0);
  alignment(0);
  m_status = 0;
  m_buffer_end = std::stack<size_t>({m_buffer_size});
  m_stack = std::stack<proplist::iterator>();
}

void cdr_stream::skip_entity(const entity_properties_t &prop)
{
  incr_position(prop.e_sz);
  alignment(0);
}

bool cdr_stream::start_member(entity_properties_t &prop, bool)
{
  record_member_start(prop);

  return true;
}

bool cdr_stream::start_struct(entity_properties_t &props)
{
  record_struct_start(props);

  return true;
}

void cdr_stream::record_member_start(entity_properties_t &prop)
{
  prop.e_off = position();
  prop.is_present = true;
}

void cdr_stream::go_to_next_member(entity_properties_t &prop)
{
  if (prop.e_sz > 0 && m_mode == stream_mode::read) {
    position(prop.e_off + prop.e_sz);
    alignment(0);  //we made a jump, so we do not know the current alignment
  }
}

void cdr_stream::record_struct_start(entity_properties_t &props)
{
  props.is_present = true;
  props.d_off = position();
}

void cdr_stream::check_struct_completeness(entity_properties_t &props, member_list_type list_type)
{
  if (m_mode != stream_mode::read)
    return;

  if (abort_status()) {
    props.is_present = false;
    return;
  }

  proplist::iterator it;
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

  while (*it) {
    if (it->must_understand_local && !it->is_present) {
      status(must_understand_fail);
      props.is_present = false;
      break;
    }
    it++;
  }
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
