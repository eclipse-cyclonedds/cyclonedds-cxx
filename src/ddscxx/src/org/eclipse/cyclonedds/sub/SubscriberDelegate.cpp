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
#include <dds/sub/AnyDataReader.hpp>
#include <dds/sub/Subscriber.hpp>
#include <dds/sub/SubscriberListener.hpp>
#include <dds/domain/DomainParticipantListener.hpp>

#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/sub/SubscriberDelegate.hpp>
#include <org/eclipse/cyclonedds/sub/AnyDataReaderDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace sub
{

SubscriberDelegate::SubscriberDelegate(
    const dds::domain::DomainParticipant& dp,
    const dds::sub::qos::SubscriberQos& qos,
    dds::sub::SubscriberListener* listener,
    const dds::core::status::StatusMask& event_mask) :
    dp_(dp),
    qos_(qos)
{
    dds_entity_t ddsc_par;
    dds_entity_t ddsc_sub;
    dds_qos_t* ddsc_qos;

    ddsc_par = static_cast<dds_entity_t>(this->dp_.delegate()->get_ddsc_entity());
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_par, "Could not get subscriber participant.");

    qos.delegate().check();
    ddsc_qos = qos.delegate().ddsc_qos();

    if (!ddsc_qos) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "Could not convert subscriber QoS.");
    }

    this->listener(listener, event_mask);
    ddsc_sub = dds_create_subscriber(ddsc_par, ddsc_qos, this->listener_callbacks);

    dds_delete_qos(ddsc_qos);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_sub, "Could not create subscriber.");
    this->set_ddsc_entity(ddsc_sub);
}

SubscriberDelegate::~SubscriberDelegate()
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
SubscriberDelegate::init(ObjectDelegate::weak_ref_type weak_ref)
{
    /* Set weak_ref before passing ourselves to other isocpp objects. */
    this->set_weak_ref(weak_ref);
    /* Add weak_ref to the map of entities */
    this->add_to_entity_map(weak_ref);
    /* Register Publisher at Participant. */
    this->dp_.delegate()->add_subscriber(*this);

    /* Enable when needed. */
    if (this->dp_.delegate()->is_auto_enable()) {
        this->enable();
    }
}

void
SubscriberDelegate::close()
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    /* Close the datareaders. */
    this->readers.all_close();

    /* Stop listener. */
    this->listener_set(NULL, dds::core::status::StatusMask::none());

    /* Unregister Subscriber from Participant. */
    this->dp_.delegate()->remove_subscriber(*this);

    org::eclipse::cyclonedds::core::EntityDelegate::close();
}

const dds::sub::qos::SubscriberQos&
SubscriberDelegate::qos() const
{
    this->check();
    return this->qos_;
}

void
SubscriberDelegate::qos(const dds::sub::qos::SubscriberQos& sqos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds_qos_t* ddsc_qos;
    dds_return_t ret;

    sqos.delegate().check();
    ddsc_qos = sqos.delegate().ddsc_qos();
    if (!ddsc_qos) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "Could not convert subscriber qos.");
    }

    ret = dds_set_qos(ddsc_entity, ddsc_qos);
    dds_delete_qos(ddsc_qos);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Could not set subscriber qos.");

    this->qos_ = sqos;
}

dds::sub::qos::DataReaderQos
SubscriberDelegate::default_datareader_qos() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds::sub::qos::DataReaderQos qos = this->default_dr_qos_;
    return qos;
}

void
SubscriberDelegate::default_datareader_qos(const dds::sub::qos::DataReaderQos& drqos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    drqos.delegate().check();
    this->default_dr_qos_ = drqos;
}

void
SubscriberDelegate::begin_coherent_access()
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

void
SubscriberDelegate::end_coherent_access()
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

const dds::domain::DomainParticipant&
SubscriberDelegate::participant() const
{
    this->check();
    return dp_;
}

void
SubscriberDelegate::listener(dds::sub::SubscriberListener* listener,
                            const ::dds::core::status::StatusMask& mask)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->listener_set(listener, mask);
    scopedLock.unlock();
}

dds::sub::SubscriberListener*
SubscriberDelegate::listener() const
{
    this->check();
    return reinterpret_cast<dds::sub::SubscriberListener*>(this->listener_get());
}

bool
SubscriberDelegate::contains_entity(
    const dds::core::InstanceHandle& handle)
{
    return this->readers.contains(handle);
}

void
SubscriberDelegate::add_datareader(
    org::eclipse::cyclonedds::core::EntityDelegate& datareader)
{
    this->readers.insert(datareader);
}

void
SubscriberDelegate::remove_datareader(
    org::eclipse::cyclonedds::core::EntityDelegate& datareader)
{
    this->readers.erase(datareader);
}

std::vector<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type>
SubscriberDelegate::find_datareaders(const std::string& topic_name)
{
    std::vector<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type> _readers;

    org::eclipse::cyclonedds::core::EntitySet::vector entities;
    org::eclipse::cyclonedds::core::EntitySet::vectorIterator iter;

    entities = this->readers.copy();
    _readers.reserve(entities.size());
    for (iter = entities.begin(); iter != entities.end(); ++iter) {
        org::eclipse::cyclonedds::core::ObjectDelegate::ref_type ref = iter->lock();
        if (ref) {
            org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type tmp =
                    ::std::dynamic_pointer_cast<AnyDataReaderDelegate>(ref);
            assert(tmp);
            if (tmp->topic_description().name() == topic_name) {
                _readers.push_back(tmp);
            }
        }
    }

    return _readers;
}

std::vector<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type>
SubscriberDelegate::get_datareaders(
    const dds::sub::status::DataState& mask)
{
    (void)mask;
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    std::vector<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type> _readers;
    return _readers;
}

void
SubscriberDelegate::notify_datareaders()
{

}

dds::sub::TSubscriber<SubscriberDelegate>
SubscriberDelegate::wrapper()
{
    SubscriberDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<SubscriberDelegate>(this->get_strong_ref());
    dds::sub::Subscriber sub(ref);
    return sub;
}

bool
SubscriberDelegate::is_auto_enable() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    bool autoEnable = this->qos_.delegate().policy<dds::core::policy::EntityFactory>().delegate().auto_enable();

    return autoEnable;
}


void
SubscriberDelegate::reset_data_on_readers()
{
}

// Subscriber events
void SubscriberDelegate::on_data_readers(dds_entity_t subscriber)
{
    dds::sub::Subscriber s = wrapper();
    (void)subscriber;
    this->listener()->on_data_on_readers(s);
}

// Reader events
void SubscriberDelegate::on_requested_deadline_missed(dds_entity_t reader,
        org::eclipse::cyclonedds::core::RequestedDeadlineMissedStatusDelegate &sd)
{
    org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate>(
                this->extract_strong_ref(reader));
    dds::sub::AnyDataReader adr(ref);

    dds::core::status::RequestedDeadlineMissedStatus s;
    s.delegate() = sd;

    this->listener()->on_requested_deadline_missed(adr, s);
}

void SubscriberDelegate::on_requested_incompatible_qos(dds_entity_t reader,
        org::eclipse::cyclonedds::core::RequestedIncompatibleQosStatusDelegate &sd)
{
    org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate>(
                this->extract_strong_ref(reader));
    dds::sub::AnyDataReader adr(ref);

    dds::core::status::RequestedIncompatibleQosStatus s;
    s.delegate() = sd;

    this->listener()->on_requested_incompatible_qos(adr, s);
}

void SubscriberDelegate::on_sample_rejected(dds_entity_t reader,
        org::eclipse::cyclonedds::core::SampleRejectedStatusDelegate &sd)
{
    org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate>(
                this->extract_strong_ref(reader));
    dds::sub::AnyDataReader adr(ref);

    dds::core::status::SampleRejectedStatus s;
    s.delegate() = sd;

    this->listener()->on_sample_rejected(adr, s);
}

void SubscriberDelegate::on_liveliness_changed(dds_entity_t reader,
        org::eclipse::cyclonedds::core::LivelinessChangedStatusDelegate &sd)
{
    org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate>(
                this->extract_strong_ref(reader));
    dds::sub::AnyDataReader adr(ref);

    dds::core::status::LivelinessChangedStatus s;
    s.delegate() = sd;

    this->listener()->on_liveliness_changed(adr, s);
}

void SubscriberDelegate::on_data_available(dds_entity_t reader)
{
    org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate>(
                this->extract_strong_ref(reader));
    dds::sub::AnyDataReader adr(ref);

    this->listener()->on_data_available(adr);
}

void SubscriberDelegate::on_subscription_matched(dds_entity_t reader,
        org::eclipse::cyclonedds::core::SubscriptionMatchedStatusDelegate &sd)
{
    org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate>(
                this->extract_strong_ref(reader));
    dds::sub::AnyDataReader adr(ref);

    dds::core::status::SubscriptionMatchedStatus s;
    s.delegate() = sd;

    this->listener()->on_subscription_matched(adr, s);
}

void SubscriberDelegate::on_sample_lost(dds_entity_t reader,
        org::eclipse::cyclonedds::core::SampleLostStatusDelegate &sd)
{
    org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate>(
                this->extract_strong_ref(reader));
    dds::sub::AnyDataReader adr(ref);

    dds::core::status::SampleLostStatus s;
    s.delegate() = sd;

    this->listener()->on_sample_lost(adr, s);
}







}
}
}
}
