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
#ifndef ENTITY_PROPERTIES_HPP_
#define ENTITY_PROPERTIES_HPP_

#include <dds/core/macros.hpp>
#include <cstdint>
#include <list>
#include <map>
#include <mutex>
#include "org/eclipse/cyclonedds/topic/TopicTraits.hpp"

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

/**
 * @brief
 * Bit bound descriptors.
 *
 * @enum bit_bound Describes the minimal bit width for enum and bitmask types.
 *
 * This value is unset for anything other than enums and bitmasks.
 * It describes the smallest piece of memory which is able to represent the entire range of values.
 *
 * @var bit_bound::bb_unset The bit width of the entity is unset.
 * @var bit_bound::bb_8_bits The bit width of the entity is at most 8 bits (1 byte).
 * @var bit_bound::bb_16_bits The bit width of the entity is at most 16 bits (2 bytes).
 * @var bit_bound::bb_32_bits The bit width of the entity is at most 32 bits (4 bytes).
 * @var bit_bound::bb_64_bits The bit width of the entity is at most 64 bits (8 bytes).
 */
enum bit_bound {
  bb_unset = 0,
  bb_8_bits = 1,
  bb_16_bits = 2,
  bb_32_bits = 4,
  bb_64_bits = 8
};

typedef struct entity_properties entity_properties_t;
typedef std::list<entity_properties_t> proplist;

/**
 * @brief
 * Helper struct to keep track of key endpoints of the a struct
 */
DDSCXX_WARNING_MSVC_OFF(4251)
class OMG_DDS_API key_endpoint: public std::map<uint32_t, key_endpoint> {
DDSCXX_WARNING_MSVC_ON(4251)
  public:
    void add_key_endpoint(const std::list<uint32_t> &key_indices);
    operator bool() const {return !empty();}
};

using org::eclipse::cyclonedds::topic::extensibility;

/**
 * @brief
 * Entity properties struct.
 *
 * This is a container for data fields inside message classes, both as a representation passed for writing
 * as well as headers taken from streams when reading in the appropriate manner.
 * Normally these are not used by the end-user, and the streaming functions all interact with these objects
 * through the get_type_props function, which is generated for all user supplied message types.
 */
struct OMG_DDS_API entity_properties
{
  entity_properties(
    uint32_t _m_id = 0,
    bool _is_optional = false):
      m_id(_m_id),
      is_optional(_is_optional) {;}

  extensibility e_ext = extensibility::ext_final; /**< The extensibility of the entity itself. */
  extensibility p_ext = extensibility::ext_final; /**< The extensibility of the entity's parent. */
  size_t e_off = 0; /**< The current offset in the stream at which the member field starts, does not include header. */
  size_t d_off = 0; /**< The current offset in the stream at which the struct starts, does not include header.*/
  uint32_t e_sz = 0; /**< The size of the current entity as member field (only used in reading from streams).*/
  uint32_t m_id = 0; /**< The member id of the entity, it is the global field by which the entity is identified. */
  bool must_understand_local = false; /**< If the reading end cannot parse a field with this header, it must discard the entire object. */
  bool must_understand_remote = false; /**< If the reading end cannot parse a field with this header, it must discard the entire object. */
  bool xtypes_necessary = false; /**< Is set if any of the members of this entity require xtypes support.*/
  bool implementation_extension = false;
  bool is_last = false; /**< Indicates terminating entry for reading/writing entities, will cause the current subroutine to end and decrement the stack.*/
  bool ignore = false; /**< Indicates that this field must be ignored.*/
  bool is_optional = false; /**< Indicates that this field can be empty (length 0) for reading/writing purposes.*/
  bool is_key = false; /**< Indicates that this field is a key field.*/
  bool is_present = false; /**< Indicates that this entity is present in the read stream.*/
  bit_bound e_bb = bb_unset; /**< The minimum number of bytes necessary to represent this entity/bitmask.*/

  DDSCXX_WARNING_MSVC_OFF(4251)
  proplist m_members_by_seq; /**< Fields in normal streaming mode, ordered by their declaration.*/
  proplist m_members_by_id; /**< Fields in normal streaming mode, ordered by their member id.*/
  proplist m_keys; /**< Fields in key streaming mode, ordered by their member id.*/
  DDSCXX_WARNING_MSVC_ON(4251)

  /**
   * @brief
   * Conversion to boolean operator.
   *
   * Checks whether the is_last flag is NOT set.
   * Exists to make iterating over lists of entity properties easier, as the last entry of a list should be
   * the one that converts to 'false', and the rest are all 'true'.
   */
  operator bool() const {return !is_last;}

  /**
   * @brief
   * Comparison operator.
   *
   * @param[in] other The other entity to compare to.
   *
   * @return True when member and sequence ids are the same.
   */
  bool operator==(const entity_properties_t &other) const {return m_id == other.m_id;}

  /**
   * @brief
   * Comparison function.
   *
   * Sorts by is_final and m_id, in that precedence.
   * Used in sorting lists of entity_properties by member id, which makes lookup of the entity
   * when receiving member id fields much quicker.
   *
   * @param[in] lhs First entity to be compared.
   * @param[in] rhs Second entity to be compared.
   *
   * @return Whether lhs should be sorted before rhs.
   */
  static bool member_id_comp(const entity_properties_t &lhs, const entity_properties_t &rhs);

  /**
   * @brief
   * Finishing function.
   *
   * Generates the m_members_by_id and m_keys from m_members_by_seq and the supplied indices.
   *
   * @param[in] keys The indices of members which are keys.
   */
  void finish(const key_endpoint &keys);

  /**
   * @brief
   * Member property setting function.
   *
   * Sets the m_id and is_optional values.
   * Created to not have to have a constructor with a prohibitively large number of parameters.
   *
   * @param[in] member_id Sets the m_id field.
   * @param[in] optional Sets the is_optional field.
   */
  void set_member_props(uint32_t member_id, bool optional);

  /**
   * @brief
   * Entity printing function.
   *
   * Prints the following information of the entity:
   * - m_id, s_id, (key)member status, which ordering is followed: sequence(declaration)/id
   * - whether it is a list terminating entry
   * - the extensibility of the parent and the entity itself
   *
   * @param[in] recurse Whether to print its own children.
   * @param[in] depth At which depth we are printing, determining the indentation at which is printed.
   * @param[in] prefix Which prefix preceeds the printed entity information.
   */
  void print(bool recurse = true, size_t depth = 0, const char *prefix = "") const;

  /**
   * @brief
   * Xtypes requirement function.
   *
   * @return Whether this contains features that cannot be sent through basic cdr serialization.
   */
  bool requires_xtypes() const;

  /**
   * @brief
   * Empties the instance.
   */
  void clear();
private:

  /**
   * @brief
   * Non-key member trimming function.
   *
   * Removes entries from the list of key members which do not have the is_key flag set.
   * This is called recursively on all members of the key list m_keys.
   */
  void trim_non_key_members();

  /**
   * @brief
   * must_understand and is_key flag propagation function.
   *
   * Will set the is_key and must_understand flag on all members of entities which are themselves
   * keys but themselves have no key members, indicating that the key stream for a this member is
   * all members of this entity.
   * This is called recursively on all members.
   */
  void propagate_flags();

  /**
   * @brief Represents which representations to keep in the function populate_from_seq.
   *
   * Used to remove unnecessary branches in the entity_properties tree.
   *
   * @var keep_in_propagate::all Keep all representations (only used at root level).
   * @var keep_in_propagate::members_by_seq Keep only the m_members_by_seq representation.
   * @var keep_in_propagate::members_by_id Keep only the m_members_by_id representation.
   * @var keep_in_propagate::keys Keep only the m_keys representation.
   */
  enum keep_in_propagate {
    members_by_seq = 1,
    members_by_id = 2,
    keys = 4,
    all = members_by_seq+members_by_id+keys
  };

  /**
   * @brief
   * Other representations population function.
   *
   * Generates the m_members_by_id and m_keys representations from m_members_by_seq.
   * Will discard unnecessary representations as indicated by to_keep.
   * Is called recursively on all representations which are kept.
   *
   * @param[in] to_keep Which representations to keep.
   */
  void populate_from_seq(keep_in_propagate to_keep = all);

  /**
   * @brief
   * Overwrites the existing key values.
   *
   * @param[in] endpoints A tree of indices indicating the which (sub)members are keys.
   */
  void set_key_values(const key_endpoint &endpoints);
};

struct OMG_DDS_API final_entry: public entity_properties_t {
  final_entry(): entity_properties_t() {
    is_last = true;
  }
};

template<typename T>
entity_properties_t get_type_props() {
  static std::mutex mtx;
  static entity_properties_t props;
  static bool initialized = false;

  if (initialized)
    return props;

  std::lock_guard<std::mutex> lock(mtx);
  props.clear();
  key_endpoint keylist;
  switch (sizeof(T)) {
    case 1:
      props.e_bb = bb_8_bits;
      break;
    case 2:
      props.e_bb = bb_16_bits;
      break;
    case 4:
      props.e_bb = bb_32_bits;
      break;
    case 8:
      props.e_bb = bb_64_bits;
      break;
  }
  props.m_members_by_seq.push_back(final_entry());
  props.m_members_by_id.push_back(final_entry());
  props.m_keys.push_back(final_entry());
  props.finish(keylist);
  initialized = true;
  return props;
}

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */

#endif
