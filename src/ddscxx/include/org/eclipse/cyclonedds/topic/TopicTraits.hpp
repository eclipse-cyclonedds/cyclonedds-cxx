/*
 * Copyright(c) 2006 to 2020 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */


/**
 * @file
 */

#ifndef CYCLONEDDS_TOPIC_TOPICTRAITS_HPP_
#define CYCLONEDDS_TOPIC_TOPICTRAITS_HPP_

#include <vector>

struct ddsi_sertype;

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace topic
{

/**
 * @brief
 * Entity extensibility descriptors.
 *
 * @enum extensibility Describes the extensibility of entities.
 *
 * This value is set for entities and their parents.
 *
 * @var extensibility::ext_final The entity representation is complete, no fields can be added or removed.
 * @var extensibility::ext_appendable The entity representation can be extended, no fields can be removed.
 * @var extensibility::ext_mutable The entity representation can be modified, fields can be removed or added.
 */
enum class extensibility {
  ext_final,
  ext_appendable,
  ext_mutable
};

/**
 * @brief
 * Encoding version descriptors.
 *
 * @enum encoding_version Describes the CDR encoding version of entities.
 *
 * @var encoding_version::basic_cdr Basic CDR encoding, does not support any xtypes functionality.
 * @var encoding_version::xcdr_v1 Version 1 Xtypes CDR encoding (deprecated).
 * @var encoding_version::xcdr_v2 Version 2 XTypes CDR encoding.
 */
enum class encoding_version {
  basic_cdr,
  xcdr_v1,
  xcdr_v2
};

template <class TOPIC> class TopicTraits
{
public:

    static ::std::vector<uint8_t> getMetaData()
    {
        return ::std::vector<uint8_t>();
    }

    static ::std::vector<uint8_t> getTypeHash()
    {
        return ::std::vector<uint8_t>();
    }

    static ::std::vector<uint8_t> getExtentions()
    {
        return ::std::vector<uint8_t>();
    }

    /**
     * @brief Returns whether TOPIC contains no key fields.
     *
     * Used in creating the CycloneDDS writer and equality comparisons with other topics.
     * This is one of the traits that is conditionally generated if there are any key fields.
     *
     * @return Whether TOPIC does not contain any fields marked as key fields.
     */
    static constexpr bool isKeyless()
    {
        return true;
    }

    /**
     * @brief Returns the name of the type of TOPIC.
     *
     * Used in creating the correct TopicDescription.
     * This trait is always generated for user-defined types, and this function is just a placeholder.
     *
     * @return The name of the type of TOPIC.
     */
    static constexpr const char *getTypeName()
    {
        return "";
    }

    /**
     * @brief Returns an instance of ddsi_sertype for TOPIC.
     *
     * Used by CycloneDDS to get a sertype, which contains the functions used by CycloneDDS which are specific to TOPIC.
     * This trait is always generated for user-defined types, and this function is just a placeholder.
     *
     * @return A pointer to a new dssi_sertype.
     */
    static inline ddsi_sertype *getSerType()
    {
        return nullptr;
    }

    /**
     * @brief Returns the size of an instance of TOPIC.
     *
     * Used by shared memory implementation to determine the size of the block necessary to contain an instance of TOPIC.
     *
     * @return The size of an instance of TOPIC.
     */
    static constexpr size_t getSampleSize()
    {
        return sizeof(TOPIC);
    }

    /**
     * @brief Returns whether instances of TOPIC reference memory outside its own declaration.
     *
     * Used by shared memory implementation.
     * This trait will be generated as false if any strings or vectors are found anywhere in TOPIC's member tree.
     *
     * @return Whether TOPIC is a selfcontained type.
     */
    static constexpr bool isSelfContained()
    {
      return true;
    }

    /**
     * @brief Returns the minimum version of XCDR necessary to serialize TOPIC objects.
     *
     * Used by the serialization implementation in ddscxx_serdata to determine which serialization method to use.
     * This trait will be generated as xcdr_v2 if any optional or non-final members are found anywhere in
     * TOPIC's member tree or if TOPIC itself is not final.
     *
     * @return The minimum XCDR version necessary to serialize TOPIC.
     */
    static constexpr encoding_version minXCDRVersion()
    {
      return encoding_version::basic_cdr;
    }

    /**
     * @brief Returns the xtypes extensibility of TOPIC.
     *
     * Used to determine which encoding type to write in the CDR header.
     * This trait will be generated if the extensibility of TOPIC differs from final.
     *
     * @return The extensibility of TOPIC.
     */
    static constexpr extensibility getExtensibility()
    {
      return extensibility::ext_final;
    }
};

}
}
}
}

#endif /* CYCLONEDDS_TOPIC_TOPICTRAITS_HPP_ */
