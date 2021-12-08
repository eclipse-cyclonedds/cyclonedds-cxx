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

#include <org/eclipse/cyclonedds/core/cdr/extended_cdr_v2_ser.hpp>
#include <algorithm>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

const uint32_t xcdr_v2_stream::bytes_1         = uint32_t(0x00000000);
const uint32_t xcdr_v2_stream::bytes_2         = uint32_t(0x10000000);
const uint32_t xcdr_v2_stream::bytes_4         = uint32_t(0x20000000);
const uint32_t xcdr_v2_stream::bytes_8         = uint32_t(0x30000000);
const uint32_t xcdr_v2_stream::nextint         = uint32_t(0x40000000);
const uint32_t xcdr_v2_stream::nextint_times_1 = uint32_t(0x50000000);
const uint32_t xcdr_v2_stream::nextint_times_4 = uint32_t(0x60000000);
const uint32_t xcdr_v2_stream::nextint_times_8 = uint32_t(0x70000000);
const uint32_t xcdr_v2_stream::lc_mask         = uint32_t(0x70000000);
const uint32_t xcdr_v2_stream::id_mask         = uint32_t(0x0FFFFFFF);
const uint32_t xcdr_v2_stream::must_understand = uint32_t(0x80000000);

bool xcdr_v2_stream::start_member(entity_properties_t &prop, bool is_set)
{
  switch (m_mode) {
    case stream_mode::write:
      if (em_header_necessary(prop)) {
        if (is_set && !write_em_header(prop))
          return false;
      } else if (prop.is_optional && !write_optional_tag(is_set)) {
          return false;
      }
      break;
    case stream_mode::move:
    case stream_mode::max:
      if (em_header_necessary(prop)) {
        if (is_set && !move_em_header())
          return false;
      } else if (prop.is_optional && !move_optional_tag()) {
        return false;
      }
      break;
    case stream_mode::read:
      if (em_header_necessary(prop) && is_set)
        m_buffer_end.push(position() + prop.e_sz);
      break;
    default:
      break;
  }

  record_member_start(prop);
  return true;
}

bool xcdr_v2_stream::finish_member(entity_properties_t &prop, bool is_set)
{
  switch (m_mode) {
    case stream_mode::write:
      prop.e_sz = static_cast<uint32_t>(position()-prop.e_off);
      if (em_header_necessary(prop)) {
        if (is_set && !finish_em_header(prop))
          return false;
      }
      break;
    case stream_mode::read:
      if (em_header_necessary(prop) && is_set)
        m_buffer_end.pop();
      break;
    default:
      break;
  }

  return true;
}

bool xcdr_v2_stream::write_d_header()
{
  align(4, true);
  m_delimiters.push(position());
  return write(*this, uint32_t(0));
}

bool xcdr_v2_stream::write_optional_tag(bool present)
{
  return write(*this, present ? uint8_t(1) : uint8_t(0));
}

bool xcdr_v2_stream::move_optional_tag()
{
  return move(*this, uint8_t(0));
}

entity_properties_t& xcdr_v2_stream::next_entity(entity_properties_t &props, bool &firstcall)
{
  member_list_type ml = member_list_type::member_by_seq;
  if (m_key)
    ml = member_list_type::key;

  if (m_mode != stream_mode::read)
    return next_prop(props, ml, firstcall);

  if (!list_necessary(props)) {
    while (1) {  //using while loop to prevent recursive calling, which could lead to stack overflow
      auto &prop = next_prop(props, ml, firstcall);

      if (!prop)
        return prop;
      else if (!bytes_available())
        break;

      bool fieldpresent = true;
      if (prop.is_optional) {
        if (!bytes_available(1)
         || !read(*this, fieldpresent))
        break;
      }

      if (fieldpresent)
        return prop;
    }
  } else {
    proplist *ptr = NULL;
    if (m_key)
      ptr = &props.m_keys;
    else
      ptr = &props.m_members_by_id;

    while (1) {  //using while loop to prevent recursive calling, which could lead to stack overflow
      if (!bytes_available(4,true) || !read_em_header(m_current_header) || !m_current_header)
        break;

      if (0 == m_current_header.e_sz)  //this field is empty
        continue;

      auto p = std::equal_range(ptr->begin(), ptr->end(), m_current_header, entity_properties_t::member_id_comp);
      if (p.first != ptr->end() && (p.first->m_id == m_current_header.m_id || (!(*p.first) && !m_current_header))) {
        p.first->e_sz = m_current_header.e_sz;
        p.first->must_understand_remote = m_current_header.must_understand_remote;
        return *(p.first);
      } else {
        return m_current_header;
      }
    }
  }

  return m_final;
}

bool xcdr_v2_stream::read_em_header(entity_properties_t &props)
{
  props = entity_properties_t();

  uint32_t emheader = 0;
  if (!read(*this,emheader))
    return false;

  uint32_t factor = 0;
  props.must_understand_remote = emheader & must_understand;
  props.m_id = emheader & id_mask;
  switch (emheader & lc_mask) {
    case bytes_1:
      props.e_sz = 1;
      break;
    case bytes_2:
      props.e_sz = 2;
      break;
    case bytes_4:
      props.e_sz = 4;
      break;
    case bytes_8:
      props.e_sz = 8;
      break;
    case nextint:
      if (!read(*this, props.e_sz))
        return false;
      break;
    case nextint_times_1:
      factor = 1;
      break;
    case nextint_times_4:
      factor = 4;
      break;
    case nextint_times_8:
      factor = 8;
      break;
  }

  if (factor) {
    if (!read(*this, props.e_sz))
      return false;
    props.e_sz *= factor;
    props.e_sz += 4;
    //move cursor back 4 bytes, due to overlap of nextint and entity
    if ((emheader & lc_mask) > nextint)
      position(position()-4);
  }

  return true;
}

bool xcdr_v2_stream::read_d_header()
{
  uint32_t d_sz;
  if (!read(*this, d_sz))
    return false;
  m_buffer_end.push(position() + d_sz);
  return true;
}

bool xcdr_v2_stream::d_header_necessary(const entity_properties_t &props)
{
  return (props.e_ext == extensibility::ext_appendable || props.e_ext == extensibility::ext_mutable)
      && !m_key;
}

bool xcdr_v2_stream::list_necessary(const entity_properties_t &props)
{
  return props.e_ext == extensibility::ext_mutable && !m_key;
}

bool xcdr_v2_stream::start_struct(entity_properties_t &props)
{
  if (d_header_necessary(props)) {
    switch (m_mode) {
      case stream_mode::write:
        if (!write_d_header())
          return false;
        break;
      case stream_mode::move:
      case stream_mode::max:
        if (!move_d_header())
          return false;
        break;
      case stream_mode::read:
        if (!read_d_header())
          return false;
        break;
      default:
        break;
    }
  }

  cdr_stream::start_struct(props);

  return true;
}

bool xcdr_v2_stream::finish_struct(entity_properties_t &props)
{
  switch (m_mode) {
    case stream_mode::write:
      if (d_header_necessary(props) && !finish_d_header())
        return false;
      break;
    case stream_mode::read:
      {
        check_struct_completeness(props, m_key ? member_list_type::key :
          (list_necessary(props) ? member_list_type::member_by_id : member_list_type::member_by_seq));
        if (d_header_necessary(props))
          m_buffer_end.pop();
      }
      break;
    default:
      break;
  }

  return !abort_status() && props.is_present;
}

bool xcdr_v2_stream::write_em_header(entity_properties_t &props)
{
  uint32_t mheader = (props.must_understand_local ? must_understand : 0)
                     + (id_mask & props.m_id) + nextint;

  return write(*this, mheader) && write(*this, uint32_t(0));
}

bool xcdr_v2_stream::move_em_header()
{
  return move(*this, uint32_t(0)) && move(*this, uint32_t(0));
}

bool xcdr_v2_stream::finish_d_header()
{
  auto current_position = position();
  auto d_off = m_delimiters.top();
  m_delimiters.pop();
  uint32_t d_sz = static_cast<uint32_t>(current_position - d_off - 4);

  if (d_sz == 0)
    return true;

  auto current_alignment = alignment();

  position(d_off);
  alignment(4);
  if (!write(*this, d_sz))
    return false;

  position(current_position);
  alignment(current_alignment);

  return true;
}

void xcdr_v2_stream::reset()
{
  cdr_stream::reset();
  m_delimiters = std::stack<size_t>();
}

bool xcdr_v2_stream::finish_em_header(entity_properties_t &props)
{
  if (props.e_sz == 0)
    return true;

  auto current_position = position();
  auto current_alignment = alignment();

  position(props.e_off - 4);
  alignment(4);
  if (!write(*this, props.e_sz))
    return false;

  position(current_position);
  alignment(current_alignment);

  return true;
}

bool xcdr_v2_stream::em_header_necessary(const entity_properties_t &props)
{
  return props.p_ext == extensibility::ext_mutable && !m_key;
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
