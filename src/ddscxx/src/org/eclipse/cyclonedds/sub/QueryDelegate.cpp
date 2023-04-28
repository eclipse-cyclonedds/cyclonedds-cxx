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
#include <dds/sub/DataReader.hpp>
#include <dds/sub/Subscriber.hpp>
#include <dds/topic/TopicInstance.hpp>
#include <dds/topic/Topic.hpp>

#include <org/eclipse/cyclonedds/sub/QueryDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>

org::eclipse::cyclonedds::sub::QueryDelegate::QueryDelegate(
    const dds::sub::AnyDataReader& dr,
    const dds::sub::status::DataState& state_filter) :
        reader_(dr), expression_("1=1"),
        state_filter_(state_filter), modified_(true)
{
    ISOCPP_BOOL_CHECK_AND_THROW((dr != dds::core::null),
                                ISOCPP_NULL_REFERENCE_ERROR,
                                "No reader provided.");
}

org::eclipse::cyclonedds::sub::QueryDelegate::QueryDelegate(
    const dds::sub::AnyDataReader& dr,
    const std::string& expression,
    const dds::sub::status::DataState& state_filter) :
        reader_(dr), expression_(expression),
        state_filter_(state_filter), modified_(true)
{
    ISOCPP_BOOL_CHECK_AND_THROW((dr != dds::core::null),
                                ISOCPP_NULL_REFERENCE_ERROR,
                                "No reader provided.");
}

org::eclipse::cyclonedds::sub::QueryDelegate::QueryDelegate(
    const dds::sub::AnyDataReader& dr,
    const std::string& expression,
    const std::vector<std::string>& params,
    const dds::sub::status::DataState& state_filter) :
         reader_(dr), expression_(expression),
         params_(params), state_filter_(state_filter), modified_(true)
{
    ISOCPP_BOOL_CHECK_AND_THROW((dr != dds::core::null),
                                ISOCPP_NULL_REFERENCE_ERROR,
                                "No reader provided.");
}

org::eclipse::cyclonedds::sub::QueryDelegate::~QueryDelegate()
{
    if (!this->closed) {
        try {
            this->close();
        } catch (...) {
            /* Empty: the exception throw should have already traced an error. */
        }
    }
}

void
org::eclipse::cyclonedds::sub::QueryDelegate::init(
    ObjectDelegate::weak_ref_type weak_ref)
{
    this->set_weak_ref(weak_ref);
    /* Add weak_ref to the map of entities */
    this->add_to_entity_map(weak_ref);
    (this->reader_)->add_query(*this);
}

/* The QueryContainer has a close and a deinit method.
 * When the QueryContainer is created as result of the
 * use of a Query with an DataReader::Selector then the
 * QueryContainer will be responsible for closing the
 * corresponding dds_entity_t handle which will be handled by
 * the close method.
 * The ReadConditionDelegate inherits both
 * from QueryContainer and from ConditionDelegate.
 * When the QueryContainer is created as result of being
 * the parent of a ReadConditionDelegate then the close of the
 * ReadConditionDelegate will call the deinit method of QueryContainer
 * which will remove the QueryContainer from the associated
 * DataReaderDelegate. Then the close of the ReadConditionDelegate
 * will call close on the ConditionDelegate to close the corresponding
 * dds_entity_t handle.
 */
void
org::eclipse::cyclonedds::sub::QueryDelegate::deinit()
{
    (this->reader_)->remove_query(*this);
}

void
org::eclipse::cyclonedds::sub::QueryDelegate::close()
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    deinit();

    DDScObjectDelegate::close();
}

const std::string&
org::eclipse::cyclonedds::sub::QueryDelegate::expression() const
{
    return expression_;
}

void
org::eclipse::cyclonedds::sub::QueryDelegate::expression(
    const std::string& expr)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    if (this->expression_ != expr) {
        this->expression_ = expr;
        this->modified_ = true;
    }
}

org::eclipse::cyclonedds::sub::QueryDelegate::iterator
org::eclipse::cyclonedds::sub::QueryDelegate::begin()
{
    return params_.begin();
}

org::eclipse::cyclonedds::sub::QueryDelegate::iterator
org::eclipse::cyclonedds::sub::QueryDelegate::end()
{
    return params_.end();
}

org::eclipse::cyclonedds::sub::QueryDelegate::const_iterator
org::eclipse::cyclonedds::sub::QueryDelegate::begin() const
{
    return params_.begin();
}

org::eclipse::cyclonedds::sub::QueryDelegate::const_iterator
org::eclipse::cyclonedds::sub::QueryDelegate::end() const
{
    return params_.end();
}

void
org::eclipse::cyclonedds::sub::QueryDelegate::add_parameter(
    const std::string& param)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    params_.push_back(param);
}

uint32_t
org::eclipse::cyclonedds::sub::QueryDelegate::parameters_length() const
{
    this->lock();
    uint32_t len =  static_cast<uint32_t>(params_.size());
    this->unlock();

    return len;
}

void
org::eclipse::cyclonedds::sub::QueryDelegate::parameters(const std::vector<std::string>& params)
{
    /* TODO */
    (void)params;
}

std::vector<std::string>
org::eclipse::cyclonedds::sub::QueryDelegate::parameters()
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    return this->params_;
}


void
org::eclipse::cyclonedds::sub::QueryDelegate::clear_parameters()
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    if (!this->params_.empty()) {
        this->params_.erase(this->params_.begin(), this->params_.end());
    }
}


const dds::sub::AnyDataReader&
org::eclipse::cyclonedds::sub::QueryDelegate::data_reader() const
{
    return this->reader_;
}

void
org::eclipse::cyclonedds::sub::QueryDelegate::state_filter(
    dds::sub::status::DataState& s)
{
    this->lock();
    if (this->state_filter_ != s) {
        this->state_filter_ = s;
        this->modified_ = true;
    }
    this->unlock();
}

dds::sub::status::DataState
org::eclipse::cyclonedds::sub::QueryDelegate::state_filter()
{
    this->lock();
    dds::sub::status::DataState filter = this->state_filter_;
    this->unlock();

    return filter;
}

bool
org::eclipse::cyclonedds::sub::QueryDelegate::state_filter_equal(
    dds::sub::status::DataState& s)
{
    bool equal = false;
    this->lock();
    equal = this->state_filter_ == s;
    this->unlock();

    return equal;
}

bool
org::eclipse::cyclonedds::sub::QueryDelegate::modify_state_filter(
    dds::sub::status::DataState& s)
{
    this->state_filter(s);
    return true;
}
