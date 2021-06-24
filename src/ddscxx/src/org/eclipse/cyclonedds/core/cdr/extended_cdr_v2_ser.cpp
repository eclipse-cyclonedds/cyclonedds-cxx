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

void xcdr_v2_stream::start_member(entity_properties_t &prop, bool present)
{
  switch (m_mode) {
    case stream_mode::write:
      if (em_header_necessary(prop)) {
        if (present)
          write_em_header(prop);
      } else if (prop.is_optional) {
        write_optional_tag(prop, present);
      }
      break;
    case stream_mode::move:
    case stream_mode::max:
      if (em_header_necessary(prop)) {
        if (present)
          move_em_header(prop);
      } else if (prop.is_optional) {
        move_optional_tag();
      }
      break;
    default:
      break;
  }
}

void xcdr_v2_stream::finish_member(entity_properties_t &prop, bool present)
{
  if (m_mode == stream_mode::write) {
    prop.e_sz = static_cast<uint32_t>(position()-prop.e_off);
    if (em_header_necessary(prop)) {
      if (present)
        finish_em_header(prop);
    }
  }
}

void xcdr_v2_stream::write_optional_tag(entity_properties_t &props, bool present)
{
  write(*this, present ? uint8_t(1) : uint8_t(0));
  props.e_off = position();
}

void xcdr_v2_stream::move_optional_tag()
{
  move(*this, uint8_t(0));
}

void xcdr_v2_stream::skip_entity(const entity_properties_t &props)
{
  incr_position(props.e_sz);
}

bool xcdr_v2_stream::bytes_available(const entity_properties_t &props)
{
  if (position() >= props.d_off + props.d_sz) {
    if (position() > props.d_off + props.d_sz)
        status(invalid_dl_entry);

    return false;
  }
  return true;
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

      if (d_header_necessary(props)
       && !bytes_available(props))
        break;

      bool fieldpresent = true;
      if (prop.is_optional)
        read(*this, fieldpresent);

      if (fieldpresent)
        return prop;
    }
  } else {
    proplist *ptr = NULL;
    if (m_key)
      ptr = &props.m_keys;
    else
      ptr = &props.m_members_by_id;

    while (bytes_available(props)) {  //using while loop to prevent recursive calling, which could lead to stack overflow

      read_em_header(m_current_header);

      if (0 == m_current_header.e_sz)  //this field is empty
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

void xcdr_v2_stream::read_em_header(entity_properties_t &props)
{
  props = entity_properties_t();

  uint32_t emheader = 0;
  read(*this,emheader);

  uint32_t factor = 0;
  props.must_understand = emheader & must_understand;
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
    read(*this, props.e_sz);
    props.e_sz *= factor;
    //move cursor back 4 bytes, due to overlap of nextint and entity
    if ((emheader & lc_mask) > nextint)
      position(position()-4);
  }
}

void xcdr_v2_stream::read_d_header(entity_properties_t &props)
{
  read(*this, props.d_sz);
  props.d_off = position();
}

bool xcdr_v2_stream::d_header_necessary(const entity_properties_t &props)
{
  return (props.e_ext == ext_appendable || props.e_ext == ext_mutable)
      && !m_key;
}

bool xcdr_v2_stream::list_necessary(const entity_properties_t &props)
{
  return props.e_ext == ext_mutable && !m_key;
}

void xcdr_v2_stream::start_struct(entity_properties_t &props)
{
  if (!d_header_necessary(props))
    return;

  switch (m_mode) {
    case stream_mode::write:
      write_d_header(props);
      break;
    case stream_mode::move:
    case stream_mode::max:
      move_d_header();
      break;
    case stream_mode::read:
      read_d_header(props);
      break;
    default:
      break;
  }
}

void xcdr_v2_stream::finish_struct(entity_properties_t &props)
{
  if (d_header_necessary(props) && m_mode == stream_mode::write)
    finish_d_header(props);
}

void xcdr_v2_stream::write_d_header(entity_properties_t &props)
{
  write(*this, uint32_t(0));

  props.d_off = position();
}

void xcdr_v2_stream::write_em_header(entity_properties_t &props)
{
  uint32_t mheader = (props.must_understand ? must_understand : 0)
                     + (id_mask & props.m_id);

  switch (props.e_bb) {
    case bb_8_bits:
      mheader += bytes_1;
      break;
    case bb_16_bits:
      mheader += bytes_2;
      break;
    case bb_32_bits:
      mheader += bytes_4;
      break;
    case bb_64_bits:
      mheader += bytes_8;
      break;
    default:
      mheader += nextint;
  }

  write(*this, mheader);
  if (props.e_bb == bb_unset)
    write(*this, uint32_t(0));

  props.e_off = position();
}

void xcdr_v2_stream::move_d_header()
{
  move(*this, uint32_t(0));
}

void xcdr_v2_stream::move_em_header(const entity_properties_t &props)
{
  move(*this, uint32_t(0));
  if (props.e_bb == bb_unset)
    move(*this, uint32_t(0));
}

void xcdr_v2_stream::finish_d_header(entity_properties_t &props)
{
  auto current_position = position();
  props.d_sz = static_cast<uint32_t>(current_position - props.d_off);

  if (props.d_sz == 0)
    return;

  auto current_alignment = alignment();

  position(props.d_off - 4);
  write(*this, props.d_sz);

  position(current_position);
  alignment(current_alignment);
}

void xcdr_v2_stream::finish_em_header(entity_properties_t &props)
{
  if (props.e_bb != bb_unset || props.e_sz == 0)
    return;

  auto current_position = position();
  auto current_alignment = alignment();

  position(props.e_off - 4);
  write(*this, props.e_sz);

  position(current_position);
  alignment(current_alignment);
}

bool xcdr_v2_stream::em_header_necessary(const entity_properties_t &props)
{
  return (props.p_ext == ext_mutable) && !m_key;
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
