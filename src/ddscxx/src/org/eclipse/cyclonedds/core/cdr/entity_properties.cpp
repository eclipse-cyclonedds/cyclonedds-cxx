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

#include <org/eclipse/cyclonedds/core/cdr/entity_properties.hpp>
#include <iostream>
#include <algorithm>
#include <cassert>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

bool entity_properties::member_id_comp(const entity_properties_t &lhs, const entity_properties_t &rhs)
{
  if (!rhs && lhs)
    return true;
  if (rhs && !lhs)
    return false;

  return lhs.m_id < rhs.m_id;
}

void entity_properties::print(bool recurse, size_t depth, const char *prefix) const
{
  std::cout << "d: " << depth;
  for (size_t i = 0; i < depth; i++) std::cout << "  ";
  std::cout << prefix << ": m_id: " << m_id << " final: " << (is_last ? "yes" : "no") << " m_u: " << (must_understand_local ? "yes" : "no") << " key: " << (is_key ? "yes" : "no");

  std::cout << " p_ext: ";
  switch(p_ext) {
    case extensibility::ext_final:
    std::cout << "FINAL";
    break;
    case extensibility::ext_appendable:
    std::cout << "APPENDABLE";
    break;
    case extensibility::ext_mutable:
    std::cout << "MUTABLE";
    break;
  }
  std::cout << " e_ext: ";
  switch(e_ext) {
    case extensibility::ext_final:
    std::cout << "FINAL";
    break;
    case extensibility::ext_appendable:
    std::cout << "APPENDABLE";
    break;
    case extensibility::ext_mutable:
    std::cout << "MUTABLE";
    break;
  }
  std::cout << std::endl;
  if (recurse) {
    for (const auto & e:m_members_by_seq) e.print(true, depth+1, "member_s");
    for (const auto & e:m_members_by_id) e.print(true, depth+1, "member_i");
    for (const auto & e:m_keys) e.print(true, depth+1, "key     ");
  }
}

void entity_properties::set_member_props(uint32_t member_id, bool optional)
{
  m_id = member_id;
  is_optional = optional;
}

void key_endpoint::add_key_endpoint(const std::list<uint32_t> &key_indices)
{
  auto *ptr = this;
  for (const auto &key_index:key_indices) {
    auto it = ptr->find(key_index);
    if (it != ptr->end()) {
      ptr = &(it->second);
    } else {
      auto p = ptr->insert({key_index,key_endpoint()});
      assert(p.second);
      ptr = &(p.first->second);
    }
  }
}

void entity_properties::set_key_values(const key_endpoint &endpoints)
{
  if (!endpoints)
    return;

  //there are key members for this entity, therefore erase all existing key member information
  for (auto &member:m_members_by_seq) {
    if (member.is_key) {
      member.is_key = false;
      member.must_understand_local = false;
    }
  }

  //look for all indicated key members, and set their key values
  for (const auto &p:endpoints) {
    auto it = std::find(m_members_by_seq.begin(), m_members_by_seq.end(), entity_properties(p.first));
    assert(it != m_members_by_seq.end());

    it->is_key = true;
    it->must_understand_local = true;
    it->set_key_values(p.second);
  }
}

bool entity_properties::requires_xtypes() const
{
  if (xtypes_necessary || is_optional || e_ext != extensibility::ext_final)
    return true;

  for (const auto &member:m_members_by_seq)
    if (member.requires_xtypes())
      return true;

  return false;
}

void entity_properties::clear()
{
  m_members_by_seq.clear();
  m_members_by_id.clear();
  m_keys.clear();
}

void entity_properties::finish(const key_endpoint &keys)
{
  set_key_values(keys);

  xtypes_necessary = requires_xtypes();
  for (auto &member:m_members_by_seq) {
    member.propagate_flags();
  }

  populate_from_seq();

  trim_non_key_members();
}

void entity_properties::trim_non_key_members()
{
  if (m_keys.empty())
    return;

  auto it = m_keys.begin();
  while (*it) {
    if (it->is_key) {
      it->trim_non_key_members();
      it++;
    } else {
      it = m_keys.erase(it);
    }
  }
}

void entity_properties::propagate_flags()
{
  if (!is_key)
    return;

  bool any_member_is_key = false;
  for (const auto &member:m_members_by_seq) {
    if (member.is_key)
      any_member_is_key = true;
  }

  for (auto &member:m_members_by_seq) {
    if (!any_member_is_key) {
      member.is_key = true;
      member.must_understand_local = true;
    }

    member.propagate_flags();
  }
}

void entity_properties::populate_from_seq(keep_in_propagate to_keep)
{
  m_members_by_id = m_members_by_seq;
  m_members_by_id.sort(member_id_comp);
  m_keys = m_members_by_id;

  if (to_keep & members_by_seq) {
    for (auto &member:m_members_by_seq)
      member.populate_from_seq(members_by_seq);
  } else {
    m_members_by_seq.clear();
  }

  if (to_keep & members_by_id) {
    for (auto &member:m_members_by_id)
      member.populate_from_seq(members_by_id);
  } else {
    m_members_by_id.clear();
  }

  if (to_keep & keys) {
    for (auto &member:m_keys)
      member.populate_from_seq(keys);
  } else {
    m_keys.clear();
  }

}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
