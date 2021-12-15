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

#include "dds/ddsrt/endian.h"
#include "dds/ddsrt/heap.h"
#include "dds/ddsrt/mh3.h"
#include "dds/ddsrt/md5.h"
#include "dds/ddsi/q_radmin.h"
#include "dds/ddsi/ddsi_serdata.h"
#include "dds/ddsi/ddsi_sertype.h"
#include "dds/ddsi/ddsi_typelib.h"
#include "dds/ddsi/ddsi_cdrstream.h"
#include "org/eclipse/cyclonedds/core/cdr/cdr_enums.hpp"

struct ddsi_sertype;
template <typename T, class S> class ddscxx_sertype;

namespace org
{
namespace eclipse
{
namespace cyclonedds
{

namespace core
{
namespace cdr
{
//forward declarations of streamer types
class basic_cdr_stream;
class xcdr_v1_stream;
class xcdr_v2_stream;
}
}

namespace topic
{

using org::eclipse::cyclonedds::core::cdr::extensibility;
using org::eclipse::cyclonedds::core::cdr::encoding_version;

template <class TOPIC> class TopicTraits
{
public:

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
        return false;
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
     * Used by CycloneDDS-CXX to get a sertype, which contains the functions used by CycloneDDS which are specific to TOPIC.
     *
     * @param[in] kind The serialization of the of the sertype to create.
     * @return A pointer to a new dssi_sertype.
     */
    static ddsi_sertype *getSerType(encoding_version kind = minXCDRVersion())
    {
        switch (kind) {
            case encoding_version::basic_cdr:
                return static_cast<ddsi_sertype*>(new ddscxx_sertype<TOPIC,org::eclipse::cyclonedds::core::cdr::basic_cdr_stream>());
                break;
            case encoding_version::xcdr_v1:
                return static_cast<ddsi_sertype*>(new ddscxx_sertype<TOPIC,org::eclipse::cyclonedds::core::cdr::xcdr_v1_stream>());
                break;
            case encoding_version::xcdr_v2:
                return static_cast<ddsi_sertype*>(new ddscxx_sertype<TOPIC,org::eclipse::cyclonedds::core::cdr::xcdr_v2_stream>());
                break;
        }
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

#ifdef OMG_DDS_EXTENSIBLE_AND_DYNAMIC_TOPIC_TYPE_SUPPORT
    /**
     * @brief Returns the typeid for TOPIC.
     *
     * Is a simple pass-through for the derivation of the typeid from the type information.
     * As C++ keeps the topic type as the template parameter, there is no need to look at the
     * sertype for this topic.
     *
     * @param[in] kind The kind of typeid.
     *
     * @return A pointer to the typeid of this topic.
     */
    static ddsi_typeid_t* getTypeId(const struct ddsi_sertype *, ddsi_typeid_kind_t kind)
    {
        return ddsi_typeinfo_typeid(TopicTraits<TOPIC>::getTypeInfo(NULL), kind);
    }

    /**
     * @brief Returns the type map for TOPIC.
     *
     * Takes the type map blob for this topic which is part of the generated type traits, and deserializes
     * the type map from this blob.
     *
     * @return A pointer to the typemap for this topic.
     */
    static ddsi_typemap_t* getTypeMap(const struct ddsi_sertype *)
    {
        ddsi_sertype_cdr_data cdr{TopicTraits<TOPIC>::type_map_blob_sz, const_cast<uint8_t*>(TopicTraits<TOPIC>::type_map_blob)};
        return ddsi_typemap_deser(&cdr);
    }

    /**
     * @brief Returns the type info for TOPIC.
     *
     * Takes the type info blob for this topic which is part of thegenerated type traits, and deserializes
     * the type info from this blob.
     *
     * @return A pointer to the typeinfo for this topic.
     */
    static ddsi_typeinfo_t* getTypeInfo(const struct ddsi_sertype *)
    {
        ddsi_sertype_cdr_data cdr{TopicTraits<TOPIC>::type_info_blob_sz, const_cast<uint8_t*>(TopicTraits<TOPIC>::type_info_blob)};
        return ddsi_typeinfo_deser(&cdr);
    }

    /**
     * @brief Checks assignability between TOPIC and another type.
     *
     * Checks whether an instance object of type A can be assigned to one of type B.
     * This is allowed when all members in A also, occur in B.
     *
     * @param[in] sertype_a The first type to compare.
     * @param[in] type_pair_b The second type to compare (both minimal and complete representations).
     *
     * @return Whether this is so.
     */
    static bool assignableFrom(const struct ddsi_sertype *sertype_a, const struct ddsi_type_pair *type_pair_b)
    {
        assert (type_pair_b);
        struct ddsi_domaingv *gv = static_cast<struct ddsi_domaingv *>(ddsrt_atomic_ldvoidp (&sertype_a->gv));
        ddsi_typeid_t *type_id = TopicTraits<TOPIC>::getTypeId (sertype_a, DDSI_TYPEID_KIND_MINIMAL);
        assert (type_id);
        struct ddsi_type *type_a = ddsi_type_lookup_locked (gv, type_id);
        ddsi_typeid_fini (type_id);
        dds_free (type_id);
        if (!type_a) {
            type_id = TopicTraits<TOPIC>::getTypeId (sertype_a, DDSI_TYPEID_KIND_COMPLETE);
            type_a = ddsi_type_lookup_locked (gv, type_id);
            assert (type_id);
            ddsi_typeid_fini (type_id);
            dds_free (type_id);
        }
        return ddsi_is_assignable_from (gv, type_a, type_pair_b);
    }

private:
    static const unsigned int type_map_blob_sz; /**< Size of blob of the cdr serialized type map, needs to be instantiated for all declared types.*/
    static const unsigned int type_info_blob_sz; /**< Size of blob of the cdr serialized type info, needs to be instantiated for all declared types.*/
    static const unsigned char type_map_blob[]; /**< Blob of the cdr serialized type map, needs to be instantiated for all declared types.*/
    static const unsigned char type_info_blob[]; /**< Blob of the cdr serialized type info, needs to be instantiated for all declared types.*/
#endif  //DDSCXX_HAS_TYPE_DISCOVERY

public:
    /**
     * @brief Returns a pointer to the derived sertype.
     *
     * Returns a nullptr if no type can be derived succesfully.
     *
     * @param[in] data_representation The type of data representation to use.
     * @return The pointer to the derived sertype.
     */
    static struct ddsi_sertype* deriveSertype(const struct ddsi_sertype *, dds_data_representation_id_t data_representation, dds_type_consistency_enforcement_qospolicy_t)
    {
        if (minXCDRVersion() == encoding_version::basic_cdr) {
            switch (data_representation) {
                case DDS_DATA_REPRESENTATION_XCDR1:
                    return getSerType(encoding_version::basic_cdr);
                    break;
                case DDS_DATA_REPRESENTATION_XCDR2:
                    return getSerType(encoding_version::xcdr_v2);
                    break;
            }
        } else if (minXCDRVersion() == encoding_version::xcdr_v2) {
            if (data_representation == DDS_DATA_REPRESENTATION_XCDR2)
              return getSerType(encoding_version::xcdr_v2);
        }
        return nullptr;
    }
};

}
}
}
}

#endif /* CYCLONEDDS_TOPIC_TOPICTRAITS_HPP_ */
