/*
 * Copyright(c) 2006 to 2021 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include "dds/dds.hpp"
#include <gtest/gtest.h>
#include "EntityProperties.hpp"
#include "EntityProperties_pragma.hpp"

using namespace org::eclipse::cyclonedds::core::cdr;
using namespace Entity_testing;

/**
 * Fixture for the testing the generated entity properties
 */
class EntityTesting : public ::testing::Test
{
  public:
    void SetUp() { }

    void TearDown() { }
};

void compare_endpoints_with_entity(const key_endpoint &ke, const entity_properties &props)
{
  for (const auto &member:props.m_members_by_seq) {
    //do not look at list terminating members
    if (!member)
      continue;

    auto it = ke.find(member.m_id);
    ASSERT_EQ(member.is_key, it != ke.end());
    if (ke.end() != it)
      compare_endpoints_with_entity(it->second, member);
  }
}

template<typename T>
void test_props(const std::list<std::list<uint32_t> > &endpoints)
{
  auto props = get_type_props<T>();

  key_endpoint ke;
  for (const auto &endpoint:endpoints)
    ke.add_key_endpoint(endpoint);

  compare_endpoints_with_entity(ke, props);
}

TEST_F(EntityTesting, entity_properties_annotation)
{
  test_props<L1_u>({});
  test_props<L1_k>({{1}});

  test_props<L2_u_u>({});
  test_props<L2_k_u>({{1,0},{1,1}});
  test_props<L2_k_k>({{1,1}});

  test_props<L3_u_u_u>({});
  test_props<L3_k_u_u>({{1,0,0},{1,0,1},{1,1,0},{1,1,1}});
  test_props<L3_k_k_u>({{1,1,0},{1,1,1}});
  test_props<L3_k_k_k>({{1,1,1}});

}

TEST_F(EntityTesting, entity_properties_pragma)
{
  test_props<P1_u>({});
  test_props<P1_k>({{1}});

  test_props<P2_u_u>({});
  test_props<P2_k_u>({{1,0}});
  test_props<P2_k_k>({{0,1}});

  test_props<P3_u_u>({});
  test_props<P3_k_u>({{0,0,0},{0,1,0},{1,0,0},{1,1,0}});
  test_props<P3_k_k>({{0,0,1},{0,1,1},{1,0,1},{1,1,1}});
}
