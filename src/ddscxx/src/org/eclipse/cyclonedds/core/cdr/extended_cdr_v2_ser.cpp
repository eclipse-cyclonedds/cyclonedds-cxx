// Copyright(c) 2020 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

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

bool xcdr_v2_stream::start_member(const entity_properties_t &prop, bool is_set)
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
      if (em_header_necessary(prop))
        m_buffer_end.push(position() + m_e_sz.top());
      break;
    default:
      break;
  }

  m_consecutives.push({false, false});
  push_member_start();
  return cdr_stream::start_member(prop, is_set);
}

bool xcdr_v2_stream::finish_member(const entity_properties_t &prop, member_id_set &member_ids, bool is_set)
{
  switch (m_mode) {
    case stream_mode::write:
      if (em_header_necessary(prop)) {
        if (is_set && !finish_em_header())
          return false;
      }
      break;
    case stream_mode::read:
      if (em_header_necessary(prop))
        m_buffer_end.pop();
      break;
    default:
      break;
  }

  m_consecutives.pop();
  pop_member_start();
  return cdr_stream::finish_member(prop, member_ids, is_set);
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

const entity_properties_t* xcdr_v2_stream::next_entity(const entity_properties_t *prop)
{
  if (m_mode != stream_mode::read)
    return cdr_stream::next_entity(prop);

  if (!list_necessary(*(prop->parent))) {
    while ((prop = cdr_stream::next_entity(prop))) {
      if (prop->p_ext == extensibility::ext_appendable &&
          !bytes_available(1, true))
        return nullptr;
      bool fieldpresent = true;
      if (prop->is_optional) {
        if (!read(*this, fieldpresent))
          return nullptr;
      }
      if (fieldpresent)
        break;
    }
  } else {
    while (1) {  //using while loop to prevent recursive calling, which could lead to stack overflow
      entity_properties_t temp;
      if (!bytes_available(4, true) ||
          !read_em_header(temp)) {
        return nullptr;  //no more fields to read
      } else if (0 == m_e_sz.top()) {
        continue; //this field is empty
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

const entity_properties_t* xcdr_v2_stream::first_entity(const entity_properties_t *props)
{
  auto prop = cdr_stream::first_entity(props);
  if (m_mode != stream_mode::read || !prop)
    return prop;

  if (!list_necessary(*props)) {
    do {
      bool fieldpresent = true;
      if (prop->is_optional) {
        if (!read(*this, fieldpresent))
          return nullptr;
      }
      if (fieldpresent)
        break;
    } while ((prop = cdr_stream::next_entity(prop)));
  } else {
    while (1) {  //using while loop to prevent recursive calling, which could lead to stack overflow
      entity_properties_t temp;
      if (!bytes_available(4, true) ||
          !read_em_header(temp)) {
        return nullptr;  //no more fields to read
      } else if (0 == m_e_sz.top()) {
        continue; //this field is empty
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

bool xcdr_v2_stream::read_em_header(entity_properties_t &props)
{
  uint32_t emheader = 0;
  if (!read(*this,emheader))
    return false;

  uint32_t factor = 0;
  props.must_understand = emheader & must_understand;
  props.m_id = emheader & id_mask;
  switch (emheader & lc_mask) {
    case bytes_1:
      m_e_sz.top() = 1;
      break;
    case bytes_2:
      m_e_sz.top() = 2;
      break;
    case bytes_4:
      m_e_sz.top() = 4;
      break;
    case bytes_8:
      m_e_sz.top() = 8;
      break;
    case nextint:
      if (!read(*this, m_e_sz.top()))
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
    if (!read(*this, m_e_sz.top()))
      return false;
    m_e_sz.top() *= factor;
    m_e_sz.top() += 4;
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

bool xcdr_v2_stream::start_struct(const entity_properties_t &props)
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

  return cdr_stream::start_struct(props);
}

bool xcdr_v2_stream::finish_struct(const entity_properties_t &props, const member_id_set &member_ids)
{
  switch (m_mode) {
    case stream_mode::write:
      if (d_header_necessary(props) && !finish_d_header())
        return false;
      break;
    case stream_mode::read:
      if (!check_struct_completeness(props, member_ids))
        return false;
      else if (d_header_necessary(props))
        m_buffer_end.pop();
      break;
    default:
      break;
  }

  return true;
}

bool xcdr_v2_stream::start_consecutive(bool is_array, bool primitive)
{
  if (m_key)
    return true;

  bool d_hdr_necessary =  false;
  if (!primitive) {
    if (is_array) {
      d_hdr_necessary = m_consecutives.size() == 0 || !m_consecutives.top().is_array;
    } else {
      d_hdr_necessary = true;
    }
  }

  if (d_hdr_necessary) {
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
        assert(0);
    }
  }

  m_consecutives.push({is_array, d_hdr_necessary});

  return true;
}

bool xcdr_v2_stream::finish_consecutive()
{
  if (m_key)
    return true;

  assert(m_consecutives.size());
  bool d_hdr = m_consecutives.top().d_header_present;
  m_consecutives.pop();
  if (d_hdr) {
    switch (m_mode) {
      case stream_mode::write:
        if (!finish_d_header())
          return false;
        break;
      case stream_mode::move:
      case stream_mode::max:
        break;
      case stream_mode::read:
        m_buffer_end.pop();
        break;
      default:
        assert(0);
    }
  }

  return true;
}

bool xcdr_v2_stream::write_em_header(const entity_properties_t &props)
{
  uint32_t mheader = (props.must_understand || props.is_key ? must_understand : 0)
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
  m_delimiters.reset();
  m_consecutives.reset();
}

bool xcdr_v2_stream::finish_em_header()
{
  uint32_t e_sz = static_cast<uint32_t>(position()-m_e_off.top());
  if (e_sz == 0)
    return true;

  auto current_position = position();
  auto current_alignment = alignment();

  position(m_e_off.top() - 4);
  alignment(4);
  if (!write(*this, e_sz))
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
