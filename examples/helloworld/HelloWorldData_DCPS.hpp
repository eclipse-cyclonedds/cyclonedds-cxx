#ifndef ISOCPP2_HELLOWORLDDATA_H
#define ISOCPP2_HELLOWORLDDATA_H

#include "dds/dds.hpp"

#include "datatopic.hpp"

#include "HelloWorldData.h"
//#include "HelloWorldDataSplDcps.h"

#include "org/eclipse/cyclonedds/topic/TopicTraits.hpp"
#include "org/eclipse/cyclonedds/topic/DataRepresentation.hpp"

namespace org { namespace eclipse { namespace cyclonedds { namespace topic {
template <>
class TopicTraits<HelloWorldData::Msg>
{
public:
    static ::org::eclipse::cyclonedds::topic::DataRepresentationId_t getDataRepresentationId()
    {
        return ::org::eclipse::cyclonedds::topic::OSPL_REPRESENTATION;
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
        return "userID";
    }

    static const char *getTypeName()
    {
        return "HelloWorldData::Msg";
    }

    static ddsi_sertopic *getSerTopic(const std::string& topic_name)
    {
        helloworld_sertopic *st = helloworld_sertopic::create_sertopic(
          topic_name, getTypeName());
        return st;
    }

#if 0
    static const dds_topic_descriptor_t *getDescriptor()
    {
        return &HelloWorldData_Msg_desc;
    }
#endif

    static size_t getSampleSize()
    {
        return sizeof(HelloWorldData::Msg);
    }
};
}}}}

namespace dds { namespace topic {
template <>
struct topic_type_name<HelloWorldData::Msg>
{
    static std::string value()
    {
        return org::eclipse::cyclonedds::topic::TopicTraits<HelloWorldData::Msg>::getTypeName();
    }
};
}}

REGISTER_TOPIC_TYPE(HelloWorldData::Msg)
#endif /* ISOCPP_HELLOWORLDDATA_H */
