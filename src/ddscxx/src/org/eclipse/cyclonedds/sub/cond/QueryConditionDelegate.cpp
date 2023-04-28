// Copyright(c) 2006 to 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <org/eclipse/cyclonedds/sub/cond/QueryConditionDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>

org::eclipse::cyclonedds::sub::cond::QueryConditionDelegate::QueryConditionDelegate(
    const dds::sub::AnyDataReader& dr,
    const std::string& expression,
    const dds::sub::status::DataState& data_state) :
        QueryDelegate(dr, expression, data_state),
        ReadConditionDelegate(dr, data_state)
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "QueryCondition not currently supported");
}

org::eclipse::cyclonedds::sub::cond::QueryConditionDelegate::QueryConditionDelegate(
    const dds::sub::AnyDataReader& dr,
    const std::string& expression,
    const std::vector<std::string>& params,
    const dds::sub::status::DataState& data_state) :
        QueryDelegate(dr, expression, params, data_state),
        ReadConditionDelegate(dr, data_state)
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "QueryCondition not currently supported");
}

/* The close() operation of Condition will try to remove this Condition from
 * its WaitSets. However, since the WaitSets hold a reference to their Conditions,
 * the destructor can never be invoked for Conditions that are still attached
 * to WaitSets.
 * For that reason we know that if the destructor is invoked, the Condition
 * can no longer be attached to a WaitSet, so we can skip the local close()
 * and immediately proceed the the close() of the UserObjectDelegate parent.
 * If we would try to invoke Condition::close() here, then we would run
 * into a deadlock when we claim the WaitSet lock in case this destructor
 * is invoked by the destructor of the WaitSet, which has the WaitSet already
 * locked before.
 */
org::eclipse::cyclonedds::sub::cond::QueryConditionDelegate::~QueryConditionDelegate()
{
}

void
org::eclipse::cyclonedds::sub::cond::QueryConditionDelegate::init(
        ObjectDelegate::weak_ref_type weak_ref)
{
    ReadConditionDelegate::init(weak_ref);
}
