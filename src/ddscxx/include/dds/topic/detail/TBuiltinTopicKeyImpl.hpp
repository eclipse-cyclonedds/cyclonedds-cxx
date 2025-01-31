// Copyright(c) 2006 to 2020 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 
#ifndef CYCLONEDDS_DDS_TOPIC_DETAIL_TBUILTINTOPICKEY_IMPL_HPP_
#define CYCLONEDDS_DDS_TOPIC_DETAIL_TBUILTINTOPICKEY_IMPL_HPP_

/**
 * @file
 */

/*
 * OMG PSM class declaration
 */
#include <dds/topic/TBuiltinTopicKey.hpp>

// Implementation

namespace dds
{
namespace topic
{

template <typename D>
const std::array<uint8_t, 16>& TBuiltinTopicKey<D>::value() const
{
    return this->delegate().value();
}

template <typename D>
void TBuiltinTopicKey<D>::value(const std::array<uint8_t, 16>& key)
{
    return this->delegate().value(key);
}

}
}

inline std::ostream& operator << (std::ostream& os, const dds::topic::TBuiltinTopicKey<org::eclipse::cyclonedds::topic::BuiltinTopicKeyDelegate>& h)
{
     os << h.delegate();
     return os;
}

// End of implementation

#endif /* CYCLONEDDS_DDS_TOPIC_DETAIL_TBUILTINTOPICKEY_IMPL_HPP_ */
