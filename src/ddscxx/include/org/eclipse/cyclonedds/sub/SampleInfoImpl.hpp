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

#ifndef CYCLONEDDS_SUB_SAMPLE_INFO_IMPL_HPP_
#define CYCLONEDDS_SUB_SAMPLE_INFO_IMPL_HPP_

#include <org/eclipse/cyclonedds/core/config.hpp>
#include <dds/sub/Rank.hpp>
#include <dds/sub/GenerationCount.hpp>
#include <dds/core/Time.hpp>
#include <dds/sub/status/DataState.hpp>
#include <dds/core/InstanceHandle.hpp>

#include <dds/dds.h>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace sub
{
class SampleInfoImpl;
}
}
}
}


class org::eclipse::cyclonedds::sub::SampleInfoImpl
{
public:
    SampleInfoImpl() : valid_(false) { }

    SampleInfoImpl(const dds_sample_info_t *from);

    inline const dds::core::Time timestamp() const
    {
        return this->source_timestamp_;
    }

    inline void timestamp(const dds::core::Time& t)
    {
        this->source_timestamp_ = t;
    }

    inline const dds::sub::status::DataState state() const
    {
        return this->state_;
    }

    inline void state(const dds::sub::status::DataState& s)
    {
        this->state_ = s;
    }

    inline dds::sub::GenerationCount generation_count() const
    {
        return this->generation_count_;
    }

    inline void generation_count(dds::sub::GenerationCount& c)
    {
        this->generation_count_ = c;
    }

    inline dds::sub::Rank rank() const
    {
        return this->rank_;
    }

    inline void rank(dds::sub::Rank& r)
    {
        this->rank_ = r;
    }

    inline bool valid() const
    {
        return this->valid_;
    }

    inline void valid(bool v)
    {
        this->valid_ = v;
    }

    inline dds::core::InstanceHandle instance_handle() const
    {
        return this->instance_handle_;
    }

    inline void instance_handle(dds::core::InstanceHandle& h)
    {
        this->instance_handle_ = h;
    }

    inline dds::core::InstanceHandle publication_handle() const
    {
        return this->publication_handle_;
    }

    inline void publication_handle(dds::core::InstanceHandle& h)
    {
        this->publication_handle_ = h;
    }

    bool operator==(const SampleInfoImpl& other) const;


private:
    static bool state_is_equal(
                    const dds::sub::status::DataState& s1,
                    const dds::sub::status::DataState& s2);

private:
    dds::core::Time source_timestamp_;
    dds::sub::status::DataState state_;
    dds::sub::GenerationCount generation_count_;
    dds::sub::Rank rank_;
    bool valid_;
    dds::core::InstanceHandle instance_handle_;
    dds::core::InstanceHandle publication_handle_;

};

#endif /* CYCLONEDDS_SUB_SAMPLE_INFO_IMPL_HPP_ */
