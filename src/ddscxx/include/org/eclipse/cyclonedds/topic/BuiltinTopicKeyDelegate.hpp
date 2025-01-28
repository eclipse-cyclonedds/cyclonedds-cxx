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

#ifndef CYCLONEDDS_TOPIC_BUILTIN_TOPIC_KEY_DELEGATE_HPP_
#define CYCLONEDDS_TOPIC_BUILTIN_TOPIC_KEY_DELEGATE_HPP_

#include <iomanip>

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
    typedef uint8_t VALUE_T;
public:
    BuiltinTopicKeyDelegate() { }
    BuiltinTopicKeyDelegate(uint8_t v[16])
    {
        std::copy(v, v + 16, key_.begin());
    }
public:
    const uint8_t* value() const
    {
        return &key_[0];
    }

    void value(uint8_t v[16])
    {
        std::copy(v, v + 16, key_.begin());
    }

    bool operator ==(const BuiltinTopicKeyDelegate& other) const
    {
        return other.key_ == key_;
    }

    friend std::ostream& operator<<(std::ostream& os, const BuiltinTopicKeyDelegate& key)
    {
        for (size_t i = 0; i < key.key_.size(); ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
            {
                os << '-';
            }
            os << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(key.key_[i]);
        }
        return os;
    }

private:
    std::array<uint8_t, 16> key_;
};

}
}
}
}

#endif /* CYCLONEDDS_TOPIC_BUILTIN_TOPIC_KEY_DELEGATE_HPP_ */
