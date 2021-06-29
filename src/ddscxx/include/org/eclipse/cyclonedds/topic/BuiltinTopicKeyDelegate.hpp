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

#ifndef CYCLONEDDS_TOPIC_BUILTIN_TOPIC_KEY_DELEGATE_HPP_
#define CYCLONEDDS_TOPIC_BUILTIN_TOPIC_KEY_DELEGATE_HPP_

#include <array>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace topic
{

class BuiltinTopicKeyDelegate
{
public:
    BuiltinTopicKeyDelegate() { }
    BuiltinTopicKeyDelegate(const uint8_t *v)
    {
        memcpy(key_.data(), v, key_.size());
    }
public:
    const uint8_t* value() const
    {
        return key_.data();
    }

    void value(const uint8_t *v)
    {
        memcpy(key_.data(), v, key_.size());
    }

    bool operator ==(const BuiltinTopicKeyDelegate& other) const
    {
        return other.key_ == key_;
    }

private:
    std::array<uint8_t, 16> key_;
};

}
}
}
}

#endif /* CYCLONEDDS_TOPIC_BUILTIN_TOPIC_KEY_DELEGATE_HPP_ */
