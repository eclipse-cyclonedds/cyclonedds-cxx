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

void xcdr_v1_stream::start_member(entity_properties_t &prop, bool present)
{
  if (!header_necessary(prop))
    return;

  if (prop.p_ext == ext_mutable && !present)
    return;

  switch (m_mode) {
    case stream_mode::write:
      write_header(prop);
      break;
    case stream_mode::move:
    case stream_mode::max:
      move_header(prop);
      break;
    default:
      break;
  }
}

void xcdr_v1_stream::finish_member(entity_properties_t &prop, bool present)
{
  if (m_mode != stream_mode::write)
    return;

  if (!header_necessary(prop))
    return;

  if (prop.p_ext == ext_mutable && !present)
    return;

  finish_write_header(prop);
}

void xcdr_v1_stream::skip_entity(const entity_properties_t &props)
{
  incr_position(props.e_sz);
}

entity_properties_t& xcdr_v1_stream::next_entity(entity_properties_t &props, bool &firstcall)
{
  member_list_type ml = member_list_type::member_by_seq;
  if (m_key)
    ml = member_list_type::key;

  if (m_mode != stream_mode::read)
    return next_prop(props, ml, firstcall);

  if (!list_necessary(props)) {
    while (1) {  //using while loop to prevent recursive calling, which could lead to stack overflow
      auto &prop = next_prop(props, ml, firstcall);

      entity_properties_t temp;
      if (prop.is_optional) {
        read_header(temp);
        if (temp.e_sz)
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
      read_header(m_current_header);

      if (!m_current_header)
        break;
      else if (0 == m_current_header.e_sz)
        continue;

      auto p = std::equal_range(ptr->begin(), ptr->end(), m_current_header, entity_properties_t::member_id_comp);
      if (p.first != ptr->end() && (p.first->m_id == m_current_header.m_id || (!(*p.first) && !m_current_header)))
        return *(p.first);
      else
        return m_current_header;
    }
  }
  return m_final;
}

bool xcdr_v1_stream::header_necessary(const entity_properties_t &props)
{
  return (props.p_ext == ext_mutable || props.is_optional) && !m_key;
}

void xcdr_v1_stream::read_header(entity_properties_t &props)
{
  props = entity_properties_t();

  align(4, false);

  uint16_t smallid = 0, smalllength = 0;

  read(*this, smallid);
  read(*this, smalllength);

  props.m_id = smallid & pid_mask;
  props.e_sz = smalllength;
  props.must_understand = pid_flag_must_understand & smallid;
  props.implementation_extension = pid_flag_impl_extension & smallid;
  switch (props.m_id) {
    case pid_list_end:
      props.is_last = true;
      break;
    case pid_ignore:
      props.ignore = true;
      break;
    case pid_extended:
    {
      uint32_t memberheader = 0, largelength = 0;
      read(*this, memberheader);
      read(*this, largelength);

      props.e_sz = largelength;
      props.must_understand = pl_extended_flag_must_understand & memberheader;
      props.implementation_extension = pl_extended_flag_impl_extension & memberheader;
      props.m_id = pl_extended_mask & memberheader;
    }
      break;
    default:
      if (props.m_id > pid_ignore)
        status(invalid_pl_entry);
  }
}

void xcdr_v1_stream::write_header(entity_properties_t &props)
{
  align(4, true);

  if (extended_header(props)) {
    uint16_t smallid = pid_extended + pid_flag_must_understand;
    uint32_t largeid = (props.m_id & pl_extended_mask) + (props.must_understand ? pl_extended_flag_must_understand : 0);
    write(*this, smallid);
    write(*this, uint16_t(8));
    write(*this, largeid);
    write(*this, uint32_t(0));  /* length field placeholder, to be completed by finish_write_header */
  } else {
    uint16_t smallid = static_cast<uint16_t>(props.m_id + (props.must_understand ? pid_flag_must_understand : 0));
    write(*this, smallid);
    write(*this, uint16_t(0));  /* length field placeholder, to be completed by finish_write_header */
  }

  props.e_off = position();
}

void xcdr_v1_stream::finish_write_header(entity_properties_t &props)
{
  auto current_position = position();
  auto current_alignment = alignment();

  props.e_sz = static_cast<uint32_t>(current_position - props.e_off);

  if (extended_header(props)) {
    position(props.e_off-4);
    write(*this, props.e_sz);
  } else {
    position(props.e_off-2);
    write(*this, uint16_t(props.e_sz));
  }

  position(current_position);
  alignment(current_alignment);
}

void xcdr_v1_stream::finish_struct(entity_properties_t &props)
{
  if (!list_necessary(props))
    return;

  switch (m_mode) {
    case stream_mode::write:
      write_final_list_entry();
      break;
    case stream_mode::move:
    case stream_mode::max:
      move_final_list_entry();
      break;
    default:
      break;
  }
}

bool xcdr_v1_stream::list_necessary(const entity_properties_t &props)
{
  return props.e_ext == ext_mutable && !m_key;
}

void xcdr_v1_stream::write_final_list_entry()
{
  uint16_t smallid = pid_flag_must_understand + pid_list_end;

  align(4, true);
  write(*this, smallid);
  write(*this, uint16_t(0));
}

void xcdr_v1_stream::move_final_list_entry()
{
  move(*this, uint32_t(0));
}

void xcdr_v1_stream::move_header(const entity_properties_t &props)
{
  move(*this, uint32_t(0));
  if (extended_header(props)) {
    move(*this, uint32_t(0));
    move(*this, uint32_t(0));
  }
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
