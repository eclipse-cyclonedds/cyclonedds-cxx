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

#include <array>
#include <algorithm>
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
    typedef std::array<uint8_t, 16> VALUE_T;
public:
    BuiltinTopicKeyDelegate() { }
    BuiltinTopicKeyDelegate(const std::array<uint8_t, 16>& key)
    {
        key_ = key;
    }
public:
    const std::array<uint8_t, 16>& value() const
    {
        return key_;
    }

    void value(const std::array<uint8_t, 16>& key)
    {
        key_ = key;
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

    void set_ddsc_value(uint8_t v[16])
    {
        std::copy(v, v + 16, key_.begin());
    }

private:
    std::array<uint8_t, 16> key_;
};

}
}
}
}

#endif /* CYCLONEDDS_TOPIC_BUILTIN_TOPIC_KEY_DELEGATE_HPP_ */
