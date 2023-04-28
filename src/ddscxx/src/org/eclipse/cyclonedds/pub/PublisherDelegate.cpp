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

#include <dds/pub/DataWriter.hpp>
#include <dds/pub/AnyDataWriter.hpp>
#include <dds/pub/Publisher.hpp>
#include <dds/pub/PublisherListener.hpp>

#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/pub/PublisherDelegate.hpp>
#include <org/eclipse/cyclonedds/pub/AnyDataWriterDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>


namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace pub
{

PublisherDelegate::PublisherDelegate(const dds::domain::DomainParticipant& dp,
                                     const dds::pub::qos::PublisherQos& qos,
                                     dds::pub::PublisherListener* listener,
                                     const dds::core::status::StatusMask& event_mask)
    :   dp_(dp),
        qos_(qos),
        default_dwqos_()
{
    dds_entity_t ddsc_par;
    dds_entity_t ddsc_pub;
    dds_qos_t* ddsc_qos;

    ddsc_par = static_cast<dds_entity_t>(this->dp_.delegate()->get_ddsc_entity());
    if (!ddsc_par) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "Could not get publisher participant.");
    }

    qos.delegate().check();
    ddsc_qos = qos.delegate().ddsc_qos();
    if (!ddsc_qos) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "Could not convert publisher QoS.");
    }

    this->listener(listener, event_mask);
    ddsc_pub = dds_create_publisher(ddsc_par, ddsc_qos, this->listener_callbacks);
    dds_delete_qos(ddsc_qos);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_pub, "Could not create publisher.");
    this->set_ddsc_entity(ddsc_pub);
}

PublisherDelegate::~PublisherDelegate()
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
PublisherDelegate::init(ObjectDelegate::weak_ref_type weak_ref)
{
    /* Set weak_ref before passing ourselves to other isocpp objects. */
    this->set_weak_ref(weak_ref);
    /* Add weak_ref to the map of entities */
    this->add_to_entity_map(weak_ref);
    /* Register Publisher at Participant. */
    this->dp_.delegate()->add_publisher(*this);

    /* Enable when needed. */
    if (this->dp_.delegate()->is_auto_enable()) {
        this->enable();
    }
}

void
PublisherDelegate::close()
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    /* Close the datawriters. */
    this->writers.all_close();

    /* Stop listener. */
    this->listener_set(NULL, dds::core::status::StatusMask::none());

    /* Unregister Publisher from Participant. */
    this->dp_.delegate()->remove_publisher(*this);

    org::eclipse::cyclonedds::core::EntityDelegate::close();
}

const dds::pub::qos::PublisherQos&
PublisherDelegate::qos() const
{
    this->check();
    return this->qos_;
}

void
PublisherDelegate::qos(const dds::pub::qos::PublisherQos& pqos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds_qos_t* ddsc_qos;
    dds_return_t ret;

    pqos.delegate().check();
    ddsc_qos = pqos.delegate().ddsc_qos();
    if (!ddsc_qos) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "Could not convert publisher qos.");
    }

    ret = dds_set_qos(ddsc_entity, ddsc_qos);
    dds_free(ddsc_qos);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Could not set publisher qos.");

    this->qos_ = pqos;
}

dds::pub::qos::DataWriterQos
PublisherDelegate::default_datawriter_qos() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dds::pub::qos::DataWriterQos qos = this->default_dwqos_;
    return qos;
}

void
PublisherDelegate::default_datawriter_qos(const dds::pub::qos::DataWriterQos& dwqos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    dwqos.delegate().check();
    this->default_dwqos_ = dwqos;
}

void
PublisherDelegate::suspend_publications()
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

void
PublisherDelegate::resume_publications()
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

void
PublisherDelegate::begin_coherent_changes()
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

void
PublisherDelegate::end_coherent_changes()
{
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

void
PublisherDelegate::wait_for_acknowledgments(const dds::core::Duration& max_wait)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    (void)max_wait;
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

void
PublisherDelegate::listener(dds::pub::PublisherListener* listener,
                            const ::dds::core::status::StatusMask& mask)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->listener_set(listener, mask);
    scopedLock.unlock();
}

dds::pub::PublisherListener*
PublisherDelegate::listener() const
{
    this->check();
    return reinterpret_cast<dds::pub::PublisherListener*>(this->listener_get());
}

const dds::domain::DomainParticipant&
PublisherDelegate::participant() const
{
    this->check();
    return dp_;
}

bool
PublisherDelegate::contains_entity(
    const dds::core::InstanceHandle& handle)
{
    return this->writers.contains(handle);
}

void
PublisherDelegate::add_datawriter(
    org::eclipse::cyclonedds::core::EntityDelegate& datawriter)
{
    this->writers.insert(datawriter);
}

void
PublisherDelegate::remove_datawriter(
    org::eclipse::cyclonedds::core::EntityDelegate& datawriter)
{
    this->writers.erase(datawriter);
}

org::eclipse::cyclonedds::pub::AnyDataWriterDelegate::ref_type
PublisherDelegate::find_datawriter(const std::string& topic_name)
{
    org::eclipse::cyclonedds::pub::AnyDataWriterDelegate::ref_type writer;
    org::eclipse::cyclonedds::core::EntitySet::vector vwriters;
    org::eclipse::cyclonedds::core::EntitySet::vectorIterator iter;

    vwriters = this->writers.copy();
    for (iter = vwriters.begin(); (!writer) && (iter != vwriters.end()); ++iter) {
        org::eclipse::cyclonedds::core::ObjectDelegate::ref_type ref = iter->lock();
        if (ref) {
            org::eclipse::cyclonedds::pub::AnyDataWriterDelegate::ref_type tmp =
                    ::std::dynamic_pointer_cast<AnyDataWriterDelegate>(ref);
            assert(tmp);
            if (tmp->topic_description().name() == topic_name) {
                writer = tmp;
            }
        }
    }

    return writer;
}

dds::pub::TPublisher<PublisherDelegate>
PublisherDelegate::wrapper()
{
    PublisherDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<PublisherDelegate>(this->get_strong_ref());
    dds::pub::Publisher pub(ref);
    return pub;
}

bool
PublisherDelegate::is_auto_enable() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    bool autoEnable = this->qos_.delegate().policy<dds::core::policy::EntityFactory>().delegate().auto_enable();

    return autoEnable;
}


void PublisherDelegate::on_offered_deadline_missed(dds_entity_t writer,
        org::eclipse::cyclonedds::core::OfferedDeadlineMissedStatusDelegate &sd)
{
    org::eclipse::cyclonedds::pub::AnyDataWriterDelegate::ref_type ref =
           ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::pub::AnyDataWriterDelegate>(
                this->extract_strong_ref(writer));
    dds::pub::AnyDataWriter adw(ref);

    dds::core::status::OfferedDeadlineMissedStatus s;
    s.delegate() = sd;

    this->listener()->on_offered_deadline_missed(adw, s);
}

void PublisherDelegate::on_offered_incompatible_qos(dds_entity_t writer,
        org::eclipse::cyclonedds::core::OfferedIncompatibleQosStatusDelegate &sd)
{
    org::eclipse::cyclonedds::pub::AnyDataWriterDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::pub::AnyDataWriterDelegate>(
                this->extract_strong_ref(writer));
    dds::pub::AnyDataWriter adw(ref);

    dds::core::status::OfferedIncompatibleQosStatus s;
    s.delegate() = sd;

    this->listener()->on_offered_incompatible_qos(adw, s);
}

void PublisherDelegate::on_liveliness_lost(dds_entity_t writer,
        org::eclipse::cyclonedds::core::LivelinessLostStatusDelegate &sd)
{
    org::eclipse::cyclonedds::pub::AnyDataWriterDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::pub::AnyDataWriterDelegate>(
                this->extract_strong_ref(writer));
    dds::pub::AnyDataWriter adw(ref);

    dds::core::status::LivelinessLostStatus s;
    s.delegate() = sd;

    this->listener()->on_liveliness_lost(adw, s);
}

void PublisherDelegate::on_publication_matched(dds_entity_t writer,
        org::eclipse::cyclonedds::core::PublicationMatchedStatusDelegate &sd)
{
    org::eclipse::cyclonedds::pub::AnyDataWriterDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::pub::AnyDataWriterDelegate>(
                this->extract_strong_ref(writer));
    dds::pub::AnyDataWriter adw(ref);

    dds::core::status::PublicationMatchedStatus s;
    s.delegate() = sd;

    this->listener()->on_publication_matched(adw, s);
}



}
}
}
}
