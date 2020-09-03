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

#include <string.h>
#include <org/eclipse/cyclonedds/topic/DataRepresentation.hpp>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace topic
{

typedef void (*copyInFunction)(const void *, void *);
typedef void (*copyOutFunction)(const void *, void *);


template <class TOPIC> class TopicTraits
{
public:
    static ::org::eclipse::cyclonedds::topic::DataRepresentationId_t getDataRepresentationId()
    {
        return ::org::eclipse::cyclonedds::topic::INVALID_REPRESENTATION;
    }

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

    static const char *getKeyList()
    {
        return "ExampleKeylist";
    }

    static const char *getTypeName()
    {
        return "ExampleName";
    }

    static ddsi_sertopic *getSerTopic(const std::string& topic_name)
    {
        return NULL;
    }

    static size_t getSampleSize()
    {
        return 0;
    }
};

}
}
}
}

#endif /* CYCLONEDDS_TOPIC_TOPICTRAITS_HPP_ */
