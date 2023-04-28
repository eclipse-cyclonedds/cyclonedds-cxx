// Copyright(c) 2006 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include "dds/dds.hpp"
#include <gtest/gtest.h>
#include "Space.hpp"

using org::eclipse::cyclonedds::topic::TopicTraits;
using TraitTest::StructDefault;
using TraitTest::StructNested;
using TraitTest::StructTopic;
using TraitTest::UnionDefault;
using TraitTest::UnionNested;
using TraitTest::UnionTopic;

template<class T>
void test_traits(unsigned int map_sz, unsigned int info_sz, bool map_blob_present, bool info_blob_present)
{
  EXPECT_EQ(TopicTraits<T>::type_map_blob_sz(), map_sz);
  EXPECT_EQ(TopicTraits<T>::type_info_blob_sz(), info_sz);
  if (map_blob_present)
    EXPECT_NE(TopicTraits<T>::type_map_blob(), nullptr);
  else
    EXPECT_EQ(TopicTraits<T>::type_map_blob(), nullptr);
  if (info_blob_present)
    EXPECT_NE(TopicTraits<T>::type_info_blob(), nullptr);
  else
    EXPECT_EQ(TopicTraits<T>::type_info_blob(), nullptr);
}

template<class T>
void test_typemap(bool typemap_present)
{
  auto *tm = TopicTraits<T>::getTypeMap(nullptr);

  if (typemap_present)
    EXPECT_NE(tm, nullptr);
  else
    EXPECT_EQ(tm, nullptr);

  if (tm) {
    ddsi_typemap_fini(tm);
    dds_free(tm);
  }
}

template<class T>
void test_typeinfo(bool typeinfo_present)
{
  auto *ti = TopicTraits<T>::getTypeInfo(nullptr);
  if (typeinfo_present)
    EXPECT_NE(ti, nullptr);
  else
    EXPECT_EQ(ti, nullptr);

  if (ti) {
    ddsi_typeinfo_fini(ti);
    dds_free(ti);
  }
}

template<class T, ddsi_typeid_kind_t kind>
void test_typeid(bool typeid_present)
{
  auto *ti = TopicTraits<T>::getTypeId(nullptr, kind);
  if (typeid_present)
    EXPECT_NE(ti, nullptr);
  else
    EXPECT_EQ(ti, nullptr);

  if (ti) {
    ddsi_typeid_fini(ti);
    dds_free(ti);
  }
}

template<class T>
void test_typeids(bool minimal_present,
                  bool complete_present,
                  bool fully_descriptive_present)
{
  test_typeid<T, DDSI_TYPEID_KIND_MINIMAL>(minimal_present);
  test_typeid<T, DDSI_TYPEID_KIND_COMPLETE>(complete_present);
  test_typeid<T, DDSI_TYPEID_KIND_FULLY_DESCRIPTIVE>(fully_descriptive_present);
}

TEST(TopicTypeDiscovery, blobs)
{
  test_traits<StructDefault>(202,100,true,true);
  test_traits<StructNested>(static_cast<unsigned int>(-1),static_cast<unsigned int>(-1),false,false);
  test_traits<StructTopic>(198,100,true,true);

  test_traits<UnionDefault>(282,100,true,true);
  test_traits<UnionNested>(static_cast<unsigned int>(-1),static_cast<unsigned int>(-1),false,false);
  test_traits<UnionTopic>(282,100,true,true);
}

TEST(TopicTypeDiscovery, typemap)
{
  test_typemap<StructDefault>(true);
  test_typemap<StructNested>(false);
  test_typemap<StructTopic>(true);

  test_typemap<UnionDefault>(true);
  test_typemap<UnionNested>(false);
  test_typemap<UnionTopic>(true);
}

TEST(TopicTypeDiscovery, typeinfo)
{
  test_typeinfo<StructDefault>(true);
  test_typeinfo<StructNested>(false);
  test_typeinfo<StructTopic>(true);

  test_typeinfo<UnionDefault>(true);
  test_typeinfo<UnionNested>(false);
  test_typeinfo<UnionTopic>(true);
}

TEST(TopicTypeDiscovery, typeID)
{
  test_typeids<StructDefault>(true, true, true);
  test_typeids<StructNested>(false, false, false);
  test_typeids<StructTopic>(true, true, true);

  test_typeids<UnionDefault>(true, true, true);
  test_typeids<UnionNested>(false, false, false);
  test_typeids<UnionTopic>(true, true, true);
}
