// Copyright(c) 2006 to 2021 ZettaScale Technology and others
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

#include <org/eclipse/cyclonedds/sub/SampleInfoImpl.hpp>
#include <org/eclipse/cyclonedds/core/MiscUtils.hpp>
#include <dds/sub/status/detail/DataStateImpl.hpp>

//org::eclipse::cyclonedds::sub::SampleInfoImpl::SampleInfoImpl() : valid_(false) { }

org::eclipse::cyclonedds::sub::SampleInfoImpl::SampleInfoImpl(const dds_sample_info_t *from) :
    source_timestamp_(org::eclipse::cyclonedds::core::convertTime(from->source_timestamp)),
    state_(dds::sub::status::DataState(
        from->sample_state == DDS_SST_READ ? dds::sub::status::SampleState::read() : dds::sub::status::SampleState::not_read(),
        from->view_state == DDS_VST_NEW ? dds::sub::status::ViewState::new_view() : dds::sub::status::ViewState::not_new_view(),
        from->instance_state == DDS_IST_ALIVE ? dds::sub::status::InstanceState::alive() :
            (from->instance_state == DDS_IST_NOT_ALIVE_DISPOSED ? dds::sub::status::InstanceState::not_alive_disposed() : dds::sub::status::InstanceState::not_alive_no_writers()))),
    generation_count_(static_cast<int32_t>(from->disposed_generation_count), static_cast<int32_t>(from->no_writers_generation_count)),
    rank_(static_cast<int32_t>(from->sample_rank),static_cast<int32_t>(from->generation_rank),static_cast<int32_t>(from->absolute_generation_rank)),
    valid_(from->valid_data),
    instance_handle_(from->instance_handle),
    publication_handle_(from->publication_handle)
{ }

bool
org::eclipse::cyclonedds::sub::SampleInfoImpl::operator==(const SampleInfoImpl& other) const
{
    return this->source_timestamp_ == other.timestamp()
           && state_is_equal(this->state_, other.state())
           && this->generation_count_ == other.generation_count()
           && this->rank_ == other.rank()
           && this->valid_ == other.valid()
           && this->instance_handle_ == other.instance_handle()
           && this->publication_handle_ == other.publication_handle();
}

bool
org::eclipse::cyclonedds::sub::SampleInfoImpl::state_is_equal(const dds::sub::status::DataState& s1, const dds::sub::status::DataState& s2)
{
    return s1.instance_state() == s2.instance_state()
           && s1.view_state() == s2.view_state()
           && s1.sample_state() == s2.sample_state();
}
