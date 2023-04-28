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

#include <dds/pub/DataWriter.hpp>
#include <dds/pub/Publisher.hpp>
#include <dds/sub/DataReader.hpp>
#include <dds/sub/AnyDataReader.hpp>
#include <dds/sub/Subscriber.hpp>
#include <dds/domain/DomainParticipant.hpp>
#include <dds/domain/DomainParticipantListener.hpp>

#include <org/eclipse/cyclonedds/domain/DomainParticipantDelegate.hpp>
#include <org/eclipse/cyclonedds/domain/DomainParticipantRegistry.hpp>
#include <org/eclipse/cyclonedds/domain/DomainParticipantListener.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/core/MiscUtils.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>
#include <org/eclipse/cyclonedds/core/ListenerDispatcher.hpp>
#include <org/eclipse/cyclonedds/pub/AnyDataWriterDelegate.hpp>
#include <org/eclipse/cyclonedds/sub/AnyDataReaderDelegate.hpp>
#include <org/eclipse/cyclonedds/sub/SubscriberDelegate.hpp>
#include <org/eclipse/cyclonedds/topic/AnyTopicDelegate.hpp>
#include <org/eclipse/cyclonedds/sub/BuiltinSubscriberDelegate.hpp>

org::eclipse::cyclonedds::core::Mutex org::eclipse::cyclonedds::domain::DomainParticipantDelegate::global_participants_lock_;
dds::domain::qos::DomainParticipantQos org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_participant_qos_;
org::eclipse::cyclonedds::core::EntitySet org::eclipse::cyclonedds::domain::DomainParticipantDelegate::participants;
org::eclipse::cyclonedds::domain::DomainWrap::map_ref_type org::eclipse::cyclonedds::domain::DomainParticipantDelegate::domain_registry_;

org::eclipse::cyclonedds::domain::DomainParticipantDelegate::DomainParticipantDelegate
(
  uint32_t id,
  const dds::domain::qos::DomainParticipantQos& qos,
  dds::domain::DomainParticipantListener *listener,
  const dds::core::status::StatusMask& event_mask,
  const std::string& config
)
  : domain_id_ (id), qos_(qos)
{
    org::eclipse::cyclonedds::domain::DomainWrap::map_ref_iter it;
    dds_qos_t* ddsc_qos;
    dds_domainid_t did;
    dds_return_t ret;

    /* Validate the qos and get the corresponding ddsc qos. */
    qos.delegate().check();

    dds_entity_t ddsc_par;

    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(global_participants_lock_);
    if (!config.empty()) {
        /* Try to find explicit domain by using the given domain id
         * which should be explicit as well. */
        ISOCPP_BOOL_CHECK_AND_THROW(id != org::eclipse::cyclonedds::domain::default_id(),
                                    ISOCPP_INVALID_ARGUMENT_ERROR,
                                    "When explicitly provide a config, a specific domain id has to be provided as well.");
        it = domain_registry_.find(static_cast<dds_domainid_t>(id));
        if (it == domain_registry_.end()) {
            /* The explicit domain is not available yet. Create it so
             * that the (to be) created participant uses that domain. */
            this->domain_ref_.reset(new org::eclipse::cyclonedds::domain::DomainWrap(static_cast<dds_domainid_t>(id), config));
        } else {
            /* The domain was already created previously.
             * When creating the cyclonedds participant, it'll use
             * that one automatically. */
        }
    }

    ddsc_qos = qos.delegate().ddsc_qos();
    this->listener(listener, event_mask);
    ddsc_par = dds_create_participant(static_cast<dds_domainid_t>(domain_id_), ddsc_qos, this->listener_callbacks);

    dds_delete_qos (ddsc_qos);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_par, "Could not create DomainParticipant.");

    /* Domain id is possibly changed when using default_id() */
    ret = dds_get_domainid(ddsc_par, &did);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Failed to create useful DomainParticipant");
    this->domain_id_ = static_cast<uint32_t>(did);

    this->set_ddsc_entity(ddsc_par);

    if (config.empty()) {
        /* Try to find implicit domain by using the domain id of
         * the created participant. */
        it = domain_registry_.find(did);
        if (it == domain_registry_.end()) {
            /* Create the implicit domain placeholder. */
            this->domain_ref_.reset(new org::eclipse::cyclonedds::domain::DomainWrap(did, config));
        }
    }
    if (this->domain_ref_) {
        /* Add new domain to registry. */
        domain_registry_[did] = this->domain_ref_;
    } else {
        /* Remember domain from registry. */
        this->domain_ref_ = domain_registry_[did];
    }
}

org::eclipse::cyclonedds::domain::DomainParticipantDelegate::DomainParticipantDelegate(
  uint32_t id,
  const dds::domain::qos::DomainParticipantQos & qos,
  dds::domain::DomainParticipantListener * listener,
  const dds::core::status::StatusMask & event_mask,
  const ddsi_config & config
)
  : domain_id_(id), qos_(qos)
{
    org::eclipse::cyclonedds::domain::DomainWrap::map_ref_iter it;
    dds_qos_t * ddsc_qos;
    dds_domainid_t did;
    dds_return_t ret;

    /* Validate the qos and get the corresponding ddsc qos. */
    qos.delegate().check();
    ddsc_qos = qos.delegate().ddsc_qos();

    dds_entity_t ddsc_par;

    org::eclipse::cyclonedds::core::ScopedMutexLock scopedLock(global_participants_lock_);
    it = domain_registry_.find(static_cast<dds_domainid_t>(id));
    if (it == domain_registry_.end()) {
      /* The explicit domain is not available yet. Create it so
       * that the (to be) created participant uses that domain. */
      this->domain_ref_.reset(new org::eclipse::cyclonedds::domain::DomainWrap(
        static_cast<dds_domainid_t>(id), config));
    } else {
      /* The domain was already created previously.
       * When creating the cyclonedds participant, it'll use
       * that one automatically. */
    }

    this->listener(listener, event_mask);
    ddsc_par = dds_create_participant(static_cast<dds_domainid_t>(id), ddsc_qos, this->listener_callbacks);

    dds_delete_qos(ddsc_qos);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ddsc_par, "Could not create DomainParticipant.");

    /* Domain id is possibly changed when using default_id() */
    ret = dds_get_domainid(ddsc_par, &did);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Failed to create useful DomainParticipant");
    this->domain_id_ = static_cast<uint32_t>(did);

    this->set_ddsc_entity(ddsc_par);

    if (this->domain_ref_) {
      /* Add new domain to registry. */
      domain_registry_[did] = this->domain_ref_;
    } else {
      /* Remember domain from registry. */
      this->domain_ref_ = domain_registry_[did];
    }
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::init(ObjectDelegate::weak_ref_type weak_ref)
{
    /* Set weak_ref. */
    this->set_weak_ref(weak_ref);
    /* Add weak_ref to the map of entities */
    this->add_to_entity_map(weak_ref);
    /* No 'factory': always enable. */
    this->enable();
    /* Include participant in list of known participants. */
    org::eclipse::cyclonedds::domain::DomainParticipantDelegate::add_participant(*this);
}

org::eclipse::cyclonedds::domain::DomainParticipantDelegate::~DomainParticipantDelegate()
{
    if (!this->closed) {
        try {
            close();
        } catch (...) {

        }
    }
}

uint32_t
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::domain_id()
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return this->domain_id_;
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::assert_liveliness()
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}

bool
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::contains_entity(
    const dds::core::InstanceHandle& handle)
{
    bool contains = false;

    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    contains = this->publishers.contains(handle);

    if (!contains) {
        contains = this->subscribers.contains(handle);
    }

    if (!contains) {
        contains = this->topics.contains(handle);
    }

    return contains;
}


dds::core::Time
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::current_time() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return org::eclipse::cyclonedds::core::convertTime(dds_time());
}

const dds::domain::qos::DomainParticipantQos&
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::qos() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return this->qos_;
}


void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::qos(
        const dds::domain::qos::DomainParticipantQos& qos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    qos.delegate().check();

    dds_qos_t* p_qos = qos.delegate().ddsc_qos();

    dds_return_t ret = dds_set_qos(this->ddsc_entity, p_qos);
    dds_delete_qos(p_qos);

    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(ret, "Could not set participant qos.");

    this->qos_ = qos;
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::close()
{
    org::eclipse::cyclonedds::domain::DomainWrap::map_ref_iter it;
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();

    this->publishers.all_close();
    this->subscribers.all_close();
    this->cfTopics.all_close();
    this->topics.all_close();

    /* Stop listener. */
    this->listener_set(NULL, dds::core::status::StatusMask::none());

    org::eclipse::cyclonedds::domain::DomainParticipantRegistry::remove(this);

    org::eclipse::cyclonedds::domain::DomainParticipantDelegate::remove_participant(*this);

    org::eclipse::cyclonedds::core::EntityDelegate::close();

    org::eclipse::cyclonedds::core::ScopedMutexLock scopedMLock(global_participants_lock_);
    /* Release domain. */
    this->domain_ref_.reset();
    /* Update registery when needed. */
    it = domain_registry_.find(static_cast<dds_domainid_t>(this->domain_id_));
    assert(it != domain_registry_.end());
    if (it->second.use_count() == 1) {
        /* Only available in the map, no reference
         * in any participant: remove it. */
        domain_registry_.erase(it);
    }
}

dds::topic::qos::TopicQos
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_topic_qos() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return this->default_topic_qos_;
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_topic_qos(
        const dds::topic::qos::TopicQos& qos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    qos.delegate().check();
    this->default_topic_qos_ = qos;
}

dds::pub::qos::PublisherQos
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_publisher_qos() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return this->default_pub_qos_;
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_publisher_qos(
        const ::dds::pub::qos::PublisherQos& qos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    qos.delegate().check();
    this->default_pub_qos_ = qos;
}

dds::sub::qos::SubscriberQos
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_subscriber_qos() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    return this->default_sub_qos_;
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_subscriber_qos(
        const ::dds::sub::qos::SubscriberQos& qos)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->check();
    qos.delegate().check();
    this->default_sub_qos_ = qos;
}

dds::domain::qos::DomainParticipantQos
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_participant_qos()
{
    global_participants_lock_.lock();
    dds::domain::qos::DomainParticipantQos qos = default_participant_qos_;
    global_participants_lock_.unlock();

    return qos;
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::default_participant_qos(
        const ::dds::domain::qos::DomainParticipantQos& qos)
{
    qos.delegate().check();

    global_participants_lock_.lock();
    default_participant_qos_= qos;
    global_participants_lock_.unlock();
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::add_participant(
        org::eclipse::cyclonedds::core::EntityDelegate& participant)
{
    org::eclipse::cyclonedds::domain::DomainParticipantDelegate::participants.insert(participant);
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::remove_participant(
        org::eclipse::cyclonedds::core::EntityDelegate& participant)
{
    org::eclipse::cyclonedds::domain::DomainParticipantDelegate::participants.erase(participant);
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::add_publisher(
        org::eclipse::cyclonedds::core::EntityDelegate& publisher)
{
    this->publishers.insert(publisher);
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::remove_publisher(
        org::eclipse::cyclonedds::core::EntityDelegate& publisher)
{
    this->publishers.erase(publisher);
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::add_subscriber(
        org::eclipse::cyclonedds::core::EntityDelegate& subscriber)
{
    this->subscribers.insert(subscriber);
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::remove_subscriber(
        org::eclipse::cyclonedds::core::EntityDelegate& subscriber)
{
    this->subscribers.erase(subscriber);
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::add_topic(
        org::eclipse::cyclonedds::core::EntityDelegate& topic)
{
    this->topics.insert(topic);
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::remove_topic(
        org::eclipse::cyclonedds::core::EntityDelegate& topic)
{
    this->topics.erase(topic);
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::add_cfTopic(
        org::eclipse::cyclonedds::core::ObjectDelegate& cfTopic)
{
    this->cfTopics.insert(cfTopic);
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::remove_cfTopic(
        org::eclipse::cyclonedds::core::ObjectDelegate& cfTopic)
{
    this->cfTopics.erase(cfTopic);
}


dds_entity_t
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::lookup_topic(
        const std::string& topic_name,
        const dds_typeinfo_t *type_info,
        const dds::core::Duration& timeout)
{
    this->check();
    return dds_find_topic(DDS_FIND_SCOPE_LOCAL_DOMAIN, this->ddsc_entity, topic_name.c_str(), type_info,  org::eclipse::cyclonedds::core::convertDuration (timeout));
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::lookup_topics(
        const std::string&,
        std::vector<dds_entity_t>&,
        uint32_t)
{
    /*
     * This is used by Topic<T>::discover_topics() and
     * AnyTopicDelegate::discover_topics(), which are used
     * by topic/discovery.hpp.
     */
    /* Add support during Topic discovery implementation. */
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Function not currently supported");
}


org::eclipse::cyclonedds::core::EntityDelegate::ref_type
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::find_topic(
        const std::string& topic_name)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    org::eclipse::cyclonedds::topic::AnyTopicDelegate::ref_type topic;

    org::eclipse::cyclonedds::core::EntitySet::vector entities;
    org::eclipse::cyclonedds::core::EntitySet::vectorIterator iter;

    entities = this->topics.copy();
    iter = entities.begin();

    for (iter = entities.begin(); iter != entities.end(); ++iter) {
        org::eclipse::cyclonedds::core::ObjectDelegate::ref_type ref = iter->lock();
        if (ref) {
            org::eclipse::cyclonedds::topic::AnyTopicDelegate::ref_type tmp =
                    ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::topic::AnyTopicDelegate>(ref);
            assert(tmp);
            if (tmp->name() == topic_name) {
                topic = tmp;
            }
        }
    }

    return topic;
}

org::eclipse::cyclonedds::core::ObjectDelegate::ref_type
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::find_cfTopic(
        const std::string& topic_name)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    org::eclipse::cyclonedds::topic::TopicDescriptionDelegate::ref_type cftopic;

    org::eclipse::cyclonedds::core::ObjectSet::vector entities;
    org::eclipse::cyclonedds::core::ObjectSet::vectorIterator iter;

    entities = this->cfTopics.copy();
    iter = entities.begin();

    for (iter = entities.begin(); iter != entities.end(); ++iter) {
        org::eclipse::cyclonedds::core::ObjectDelegate::ref_type ref = iter->lock();
        if (ref) {
            org::eclipse::cyclonedds::topic::TopicDescriptionDelegate::ref_type tmp =
                    ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::topic::TopicDescriptionDelegate>(ref);
            assert(tmp);
            if (tmp->name() == topic_name) {
                cftopic = tmp;
            }
        }
    }

    return cftopic;
}

org::eclipse::cyclonedds::domain::DomainParticipantDelegate::ref_type
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::lookup_participant(uint32_t domain_id)
{
    org::eclipse::cyclonedds::domain::DomainParticipantDelegate::ref_type participant;

    org::eclipse::cyclonedds::core::EntitySet::vector entities;
    org::eclipse::cyclonedds::core::EntitySet::vectorIterator iter;

    entities = participants.copy();
    iter = entities.begin();

    for (iter = entities.begin(); iter != entities.end(); ++iter) {
        org::eclipse::cyclonedds::core::ObjectDelegate::ref_type ref = iter->lock();
        if (ref) {
            org::eclipse::cyclonedds::domain::DomainParticipantDelegate::ref_type tmp =
                    ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::domain::DomainParticipantDelegate>(ref);
            assert(tmp);
            if (tmp->domain_id() == domain_id) {
                participant = tmp;
            }
        }
    }

    return participant;
}

org::eclipse::cyclonedds::core::EntityDelegate::ref_type
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::builtin_subscriber()
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

    org::eclipse::cyclonedds::core::EntityDelegate::ref_type builtinSub = this->builtin_subscriber_.lock();

    return builtinSub;
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::builtin_subscriber(
    const org::eclipse::cyclonedds::core::EntityDelegate::ref_type subscriber)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->builtin_subscriber_ = subscriber;
}


void org::eclipse::cyclonedds::domain::DomainParticipantDelegate::ignore_participant(
    const ::dds::core::InstanceHandle& handle)
{
    (void)handle;
    ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "ignore_participant not implemented");
}


void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::listener(
        dds::domain::DomainParticipantListener *listener,
        const ::dds::core::status::StatusMask& mask)
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    this->listener_set(listener, mask);
    scopedLock.unlock();
}


dds::domain::DomainParticipantListener*
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::listener() const
{
    this->check();
    return reinterpret_cast<dds::domain::DomainParticipantListener*>(this->listener_get());
}


dds::domain::TDomainParticipant<org::eclipse::cyclonedds::domain::DomainParticipantDelegate>
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::wrapper()
{
    DomainParticipantDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<DomainParticipantDelegate>(this->get_strong_ref());
    dds::domain::DomainParticipant dp(ref);
    return dp;
}

bool
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::is_auto_enable() const
{
    org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
    return this->qos_.delegate().policy<dds::core::policy::EntityFactory>().delegate().auto_enable();
}

void
org::eclipse::cyclonedds::domain::DomainParticipantDelegate::listener_notify(
        ObjectDelegate::ref_type source,
        uint32_t                 triggerMask,
        void                    *eventData,
        void                    *l)
{
    (void)source;
    (void)triggerMask;
    (void)eventData;
    (void)l;
}


// Subscriber events
void org::eclipse::cyclonedds::domain::DomainParticipantDelegate::on_data_readers(dds_entity_t subscriber)
{
    org::eclipse::cyclonedds::sub::SubscriberDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::sub::SubscriberDelegate>(
                this->extract_strong_ref(subscriber));
    dds::sub::Subscriber sub(ref);

    this->listener()->on_data_on_readers(sub);
}

// Reader events
void org::eclipse::cyclonedds::domain::DomainParticipantDelegate::on_requested_deadline_missed(dds_entity_t reader,
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

void org::eclipse::cyclonedds::domain::DomainParticipantDelegate::on_requested_incompatible_qos(dds_entity_t reader,
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

void org::eclipse::cyclonedds::domain::DomainParticipantDelegate::on_sample_rejected(dds_entity_t reader,
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

void org::eclipse::cyclonedds::domain::DomainParticipantDelegate::on_liveliness_changed(dds_entity_t reader,
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

void org::eclipse::cyclonedds::domain::DomainParticipantDelegate::on_data_available(dds_entity_t reader)
{
    org::eclipse::cyclonedds::sub::AnyDataReaderDelegate::ref_type ref =
            ::std::dynamic_pointer_cast<org::eclipse::cyclonedds::sub::AnyDataReaderDelegate>(
                this->extract_strong_ref(reader));
    dds::sub::AnyDataReader adr(ref);

    this->listener()->on_data_available(adr);
}

void org::eclipse::cyclonedds::domain::DomainParticipantDelegate::on_subscription_matched(dds_entity_t reader,
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

void org::eclipse::cyclonedds::domain::DomainParticipantDelegate::on_sample_lost(dds_entity_t reader,
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
