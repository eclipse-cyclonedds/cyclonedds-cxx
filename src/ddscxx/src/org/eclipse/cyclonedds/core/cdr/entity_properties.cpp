// Copyright(c) 2021 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <org/eclipse/cyclonedds/core/cdr/entity_properties.hpp>
#include <iostream>
#include <algorithm>
#include <cassert>
#include <string>

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

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

static void add_key(const key_endpoint &key, entity_properties_t &props)
{
  if (key.size()) {  //something is setting key values at this level, set all keys to false
    props.erase_key_values();
  } else if (props.parent) {
    props.set_key_values();
  }

  for (const auto & k:key) {
    auto ptr = props.first_member;  //look for the entry with the key id
    while (ptr) {
      if (ptr->m_id == k.first) {
        break;
      } else {
        ptr = ptr->next_on_level;
      }
    }

    assert(ptr);
    ptr->is_key = true;  //set this to be a key
    add_key(k.second, *ptr);
  }
}

void entity_properties_t::finish(propvec &props, const key_endpoint &keys)
{
  assert(props.size());

  for (size_t i = 0; i < props.size(); i++) {
    auto ptr = props.data()+i;
    for (size_t j = i+1; j < props.size(); j++) {
      auto ptr2 = props.data()+j;
      if (ptr->depth == ptr2->depth) {
        ptr->next_on_level = ptr2;
        ptr2->prev_on_level = ptr;
        break;
      } else if (ptr2->depth < ptr->depth) {
        break;
      }
    }

    entity_properties_t *parent = nullptr;
    if (ptr->prev_on_level)
      parent = ptr->prev_on_level->parent;
    else if (ptr->depth)
      parent = ptr-1;

    ptr->parent = parent;
    if (ptr->prev_on_level == nullptr && parent)
      parent->first_member = ptr;

    if (ptr->parent)
      ptr->p_ext = ptr->parent->e_ext;
  }

  auto &root = props[0];
  auto ptr = root.first_member;
  while (ptr && !root.xtypes_necessary) {
    root.xtypes_necessary |= ptr->xtypes_necessary;
    ptr = ptr->next_on_level;
  }

  //use key endpoints
  add_key(keys, props[0]);
}

void entity_properties_t::print() const
{
  std::cout <<  std::string( 2*depth, ' ' ) << "id: " << m_id << std::endl;
  std::cout <<  std::string( 2*depth+2, ' ' ) << "key: " << (is_key ? "true" : "false") << ", optional: " << (is_optional ? "true" : "false") << ", has parent: " << (parent ? "true" : "false") << std::endl;
  std::cout <<  std::string( 2*depth+2, ' ' ) << "must_understand: " << (must_understand ? "true" : "false") << ", xtypes necessary: " << (xtypes_necessary ? "true" : "false") << std::endl;
}

bool entity_properties_t::has_keys() const
{
  auto ptr = first_member;
  while (ptr) {
    if (ptr->is_key)
      return true;
    ptr = ptr->next_on_level;
  }

  return false;
}

void entity_properties_t::set_key_values()
{
  auto h_k = has_keys();

  auto ptr = first_member;
  while (ptr) {
    if (!h_k)
      ptr->is_key = true;
    if (ptr->is_key)
      ptr->set_key_values();
    ptr = ptr->next_on_level;
  }
}

void entity_properties_t::erase_key_values()
{
  auto ptr = first_member;
  while (ptr) {
    ptr->is_key = false;
    ptr = ptr->next_on_level;
  }
}

void entity_properties_t::append_struct_contents(propvec &appendto, const propvec &toappend)
{
  auto oldsize = appendto.size();

  appendto.insert(appendto.end(), toappend.begin()+1, toappend.end());

  for (size_t i = oldsize; i < appendto.size(); i++) {
    appendto[i].depth++;
    appendto[i].next_on_level = nullptr;
    appendto[i].prev_on_level = nullptr;
    appendto[i].parent        = nullptr;
    appendto[i].first_member  = nullptr;
  }
}

void entity_properties_t::print(const propvec &in)
{
  for (const auto & e:in)
    e.print();
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */
