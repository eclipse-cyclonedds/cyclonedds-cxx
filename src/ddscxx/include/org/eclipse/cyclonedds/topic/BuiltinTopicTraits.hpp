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
#ifndef CYCLONEDDS_TOPIC_BUILTIN_TOPIC_TRAITS_HPP
#define CYCLONEDDS_TOPIC_BUILTIN_TOPIC_TRAITS_HPP

#include <dds/topic/BuiltinTopic.hpp>
#include <org/eclipse/cyclonedds/topic/TopicTraits.hpp>
#include <org/eclipse/cyclonedds/topic/BuiltinTopicCopy.hpp>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace topic
{

template <>
class TopicTraits<dds::topic::ParticipantBuiltinTopicData>
{
public:
  static bool isKeyless()
  {
    return true;
  }

  static const char *getTypeName()
  {
    return "dds::topic::ParticipantBuiltinTopicData";
  }

  static ddsi_sertype *getSerType()
  {
    auto *st = new ddscxx_sertype<dds::topic::ParticipantBuiltinTopicData>();
    return static_cast<ddsi_sertype*>(st);
  }

  static size_t getSampleSize()
  {
    return sizeof(dds::topic::ParticipantBuiltinTopicData);
  }

  static bool isSelfContained()
  {
    return false;
  }
};

template <>
class TopicTraits<dds::topic::TopicBuiltinTopicData>
{
public:
  static bool isKeyless()
  {
    return true;
  }

  static const char *getTypeName()
  {
    return "dds::topic::TopicBuiltinTopicData";
  }

  static ddsi_sertype *getSerType()
  {
    auto *st = new ddscxx_sertype<dds::topic::TopicBuiltinTopicData>();
    return static_cast<ddsi_sertype*>(st);
  }

  static size_t getSampleSize()
  {
    return sizeof(dds::topic::TopicBuiltinTopicData);
  }

  static bool isSelfContained()
  {
    return false;
  }
};

template <>
class TopicTraits<dds::topic::PublicationBuiltinTopicData>
{
public:
  static bool isKeyless()
  {
    return true;
  }

  static const char *getTypeName()
  {
    return "dds::topic::PublicationBuiltinTopicData";
  }

  static ddsi_sertype *getSerType()
  {
    auto *st = new ddscxx_sertype<dds::topic::PublicationBuiltinTopicData>();
    return static_cast<ddsi_sertype*>(st);
  }

  static size_t getSampleSize()
  {
    return sizeof(dds::topic::PublicationBuiltinTopicData);
  }

  static bool isSelfContained()
  {
    return false;
  }
};

template <>
class TopicTraits<dds::topic::SubscriptionBuiltinTopicData>
{
public:
  static bool isKeyless()
  {
    return true;
  }

  static const char *getTypeName()
  {
    return "dds::topic::SubscriptionBuiltinTopicData";
  }

  static ddsi_sertype *getSerType()
  {
    auto *st = new ddscxx_sertype<dds::topic::SubscriptionBuiltinTopicData>();
    return static_cast<ddsi_sertype*>(st);
  }

  static size_t getSampleSize()
  {
    return sizeof(dds::topic::SubscriptionBuiltinTopicData);
  }

  static bool isSelfContained()
  {
    return false;
  }
};

}  /* topic */
}  /* cyclonedds */
}  /* eclipse */
}  /* org */

#endif /* CYCLONEDDS_TOPIC_BUILTIN_TOPIC_TRAITS_HPP */
