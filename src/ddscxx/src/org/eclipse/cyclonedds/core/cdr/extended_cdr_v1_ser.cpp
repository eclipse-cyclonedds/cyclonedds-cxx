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

#include <org/eclipse/cyclonedds/core/cdr/extended_cdr_v1_ser.hpp>
#include <algorithm>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

const uint16_t xcdr_v1_stream::pid_mask                 = 0x3FFF;
const uint16_t xcdr_v1_stream::pid_extended             = 0x3F01;
const uint16_t xcdr_v1_stream::pid_list_end             = 0x3F02;
const uint16_t xcdr_v1_stream::pid_ignore               = 0x3F03;
const uint16_t xcdr_v1_stream::pid_flag_impl_extension  = 0x8000;
const uint16_t xcdr_v1_stream::pid_flag_must_understand = 0x4000;

const uint32_t xcdr_v1_stream::pl_extended_mask                 = 0x0FFFFFFF;
const uint32_t xcdr_v1_stream::pl_extended_flag_impl_extension  = 0x80000000;
const uint32_t xcdr_v1_stream::pl_extended_flag_must_understand = 0x40000000;

bool xcdr_v1_stream::start_member(entity_properties_t &prop, bool is_set)
{
  if (header_necessary(prop) && (prop.p_ext != extensibility::ext_mutable || is_set)) {
    switch (m_mode) {
      case stream_mode::write:
        if (!write_header(prop))
          return false;
        break;
      case stream_mode::move:
      case stream_mode::max:
        if (!move_header(prop))
          return false;
        break;
      case stream_mode::read:
        m_buffer_end.push(position() + prop.e_sz);
        break;
      default:
        break;
    }
  }

  record_member_start(prop);
  return true;
}

bool xcdr_v1_stream::finish_member(entity_properties_t &prop, bool is_set)
{
  if (abort_status())
    return false;

  if (header_necessary(prop)) {
    switch (m_mode) {
      case stream_mode::write:
        if (prop.p_ext != extensibility::ext_mutable || is_set)
          return finish_write_header(prop);
        break;
      case stream_mode::read:
        m_buffer_end.pop();
        break;
      default:
        break;
    }
  }

  return true;
}

entity_properties_t& xcdr_v1_stream::next_entity(entity_properties_t &props, bool &firstcall)
{
  if (abort_status())
    return m_final;

  member_list_type ml = member_list_type::member_by_seq;
  if (m_key)
    ml = member_list_type::key;

  if (m_mode != stream_mode::read)
    return next_prop(props, ml, firstcall);

  if (!list_necessary(props)) {
    while (1) {  //using while loop to prevent recursive calling, which could lead to stack overflow
      auto &prop = next_prop(props, ml, firstcall);

      if (prop.is_optional) {
        entity_properties_t temp;
        if (!read_header(temp))
          break;

        prop.e_sz = temp.e_sz;
        if (prop.e_sz)
          return prop;
      } else {
        return prop;
      }

    }
  } else {
    proplist *ptr = NULL;
    if (m_key)
      ptr = &props.m_keys;
    else
      ptr = &props.m_members_by_id;

    while (1) {  //using while loop to prevent recursive calling, which could lead to stack overflow

      if (!read_header(m_current_header) || !m_current_header)
        break;
      else if (0 == m_current_header.e_sz)
        continue;

      auto p = std::equal_range(ptr->begin(), ptr->end(), m_current_header, entity_properties_t::member_id_comp);
      if (p.first != ptr->end() && (p.first->m_id == m_current_header.m_id || (!(*p.first) && !m_current_header))) {
        p.first->must_understand_remote = m_current_header.must_understand_remote;
        p.first->e_sz = m_current_header.e_sz;
        return *(p.first);
      } else {
        return m_current_header;
      }
    }
  }
  return m_final;
}

bool xcdr_v1_stream::header_necessary(const entity_properties_t &props)
{
  return (props.p_ext == extensibility::ext_mutable || props.is_optional) && !m_key;
}

bool xcdr_v1_stream::read_header(entity_properties_t &out)
{
  out = entity_properties_t();
  uint16_t smallid = 0, smalllength = 0;

  if (!align(4, false)
   || !bytes_available(4)
   || !read(*this, smallid)
   || !read(*this, smalllength))
    return false;

  out.m_id = smallid & pid_mask;
  out.e_sz = smalllength;
  out.must_understand_remote = pid_flag_must_understand & smallid;
  out.implementation_extension = pid_flag_impl_extension & smallid;
  switch (out.m_id) {
    case pid_list_end:
      out.is_last = true;
      break;
    case pid_ignore:
      out.ignore = true;
      break;
    case pid_extended:
    {
      uint32_t memberheader = 0, largelength = 0;
      if (!bytes_available(8)
       || !read(*this, memberheader)
       || !read(*this, largelength))
        return false;

      out.e_sz = largelength;
      out.must_understand_remote = pl_extended_flag_must_understand & memberheader;
      out.implementation_extension = pl_extended_flag_impl_extension & memberheader;
      out.m_id = pl_extended_mask & memberheader;
    }
      break;
    default:
      if (out.m_id > pid_ignore)
        status(invalid_pl_entry);
  }

  return true;
}

bool xcdr_v1_stream::write_header(entity_properties_t &props)
{
  if (!align(4, true)) {
    return false;
  } else if (extended_header(props)) {
    uint16_t smallid = pid_extended + pid_flag_must_understand;
    uint32_t largeid = (props.m_id & pl_extended_mask) + (props.must_understand_local ? pl_extended_flag_must_understand : 0);
    return write(*this, smallid) && write(*this, uint16_t(8))
        && write(*this, largeid) && write(*this, uint32_t(0));  /* length field placeholder, to be completed by finish_write_header */
  } else {
    uint16_t smallid = static_cast<uint16_t>(props.m_id + (props.must_understand_local ? pid_flag_must_understand : 0));
    return write(*this, smallid) && write(*this, uint16_t(0));  /* length field placeholder, to be completed by finish_write_header */
  }
}

bool xcdr_v1_stream::finish_write_header(entity_properties_t &props)
{
  auto current_position = position();
  auto current_alignment = alignment();

  props.e_sz = static_cast<uint32_t>(current_position - props.e_off);

  if (extended_header(props)) {
    position(props.e_off-4);
    if (!write(*this, props.e_sz))
      return false;
  } else {
    position(props.e_off-2);
    alignment(2);
    if (!write(*this, uint16_t(props.e_sz)))
      return false;
  }

  position(current_position);
  alignment(current_alignment);

  return true;
}

bool xcdr_v1_stream::finish_struct(entity_properties_t &props)
{
  switch (m_mode) {
    case stream_mode::write:
      if (list_necessary(props))
        return write_final_list_entry();
      break;
    case stream_mode::move:
    case stream_mode::max:
      if (list_necessary(props))
        return move_final_list_entry();
      break;
    case stream_mode::read:
      check_struct_completeness(props, m_key ? member_list_type::key :
          (list_necessary(props) ? member_list_type::member_by_id : member_list_type::member_by_seq));
      break;
    default:
      break;
  }

  return !abort_status() && props.is_present;
}

bool xcdr_v1_stream::list_necessary(const entity_properties_t &props)
{
  return props.e_ext == extensibility::ext_mutable && !m_key;
}

bool xcdr_v1_stream::write_final_list_entry()
{
  uint16_t smallid = pid_flag_must_understand + pid_list_end;

  return (align(4, true) && write(*this, smallid) && write(*this, uint16_t(0)));
}

bool xcdr_v1_stream::move_final_list_entry()
{
  return move(*this, uint32_t(0));
}

bool xcdr_v1_stream::move_header(const entity_properties_t &props)
{
  if (!move(*this, uint32_t(0)))
    return false;

  if (extended_header(props)) {
    return (move(*this, uint32_t(0)) && move(*this, uint32_t(0)));
  }

  return true;
}

bool xcdr_v1_stream::extended_header(const entity_properties_t &props)
{
  return !props.e_bb || props.m_id >= pid_extended;
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
