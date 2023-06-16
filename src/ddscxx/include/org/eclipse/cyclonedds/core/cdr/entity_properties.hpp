// Copyright(c) 2021 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 
#ifndef ENTITY_PROPERTIES_HPP_
#define ENTITY_PROPERTIES_HPP_

#include <dds/core/macros.hpp>
#include <cstdint>
#include <list>
#include <vector>
#include <set>
#include <map>
#include <atomic>
#include <mutex>

#if DDSCXX_USE_BOOST
#include <boost/type_traits.hpp>
#define DDSCXX_STD_IMPL boost
#else
#include <type_traits>
#define DDSCXX_STD_IMPL std
#endif

#include "cdr_enums.hpp"

namespace org {
namespace eclipse {
namespace cyclonedds {
namespace core {
namespace cdr {

#define decl_ref_type(x) DDSCXX_STD_IMPL::remove_cv_t<DDSCXX_STD_IMPL::remove_reference_t<decltype(x)>>

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
enum class bit_bound {
  bb_unset = 0,
  bb_8_bits = 1,
  bb_16_bits = 2,
  bb_32_bits = 4,
  bb_64_bits = 8
};

/**
 * @brief
 * Key mode descriptors.
 *
 * @enum key_mode Describes the manner of writing of a stream.
 *
 * This value is used to determine which fields to write and in which order to write them.
 * Mainly used to get the first/next/previous entity from the entity properties describing a datatype.
 *
 * @var key_mode::unset The key_mode of streaming is unset.
 * @var key_mode::not_key The key_mode of streaming is not a key, all members will be streamed in declaration order.
 * @var key_mode::unsorted The key_mode of streaming is key mode, only members will be streamed in declaration order.
 * @var key_mode::sorted The key_mode of streaming is keyhash mode, only members will be streamed in member id order.
 */
enum class key_mode {
  unset,
  not_key,
  unsorted,
  sorted
};

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

/**
 * @brief
 * Generic get_bit_bound fallback function.
 *
 * Returns a bb_unset for all non-primitive, non-enum types or primitives/enums with non 1,2,4,8 sizes.
 *
 * @return bb_unset always.
 */
template<typename T, DDSCXX_STD_IMPL::enable_if_t<(!std::is_arithmetic<T>::value && !std::is_enum<T>::value) || (sizeof(T) != 1 && sizeof(T) != 2 && sizeof(T) != 4 && sizeof(T) != 8), bool> = true >
constexpr bit_bound get_bit_bound() { return bit_bound::bb_unset; }

/**
 * @brief
 * Primitive type/enum get_bit_bound function.
 *
 * Returns the bitbound for primitives/enums with size 1.
 * This function will be specialized for all enums with a manually defined bit_bound.
 *
 * @return bb_8_bits always.
 */
template<typename T, DDSCXX_STD_IMPL::enable_if_t<(std::is_arithmetic<T>::value || std::is_enum<T>::value) && sizeof(T) == 1, bool> = true >
constexpr bit_bound get_bit_bound() { return bit_bound::bb_8_bits; }

/**
 * @brief
 * Primitive type/enum get_bit_bound function.
 *
 * Returns the bitbound for primitives/enums with size 2.
 * This function will be specialized for all enums with a manually defined bit_bound.
 *
 * @return bb_16_bits always.
 */
template<typename T, DDSCXX_STD_IMPL::enable_if_t<(std::is_arithmetic<T>::value || std::is_enum<T>::value) && sizeof(T) == 2, bool> = true >
constexpr bit_bound get_bit_bound() { return bit_bound::bb_16_bits; }

/**
 * @brief
 * Primitive type/enum get_bit_bound function.
 *
 * Returns the bitbound for primitives/enums with size 4.
 * This function will be specialized for all enums with a manually defined bit_bound.
 *
 * @return bb_32_bits always.
 */
template<typename T, DDSCXX_STD_IMPL::enable_if_t<(std::is_arithmetic<T>::value || std::is_enum<T>::value) && sizeof(T) == 4, bool> = true >
constexpr bit_bound get_bit_bound() { return bit_bound::bb_32_bits; }


/**
 * @brief
 * Primitive type/enum get_bit_bound function.
 *
 * Returns the bitbound for primitives/enums with size 8.
 * This function will be specialized for all enums with a manually defined bit_bound.
 *
 * @return bb_64_bits always.
 */
template<typename T, DDSCXX_STD_IMPL::enable_if_t<(std::is_arithmetic<T>::value || std::is_enum<T>::value) && sizeof(T) == 8, bool> = true >
constexpr bit_bound get_bit_bound() { return bit_bound::bb_64_bits; }

typedef struct entity_properties entity_properties_t;
typedef std::vector<entity_properties_t> propvec;
typedef std::set<uint32_t> member_id_set;

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
    uint32_t _depth = 0,
    uint32_t _m_id = 0,
    bool _is_optional = false,
    bit_bound _bb = bit_bound::bb_unset,
    extensibility _ext = extensibility::ext_final,
    bool _must_understand = true):
      e_ext(_ext),
      m_id(_m_id),
      depth(_depth),
      must_understand(_must_understand),
      xtypes_necessary(_ext != extensibility::ext_final || _is_optional),
      is_optional(_is_optional),
      e_bb(_bb) {;}

  extensibility e_ext = extensibility::ext_final; /**< The extensibility of the entity itself. */
  extensibility p_ext = extensibility::ext_final; /**< The extensibility of the entity's parent. */
  uint32_t m_id = 0;                              /**< The member id of the entity, it is the global field by which the entity is identified. */
  uint32_t depth = 0;                             /**< The depth of this entity.*/
  bool must_understand = false;                   /**< If the reading end cannot parse a field with this header, it must discard the entire object.*/
  bool xtypes_necessary = false;                  /**< Is set if any of the members of this entity require xtypes support.*/
  bool implementation_extension = false;          /**< Can be set in XCDR_v1 stream parameter list headers.*/
  bool ignore = false;                            /**< Indicates that this field must be ignored.*/
  bool is_optional = false;                       /**< Indicates that this field can be empty (length 0) for reading/writing purposes.*/
  bool is_key = false;                            /**< Indicates that this field is a key field.*/
  bit_bound e_bb = bit_bound::bb_unset;           /**< The minimum number of bytes necessary to represent this entity/bitmask.*/

  entity_properties_t  *parent              = nullptr,  /**< Pointer to the parent of this entity.*/
                       *first_member        = nullptr,  /**< Pointer to the first entity which is a member of this entity.*/
                       *next_on_level       = nullptr,  /**< Pointer to the next entity on the same level.*/
                       *prev_on_level       = nullptr,  /**< Pointer to the previous entity on the same level.*/
                       *first_unsorted_key  = nullptr,  /**< Pointer to the first entity which is a key member of this entity, going by declaration order.*/
                       *next_unsorted_key   = nullptr,  /**< Pointer to the next entity which is a key member on the same level, going by declaration order.*/
                       *prev_unsorted_key   = nullptr,  /**< Pointer to the previous entity which is a key member on the same level, going by declaration order.*/
                       *first_sorted_key    = nullptr,  /**< Pointer to the first entity which is a key member of this entity, going by member id order.*/
                       *next_sorted_key     = nullptr,  /**< Pointer to the next entity which is a key member on the same level, going by member id order.*/
                       *prev_sorted_key     = nullptr;  /**< Pointer to the previous entity which is a key member on the same level, going by member id order.*/

  /**
   * @brief
   * Print function.
   *
   * This function write the contents (id, is_key, is_optional, must_understand, xtypes_necessary) of this entity to std::cout.
   */
  void print() const;

  /**
   * @brief
   * Sets is_key flags on members.
   *
   * This function will set is_key flags on all members if the entity is keyless
   * and will recursively call this on all members with the is_key flag set.
   * Used in the finish function to finish the entire entity_properties_t tree.
   */
  void set_key_values();

  /**
   * @brief
   * Removes all is_key flags from members.
   *
   * This function will set all is_key flags of members of this entity to false.
   * Used in the finish function to finish the entire entity_properties_t tree.
   */
  void erase_key_values();

  /**
   * @brief
   * Finishes a tree representing an entire datamodel.
   *
   * This function will set the next_on_level and prev_on_level pointers to create a sub linked list
   * of the members of the entity at that level. Also it will set the parent pointer of member entities
   * and the first_member pointer of entities which have sub entities (members).
   * When all this is done, it will take the key information in the key_endpoint map to (un)set the is_key
   * flags on all members.
   *
   * @param[in, out] props The list of entities representing the datamodel.
   * @param[in] keys The map of key indices.
   */
  static void finish(propvec &props, const key_endpoint &keys);

  /**
   * @brief
   * Appends a sub tree to the current tree as a member.
   *
   * This function will take the contents of toappend, insert them at the end of appendto and increase the
   * depth of all entities added. It is used to add a constructed type as a member of another constructed type.
   *
   * @param[in, out] appendto The tree to append to.
   * @param[in] toappend The sub tree to append.
   */
  static void append_struct_contents(propvec &appendto, const propvec &toappend);

  /**
   * @brief
   * Prints the contents of a tree representing a datatype to the screen.
   *
   * @param[in] in The tree to print.
   */
  static void print(const propvec &in);

  /**
    * @brief
    * Returns the previous entity at the current level (if any).
    *
    * @param[in] key The key mode to get the first entity for.
    *
    * @return Pointer to the first entity, or nullptr if there are none.
    */
  const entity_properties_t* first_entity(key_mode key) const;

  /**
    * @brief
    * Returns the next entity at the current level (if any).
    *
    * @param[in] key The key mode to get the next entity for.
    *
    * @return Pointer to the next entity, or nullptr if there are none.
    */
  const entity_properties_t* next_entity(key_mode key) const;

  /**
    * @brief
    * Returns the previous entity at the current level (if any).
    *
    * @param[in] key The key mode to get the previous entity for.
    *
    * @return Pointer to the previous entity, or nullptr if there are none.
    */
  const entity_properties_t* previous_entity(key_mode key) const;

private:

  /**
   * @brief
   * Checks whether this entity is not keyless.
   *
   * This function will check all members (if any) and if any of them has the is_key flag set, this will return true.
   * This function is called BEFORE the total key calculation is done, so looking at the pointers to key members does not work yet.
   * If none have this flag set, it will return false.
   * Used in the finish function to finish the entire entity_properties_t tree.
   *
   * @return true if any of the members have a key, false otherwise.
   */
  bool has_keys() const;
};

/**
 * @brief
 * Forward declaration for type properties getter function.
 *
 * This template function is replaced/implemented by the types implemented through IDL generation.
 * It generates a static container which is initialized the first time the function is called,
 * this is then returned.
 *
 * @return propvec "Tree" representing the type.
 */
template<typename T>
const propvec& get_type_props();

}
}
}
}
}  /* namespace org / eclipse / cyclonedds / core / cdr */

#endif
