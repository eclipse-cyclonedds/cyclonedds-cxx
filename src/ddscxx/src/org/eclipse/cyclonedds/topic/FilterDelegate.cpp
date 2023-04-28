// Copyright(c) 2006 to 2020 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

/**
 * @file
 */

#include <org/eclipse/cyclonedds/topic/FilterDelegate.hpp>


org::eclipse::cyclonedds::topic::FilterDelegate::FilterDelegate() { }
org::eclipse::cyclonedds::topic::FilterDelegate::FilterDelegate(const std::string& query_expression)
    : myExpression(query_expression) { }

const std::string&
org::eclipse::cyclonedds::topic::FilterDelegate::expression() const
{
    return myExpression;
}

org::eclipse::cyclonedds::topic::FilterDelegate::const_iterator
org::eclipse::cyclonedds::topic::FilterDelegate::begin() const
{
    return myParams.begin();
}

org::eclipse::cyclonedds::topic::FilterDelegate::const_iterator
org::eclipse::cyclonedds::topic::FilterDelegate::end() const
{
    return myParams.end();
}

org::eclipse::cyclonedds::topic::FilterDelegate::iterator
org::eclipse::cyclonedds::topic::FilterDelegate::begin()
{
    return myParams.begin();
}

org::eclipse::cyclonedds::topic::FilterDelegate::iterator
org::eclipse::cyclonedds::topic::FilterDelegate::end()
{
    return myParams.end();
}

void
org::eclipse::cyclonedds::topic::FilterDelegate::add_parameter(const std::string& param)
{
    myParams.push_back(param);
}

uint32_t
org::eclipse::cyclonedds::topic::FilterDelegate::parameters_length() const
{
    return static_cast<uint32_t>(myParams.size());
}

bool
org::eclipse::cyclonedds::topic::FilterDelegate::operator ==(const FilterDelegate& other) const
{
    return other.myExpression == myExpression && other.myParams == myParams;
}

// End of implementation
