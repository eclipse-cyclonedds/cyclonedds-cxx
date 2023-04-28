// Copyright(c) 2020 to 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

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

bool xcdr_v1_stream::start_member(const entity_properties_t &prop, bool is_set)
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
        m_buffer_end.push(position() + m_e_sz.top());
        break;
      default:
        break;
    }
  }

  push_member_start();
  return cdr_stream::start_member(prop, is_set);
}

bool xcdr_v1_stream::finish_member(const entity_properties_t &prop, member_id_set &member_ids, bool is_set)
{
  if (header_necessary(prop)) {
    switch (m_mode) {
      case stream_mode::write:
        if ((prop.p_ext != extensibility::ext_mutable || is_set) &&
            !finish_write_header(prop))
          return false;
        break;
      case stream_mode::read:
        m_buffer_end.pop();
        break;
      default:
        break;
    }
  }

  pop_member_start();
  return cdr_stream::finish_member(prop, member_ids, is_set);
}

const entity_properties_t* xcdr_v1_stream::next_entity(const entity_properties_t *prop)
{
  if (m_mode != stream_mode::read)
    return cdr_stream::next_entity(prop);

  if (!list_necessary(*(prop->parent))) {
    while ((prop = cdr_stream::next_entity(prop))) {
      if (prop->p_ext == extensibility::ext_appendable &&
          !bytes_available(1, true))
        return nullptr;
      if (prop->is_optional) {
        entity_properties_t temp;
        bool is_final = false;
        if (!read_header(temp, is_final)) {
          return nullptr;  //read failure
        } else if (is_final) {
          return nullptr;  //final field
        } else if (m_e_sz.top() != 0) {
          break;
        }
      } else {
        break;
      }
    }
  } else {
    while (1) {  //using while loop to prevent recursive calling, which could lead to stack overflow
      entity_properties_t temp;
      bool is_final = false;
      if (!read_header(temp, is_final)) {
        return nullptr;  //read failure
      } else if (is_final) {
        return nullptr;  //final field
      } else if (0 == m_e_sz.top()) {
        continue;  //empty field
      } else if (temp.ignore) {
        //ignore this field
        incr_position(m_e_sz.top());
        alignment(0);
        continue;
      }

      //search forward
      auto p = prop;
      while (p && p->m_id != temp.m_id)
        p = cdr_stream::next_entity(p);

      //search backward
      if (!p) {
        p = prop;
        while (p && p->m_id != temp.m_id)
          p = cdr_stream::previous_entity(p);
      }

      if (!p) {  //could not find this entry in the list of parameters
        if (temp.must_understand &&
            status(must_understand_fail))
          return nullptr;
        incr_position(m_e_sz.top());
        alignment(0);
      } else {
        prop = p;
        break;
      }
    }
  }
  return prop;
}

const entity_properties_t* xcdr_v1_stream::first_entity(const entity_properties_t *props)
{
  auto prop = cdr_stream::first_entity(props);
  if (m_mode != stream_mode::read || !prop)
    return prop;

  if (!list_necessary(*props)) {
    do {
      if (prop->is_optional) {
        entity_properties_t temp;
        bool is_final = false;
        if (!read_header(temp, is_final)) {
          return nullptr;  //read failure
        } else if (is_final) {
          return nullptr;  //final field
        } else if (m_e_sz.top() != 0) {
          break;
        }
      } else {
        break;
      }
    } while  ((prop = cdr_stream::next_entity(prop)));
  } else {
    while (1) {  //using while loop to prevent recursive calling, which could lead to stack overflow
      entity_properties_t temp;
      bool is_final = false;
      if (!read_header(temp, is_final)) {
        return nullptr;  //read failure
      } else if (is_final) {
        return nullptr;  //final field
      } else if (0 == m_e_sz.top()) {
        continue;  //empty field
      } else if (temp.ignore) {
        //ignore this field
        incr_position(m_e_sz.top());
        alignment(0);
        continue;
      }

      //search forward
      auto p = prop;
      while (p && p->m_id != temp.m_id)
        p = cdr_stream::next_entity(p);

      if (!p) {  //could not find this entry in the list of parameters
        if (temp.must_understand &&
            status(must_understand_fail))
          return nullptr;
        incr_position(m_e_sz.top());
        alignment(0);
      } else {
        prop = p;
        break;
      }
    }
  }
  return prop;
}

bool xcdr_v1_stream::header_necessary(const entity_properties_t &props)
{
  return (props.p_ext == extensibility::ext_mutable || props.is_optional) && !m_key;
}

bool xcdr_v1_stream::read_header(entity_properties_t &out, bool &is_final)
{
  uint16_t smallid = 0, smalllength = 0;

  if (!align(4, false)
   || !bytes_available(4)
   || !read(*this, smallid)
   || !read(*this, smalllength))
    return false;

  out.m_id = smallid & pid_mask;
  m_e_sz.top() = smalllength;
  out.must_understand = pid_flag_must_understand & smallid;
  out.implementation_extension = pid_flag_impl_extension & smallid;
  switch (out.m_id) {
    case pid_list_end:
      is_final = true;
      break;
    case pid_ignore:
      out.ignore = true;
      break;
    case pid_extended:
    {
      uint32_t memberheader = 0;
      if (!bytes_available(8)
       || !read(*this, memberheader)
       || !read(*this, m_e_sz.top()))
        return false;

      out.must_understand = pl_extended_flag_must_understand & memberheader;
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

bool xcdr_v1_stream::write_header(const entity_properties_t &props)
{
  if (!align(4, true)) {
    return false;
  } else if (extended_header(props)) {
    uint16_t smallid = pid_extended + pid_flag_must_understand;
    uint32_t largeid = (props.m_id & pl_extended_mask) + (props.must_understand || props.is_key ? pl_extended_flag_must_understand : 0);
    return write(*this, smallid) && write(*this, uint16_t(8))
        && write(*this, largeid) && write(*this, uint32_t(0));  /* length field placeholder, to be completed by finish_write_header */
  } else {
    uint16_t smallid = static_cast<uint16_t>(props.m_id + (props.must_understand || props.is_key ? pid_flag_must_understand : 0));
    return write(*this, smallid) && write(*this, uint16_t(0));  /* length field placeholder, to be completed by finish_write_header */
  }
}

bool xcdr_v1_stream::finish_write_header(const entity_properties_t &props)
{
  auto current_position = position();
  auto current_alignment = alignment();

  uint32_t e_sz = static_cast<uint32_t>(current_position - m_e_off.top());
  if (extended_header(props)) {
    position(m_e_off.top()-4);
    if (!write(*this, e_sz))
      return false;
  } else {
    position(m_e_off.top()-2);
    alignment(2);
    if (!write(*this, uint16_t(e_sz)))
      return false;
  }

  position(current_position);
  alignment(current_alignment);

  return true;
}

bool xcdr_v1_stream::finish_struct(const entity_properties_t &props, const member_id_set &member_ids)
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
      return check_struct_completeness(props, member_ids);
      break;
    default:
      return false;
  }
  return true;
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
  return props.e_bb == bb_unset || props.m_id >= pid_extended;
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
