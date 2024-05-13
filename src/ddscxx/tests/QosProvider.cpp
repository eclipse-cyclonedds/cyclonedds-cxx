// Copyright(c) 2006 to 2022 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <gtest/gtest.h>

#include "dds/dds.hpp"
#include "dds/ddsc/dds_public_qos_provider.h"
#include "dds/ddsrt/string.h"
#include "dds/ddsrt/io.h"

#define QOS_LENGTH_UNLIMITED       "LENGTH_UNLIMITED"
#define QOS_DURATION_INFINITY      "DURATION_INFINITY"
#define QOS_DURATION_INFINITY_SEC  "DURATION_INFINITE_SEC"
#define QOS_DURATION_INFINITY_NSEC "DURATION_INFINITE_NSEC"

#define DEF(libs) \
  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<dds>" libs "\n</dds>"
#define LIB(lib_name,profiles) \
  "\n <qos_library name=\"" #lib_name "\">" profiles "\n </qos_library>"
#define N_LIB(profiles) \
  "\n <qos_library>" profiles "\n </qos_library>"
#define PRO(prof_name,ents) \
  "\n  <qos_profile name=\"" #prof_name "\">" ents "\n  </qos_profile>"
#define N_PRO(ents) \
  "\n  <qos_profile>" ents "\n  </qos_profile>"
#define ENT(qos,kind) \
  "\n   <" #kind "_qos>" qos "\n   </" #kind "_qos>"
#define ENT_N(nm,qos,kind) \
  "\n   <" #kind "_qos name=\"" #nm "\">" qos "\n   </" #kind "_qos>"


#define QOS_DURATION_FMT(unit) \
  "<" #unit ">%lli</" #unit ">"
#define QOS_DURATION_FMT_STR(unit) \
  "<" #unit ">%s</" #unit ">"
#define QOS_POLICY_DEADLINE_PERIOD_FMT(duration_fmt) \
  "<period>" duration_fmt "</period>"
#define QOS_POLICY_DEADLINE_FMT(unit) \
  "<deadline>" QOS_POLICY_DEADLINE_PERIOD_FMT(unit) "</deadline>"
#define QOS_POLICY_DESTINATION_ORDER_FMT(k) \
  "<destination_order><kind>" #k "</kind></destination_order>"
#define QOS_POLICY_DURABILITY_FMT(k) \
  "<durability><kind>" #k "</kind></durability>"
#define QOS_HISTORY_FMT(hk) \
  "<history><kind>" #hk "</kind><depth>%d</depth></history>"
#define QOS_SERVICE_CLEANUP_DELAY_FMT(duration_fmt) \
  "<service_cleanup_delay>" duration_fmt "</service_cleanup_delay>"
#define QOS_RESOURCE_LIMITS_MS(ms_f) \
  "<max_samples>" ms_f "</max_samples>"
#define QOS_RESOURCE_LIMITS_MI(mi_f) \
  "<max_instances>" mi_f "</max_instances>"
#define QOS_RESOURCE_LIMITS_MSPI(mspi_f) \
  "<max_samples_per_instance>" mspi_f "</max_samples_per_instance>"
#define QOS_RESOURCE_LIMITS \
  "%s%s%s"
#define QOS_DURABILITY_SERVICE_HISTORY(hk) \
  "<history_kind>" #hk "</history_kind><history_depth>%d</history_depth>"
#define QOS_POLICY_DURABILITY_SERVICE_FMT \
  "<durability_service>%s%s" QOS_RESOURCE_LIMITS "</durability_service>"
#define QOS_POLICY_ENTITYFACTORY_FMT(val) \
  "<entity_factory>" \
    "<autoenable_created_entities>" #val "</autoenable_created_entities>" \
  "</entity_factory>"
#define QOS_BASE64_VALUE \
  "<value>%s</value>"
#define QOS_POLICY_GOUPDATA_FMT \
  "<group_data>" QOS_BASE64_VALUE "</group_data>"
#define QOS_POLICY_HISTORY_FMT(hk) \
  QOS_HISTORY_FMT(hk)
#define QOS_POLICY_LATENCYBUDGET_FMT(duration_fmt) \
  "<latency_budget><duration>" duration_fmt "</duration></latency_budget>"
#define QOS_POLICY_LIFESPAN_FMT(duration_fmt) \
  "<lifespan><duration>" duration_fmt "</duration></lifespan>"
#define QOS_LIVELINESS_KIND(lk) \
  "<kind>" #lk "</kind>"
#define QOS_LIVELINESS_DURATION(duration_fmt) \
  "<lease_duration>" duration_fmt "</lease_duration>"
#define QOS_POLICY_LIVELINESS_FMT \
  "<liveliness>%s%s</liveliness>"
#define QOS_POLICY_OWNERSHIP_FMT(ok) \
  "<ownership><kind>" #ok "</kind></ownership>"
#define QOS_POLICY_OWNERSHIPSTRENGTH_FMT \
  "<ownership_strength><value>%d</value></ownership_strength>"
#define QOS_PARTITION_ELEMENT \
  "<element>%s</element>"
#define QOS_POLIC_PARTITION_FMT \
  "<partition><name>%s</name></partition>"
#define QOS_ACCESS_SCOPE_KIND(ask) \
  "<access_scope>" #ask "</access_scope>"
#define QOS_COHERENT_ACCESS(ca) \
  "<coherent_access>" #ca "</coherent_access>"
#define QOS_ORDERED_ACCESS(oa) \
 "<ordered_access>" #oa "</ordered_access>"
#define QOS_POLICY_PRESENTATION_FMT \
  "<presentation>%s%s%s</presentation>"
#define QOS_RELIABILITY_KIND(rk) \
  "<kind>" #rk "</kind>"
#define QOS_RELIABILITY_DURATION(duration_fmt) \
  "<max_blocking_time>" duration_fmt "</max_blocking_time>"
#define QOS_POLICY_RELIABILITY_FMT \
  "<reliability>%s%s</reliability>"
#define QOS_POLICY_RESOURCE_LIMITS_FMT \
  "<resource_limits>" QOS_RESOURCE_LIMITS "</resource_limits>"
#define QOS_POLICY_TIMEBASEDFILTER_FMT(duration_fmt) \
  "<time_based_filter><minimum_separation>" \
    duration_fmt \
  "</minimum_separation></time_based_filter>"
#define QOS_POLICY_TOPICDATA_FMT \
  "<topic_data>" QOS_BASE64_VALUE "</topic_data>"
#define QOS_POLICY_TRANSPORTPRIORITY_FMT \
  "<transport_priority><value>%d</value></transport_priority>"
#define QOS_POLICY_USERDATA_FMT \
  "<user_data>" QOS_BASE64_VALUE "</user_data>"
#define QOS_NOWRITER_DELAY(duration_fmt) \
  "<autopurge_nowriter_samples_delay>" duration_fmt "</autopurge_nowriter_samples_delay>"
#define QOS_DISPOSED_DELAY(duration_fmt) \
  "<autopurge_disposed_samples_delay>" \
    duration_fmt \
  "</autopurge_disposed_samples_delay>"
#define QOS_POLICY_READERDATALIFECYCLE_FMT \
  "<reader_data_lifecycle>%s%s</reader_data_lifecycle>"
#define QOS_POLICY_WRITERDATA_LIFECYCLE_FMT(aui) \
  "<writer_data_lifecycle>" \
    "<autodispose_unregistered_instances>" #aui "</autodispose_unregistered_instances>" \
  "</writer_data_lifecycle>"



/**
 * Fixture for the DataReader tests
 */
class QosProvider : public ::testing::Test
{
public:
    dds::domain::qos::DomainParticipantQos pQos;
    dds::sub::qos::SubscriberQos subQos;
    dds::pub::qos::PublisherQos pubQos;
    dds::topic::qos::TopicQos tQos;
    dds::sub::qos::DataReaderQos rQos;
    dds::pub::qos::DataWriterQos wQos;

    enum duration_unit
    {
      sec = 0,
      nsec = 1
    };

    typedef struct sysdef_qos_conf
    {
      enum duration_unit deadline_unit;
      enum duration_unit durability_serv_unit;
      enum duration_unit latency_budget_unit;
      enum duration_unit lifespan_unit;
      enum duration_unit liveliness_unit;
      enum duration_unit reliability_unit;
      enum duration_unit time_based_filter_unit;
      enum duration_unit reader_data_lifecycle_nowriter_unit;
      enum duration_unit reader_data_lifecycle_disposed_unit;
    } sysdef_qos_conf_t;

constexpr static const unsigned char base64_etable[64] = {
  'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U',
  'V','W','X','Y','Z','a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p',
  'q','r','s','t','u','v','w','x','y','z','0','1','2','3','4','5','6','7','8','9','+','/'
};

static uint32_t b64_encode (const unsigned char *text, const uint32_t sz, unsigned char **buff)
{
  uint32_t act_len = (sz * 4U/3U); 
  uint32_t buff_len = (act_len % 4U) != 0u? ((act_len / 4U + 1U)*4U): act_len;
  *buff =  static_cast<unsigned char *>(ddsrt_malloc(buff_len));
  (void) memset (*buff, '=', buff_len);

  for (size_t i = 0, j = 0; i < buff_len && j < sz; i+=4, j+=3)
  {
    unsigned char chunk[4] = {0x00, 0x00, 0x00, 0x00};
    size_t cp_sz = (sz - j);
    unsigned char tmp[3] = {text[j], static_cast<unsigned char>(((cp_sz > 1)? text[j+1]: 0x00)), static_cast<unsigned char>((cp_sz > 2)? text[j+2]: 0x00)};
    chunk[3] = base64_etable[tmp[2] & 0x3FU];
    chunk[2] = base64_etable[((tmp[1] & 0x0FU) << 0x02U) | (tmp[2] & 0xC0U) >> 0x06U];
    chunk[1] = base64_etable[((tmp[0] & 0x03U) << 0x04U) | (tmp[1] >> 0x04U)];
    chunk[0] = base64_etable[(tmp[0] >> 0x02U)];
    (void) memcpy(*(buff)+i, chunk, cp_sz < 3? cp_sz + 1: 4U);
  }

  return buff_len;
}

#define QOS_FORMAT "     "
#define CHECK_RET_OK(ret) \
  if (ret < 0) goto fail;
    static inline dds_return_t qos_to_conf(dds_qos_t *qos, const sysdef_qos_conf_t *conf, char **out, dds_qos_kind_t kind, uint64_t *validate_mask, const bool ignore_ent)
    {
      char *sysdef_qos = ddsrt_strdup("");
      dds_return_t ret = DDS_RETCODE_OK;
      if ((ignore_ent || (kind == DDS_TOPIC_QOS || kind == DDS_READER_QOS || kind == DDS_WRITER_QOS)) &&
          (ret >= 0) && (qos->present & DDSI_QP_DEADLINE))
      {
        char *deadline;
        if (qos->deadline.deadline == DDS_INFINITY)
        {
          if (conf->deadline_unit == sec)
            ret = ddsrt_asprintf(&deadline, QOS_POLICY_DEADLINE_FMT(QOS_DURATION_FMT_STR(sec)),
                                 QOS_DURATION_INFINITY_SEC);
          else
            ret = ddsrt_asprintf(&deadline, QOS_POLICY_DEADLINE_FMT(QOS_DURATION_FMT_STR(nanosec)),
                                 QOS_DURATION_INFINITY_NSEC);
        } else {
          if (conf->deadline_unit == sec)
            ret = ddsrt_asprintf(&deadline, QOS_POLICY_DEADLINE_FMT(QOS_DURATION_FMT(sec)),
                                 static_cast<long long>(qos->deadline.deadline/DDS_NSECS_IN_SEC));
          else
            ret = ddsrt_asprintf(&deadline, QOS_POLICY_DEADLINE_FMT(QOS_DURATION_FMT(nanosec)),
                                 static_cast<long long>(qos->deadline.deadline));
        }
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, deadline);
        ddsrt_free(tmp);
        ddsrt_free(deadline);
        *validate_mask |= DDSI_QP_DEADLINE;
      }
      if ((ignore_ent || (kind == DDS_TOPIC_QOS || kind == DDS_READER_QOS || kind == DDS_WRITER_QOS)) &&
          (ret >= 0) && (qos->present & DDSI_QP_DESTINATION_ORDER))
      {
        char *dest_order;
        if (qos->destination_order.kind == DDS_DESTINATIONORDER_BY_RECEPTION_TIMESTAMP)
          ret = ddsrt_asprintf(&dest_order, "%s", QOS_POLICY_DESTINATION_ORDER_FMT(BY_RECEPTION_TIMESTAMP_DESTINATIONORDER_QOS));
        else
          ret = ddsrt_asprintf(&dest_order, "%s", QOS_POLICY_DESTINATION_ORDER_FMT(BY_SOURCE_TIMESTAMP_DESTINATIONORDER_QOS));
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, dest_order);
        ddsrt_free(tmp);
        ddsrt_free(dest_order);
        *validate_mask |= DDSI_QP_DESTINATION_ORDER;
      }
      if ((ignore_ent || (kind != DDS_SUBSCRIBER_QOS && kind != DDS_PUBLISHER_QOS && kind != DDS_PARTICIPANT_QOS)) &&
          (ret >= 0) && (qos->present & DDSI_QP_DURABILITY))
      {
        char *durability;
        if (qos->durability.kind == DDS_DURABILITY_VOLATILE)
          ret = ddsrt_asprintf(&durability, "%s", QOS_POLICY_DURABILITY_FMT(VOLATILE_DURABILITY_QOS));
        else if (qos->durability.kind == DDS_DURABILITY_TRANSIENT_LOCAL)
          ret = ddsrt_asprintf(&durability, "%s", QOS_POLICY_DURABILITY_FMT(TRANSIENT_LOCAL_DURABILITY_QOS));
        else if (qos->durability.kind == DDS_DURABILITY_TRANSIENT)
          ret = ddsrt_asprintf(&durability, "%s", QOS_POLICY_DURABILITY_FMT(TRANSIENT_DURABILITY_QOS));
        else
          ret = ddsrt_asprintf(&durability, "%s", QOS_POLICY_DURABILITY_FMT(PERSISTENT_DURABILITY_QOS));
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, durability);
        ddsrt_free(tmp);
        ddsrt_free(durability);
        *validate_mask |= DDSI_QP_DURABILITY;
      }
      if ((ignore_ent || (kind == DDS_WRITER_QOS || kind == DDS_TOPIC_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_DURABILITY_SERVICE)
      {
        char *service_cleanup_delay;
        if (qos->durability_service.service_cleanup_delay == DDS_INFINITY)
        {
          if (conf->durability_serv_unit == sec)
            ret = ddsrt_asprintf(&service_cleanup_delay, QOS_SERVICE_CLEANUP_DELAY_FMT(QOS_DURATION_FMT_STR(sec)),
                                 QOS_DURATION_INFINITY_SEC);
          else
            ret = ddsrt_asprintf(&service_cleanup_delay, QOS_SERVICE_CLEANUP_DELAY_FMT(QOS_DURATION_FMT_STR(nanosec)),
                                 QOS_DURATION_INFINITY_NSEC);
        } else {
          if (conf->durability_serv_unit == sec)
            ret = ddsrt_asprintf(&service_cleanup_delay, QOS_SERVICE_CLEANUP_DELAY_FMT(QOS_DURATION_FMT(sec)),
                                 static_cast<long long>(qos->durability_service.service_cleanup_delay/DDS_NSECS_IN_SEC));
          else
            ret = ddsrt_asprintf(&service_cleanup_delay, QOS_SERVICE_CLEANUP_DELAY_FMT(QOS_DURATION_FMT(nanosec)),
                                 static_cast<long long>(qos->durability_service.service_cleanup_delay));
        }
        CHECK_RET_OK(ret);
        char *history;
        if (qos->durability_service.history.kind == DDS_HISTORY_KEEP_LAST)
          ret = ddsrt_asprintf(&history, QOS_DURABILITY_SERVICE_HISTORY(KEEP_LAST_HISTORY_QOS), qos->durability_service.history.depth);
        else
          ret = ddsrt_asprintf(&history, QOS_DURABILITY_SERVICE_HISTORY(KEEP_ALL_HISTORY_QOS), qos->durability_service.history.depth);
        CHECK_RET_OK(ret);
        char *durability_service;
        int32_t ms,mi,mspi;
        ms = qos->durability_service.resource_limits.max_samples;
        mi = qos->durability_service.resource_limits.max_instances;
        mspi = qos->durability_service.resource_limits.max_samples_per_instance;
        char *ms_f,*mi_f,*mspi_f;
        (void) ddsrt_asprintf(&ms_f,(ms<0? QOS_RESOURCE_LIMITS_MS(QOS_LENGTH_UNLIMITED): QOS_RESOURCE_LIMITS_MS("%d")), ms);
        (void) ddsrt_asprintf(&mi_f,(mi<0? QOS_RESOURCE_LIMITS_MI(QOS_LENGTH_UNLIMITED): QOS_RESOURCE_LIMITS_MI("%d")), mi);
        (void) ddsrt_asprintf(&mspi_f,(mspi<0? QOS_RESOURCE_LIMITS_MSPI(QOS_LENGTH_UNLIMITED): QOS_RESOURCE_LIMITS_MSPI("%d")), mspi);
        ret = ddsrt_asprintf(&durability_service, QOS_POLICY_DURABILITY_SERVICE_FMT, service_cleanup_delay, history, ms_f, mi_f, mspi_f);
        ddsrt_free(ms_f);ddsrt_free(mi_f);ddsrt_free(mspi_f);
        ddsrt_free(service_cleanup_delay);
        ddsrt_free(history);
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, durability_service);
        ddsrt_free(tmp);
        ddsrt_free(durability_service);
        *validate_mask |= DDSI_QP_DURABILITY_SERVICE;
      }
      if ((ignore_ent || (kind == DDS_PARTICIPANT_QOS || kind == DDS_PUBLISHER_QOS || kind == DDS_SUBSCRIBER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_ADLINK_ENTITY_FACTORY)
      {
        char *entity_factory;
        if (qos->entity_factory.autoenable_created_entities == 0)
          ret = ddsrt_asprintf(&entity_factory, "%s", QOS_POLICY_ENTITYFACTORY_FMT(false));
        else
          ret = ddsrt_asprintf(&entity_factory, "%s", QOS_POLICY_ENTITYFACTORY_FMT(true));
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, entity_factory);
        ddsrt_free(tmp);
        ddsrt_free(entity_factory);
        *validate_mask |= DDSI_QP_ADLINK_ENTITY_FACTORY;
      }
      if ((ignore_ent || (kind == DDS_PUBLISHER_QOS || kind == DDS_SUBSCRIBER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_GROUP_DATA)
      {
        if (qos->group_data.length > 0)
        {
          unsigned char *data_buff;
          size_t len = b64_encode(qos->group_data.value, qos->group_data.length, &data_buff);

          char *data = ddsrt_strdup("");
          for (uint32_t i = 0; i < len; i++) {
            char *tmp = data;
            ret = ddsrt_asprintf(&data, "%s%c", data, data_buff[i]);
            ddsrt_free(tmp);
            CHECK_RET_OK(ret);
          }

          char *group_data;
          ret = ddsrt_asprintf(&group_data, QOS_POLICY_GOUPDATA_FMT, data);
          ddsrt_free(data_buff);
          ddsrt_free(data);
          CHECK_RET_OK(ret);
          char *tmp = sysdef_qos;
          ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, group_data);
          ddsrt_free(tmp);
          ddsrt_free(group_data);
          *validate_mask |= DDSI_QP_GROUP_DATA;
        }
      }
      if ((ignore_ent || (kind == DDS_TOPIC_QOS || kind == DDS_READER_QOS || kind == DDS_WRITER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_HISTORY)
      {
        char *history;
        if (qos->history.kind == DDS_HISTORY_KEEP_LAST)
          ret = ddsrt_asprintf(&history, QOS_POLICY_HISTORY_FMT(KEEP_LAST_HISTORY_QOS), qos->history.depth);
        else
          ret = ddsrt_asprintf(&history, QOS_POLICY_HISTORY_FMT(KEEP_ALL_HISTORY_QOS), qos->history.depth);
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, history);
        ddsrt_free(tmp);
        ddsrt_free(history);
        *validate_mask |= DDSI_QP_HISTORY;
      }
      if ((ignore_ent || (kind == DDS_TOPIC_QOS || kind == DDS_WRITER_QOS || kind == DDS_READER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_LATENCY_BUDGET)
      {
        char *latency_budget;
        if (qos->latency_budget.duration == DDS_INFINITY)
        {
          if (conf->latency_budget_unit == sec)
            ret = ddsrt_asprintf(&latency_budget, QOS_POLICY_LATENCYBUDGET_FMT(QOS_DURATION_FMT_STR(sec)),
                                 QOS_DURATION_INFINITY_SEC);
          else
            ret = ddsrt_asprintf(&latency_budget, QOS_POLICY_LATENCYBUDGET_FMT(QOS_DURATION_FMT_STR(nanosec)),
                                 QOS_DURATION_INFINITY_NSEC);
        } else {
          if (conf->latency_budget_unit == sec)
            ret = ddsrt_asprintf(&latency_budget, QOS_POLICY_LATENCYBUDGET_FMT(QOS_DURATION_FMT(sec)),
                                 static_cast<long long>(qos->latency_budget.duration/DDS_NSECS_IN_SEC));
          else
            ret = ddsrt_asprintf(&latency_budget, QOS_POLICY_LATENCYBUDGET_FMT(QOS_DURATION_FMT(nanosec)),
                                 static_cast<long long>(qos->latency_budget.duration));
        }
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, latency_budget);
        ddsrt_free(tmp);
        ddsrt_free(latency_budget);
        *validate_mask |= DDSI_QP_LATENCY_BUDGET;
      }
      if ((ignore_ent || (kind == DDS_WRITER_QOS || kind == DDS_TOPIC_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_LIFESPAN)
      {
        char *lifespan;
        if (qos->lifespan.duration == DDS_INFINITY)
        {
          if (conf->lifespan_unit == sec)
            ret = ddsrt_asprintf(&lifespan, QOS_POLICY_LIFESPAN_FMT(QOS_DURATION_FMT_STR(sec)),
                                 QOS_DURATION_INFINITY_SEC);
          else
           ret = ddsrt_asprintf(&lifespan, QOS_POLICY_LIFESPAN_FMT(QOS_DURATION_FMT_STR(nanosec)),
                                QOS_DURATION_INFINITY_NSEC);
        } else {
          if (conf->lifespan_unit == sec)
            ret = ddsrt_asprintf(&lifespan, QOS_POLICY_LIFESPAN_FMT(QOS_DURATION_FMT(sec)),
                                 static_cast<long long>(qos->lifespan.duration/DDS_NSECS_IN_SEC));
          else
           ret = ddsrt_asprintf(&lifespan, QOS_POLICY_LIFESPAN_FMT(QOS_DURATION_FMT(nanosec)),
                                static_cast<long long>(qos->lifespan.duration));
        }
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, lifespan);
        ddsrt_free(tmp);
        ddsrt_free(lifespan);
        *validate_mask |= DDSI_QP_LIFESPAN;
      }
      if ((ignore_ent || (kind != DDS_PUBLISHER_QOS && kind != DDS_SUBSCRIBER_QOS && kind != DDS_PARTICIPANT_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_LIVELINESS)
      {
        char *duration;
        if (qos->liveliness.lease_duration == DDS_INFINITY)
        {
          if (conf->liveliness_unit == sec)
            ret = ddsrt_asprintf(&duration, QOS_LIVELINESS_DURATION(QOS_DURATION_FMT_STR(sec)),
                                 QOS_DURATION_INFINITY_SEC);
          else
            ret = ddsrt_asprintf(&duration, QOS_LIVELINESS_DURATION(QOS_DURATION_FMT_STR(nanosec)),
                                 QOS_DURATION_INFINITY_NSEC);
        } else {
          if (conf->liveliness_unit == sec)
            ret = ddsrt_asprintf(&duration, QOS_LIVELINESS_DURATION(QOS_DURATION_FMT(sec)),
                                 static_cast<long long>(qos->liveliness.lease_duration/DDS_NSECS_IN_SEC));
          else
            ret = ddsrt_asprintf(&duration, QOS_LIVELINESS_DURATION(QOS_DURATION_FMT(nanosec)),
                                 static_cast<long long>(qos->liveliness.lease_duration));
        }
        CHECK_RET_OK(ret);
        char *liveliness_kind;
        if (qos->liveliness.kind == DDS_LIVELINESS_AUTOMATIC)
          ret = ddsrt_asprintf(&liveliness_kind, "%s", QOS_LIVELINESS_KIND(AUTOMATIC_LIVELINESS_QOS));
        else if (qos->liveliness.kind == DDS_LIVELINESS_MANUAL_BY_PARTICIPANT)
          ret = ddsrt_asprintf(&liveliness_kind, "%s", QOS_LIVELINESS_KIND(MANUAL_BY_PARTICIPANT_LIVELINESS_QOS));
        else
          ret = ddsrt_asprintf(&liveliness_kind, "%s", QOS_LIVELINESS_KIND(MANUAL_BY_TOPIC_LIVELINESS_QOS));
        CHECK_RET_OK(ret);
        char *liveliness;
        ret = ddsrt_asprintf(&liveliness, QOS_POLICY_LIVELINESS_FMT, duration, liveliness_kind);
        ddsrt_free(duration);
        ddsrt_free(liveliness_kind);
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, liveliness);
        ddsrt_free(tmp);
        ddsrt_free(liveliness);
        *validate_mask |= DDSI_QP_LIVELINESS;
      }
      if ((ignore_ent || (kind != DDS_PUBLISHER_QOS && kind != DDS_SUBSCRIBER_QOS && kind != DDS_PARTICIPANT_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_OWNERSHIP)
      {
        char *ownership;
        if (qos->ownership.kind == DDS_OWNERSHIP_SHARED)
          ret = ddsrt_asprintf(&ownership, "%s", QOS_POLICY_OWNERSHIP_FMT(SHARED_OWNERSHIP_QOS));
        else
          ret = ddsrt_asprintf(&ownership, "%s", QOS_POLICY_OWNERSHIP_FMT(EXCLUSIVE_OWNERSHIP_QOS));
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, ownership);
        ddsrt_free(tmp);
        ddsrt_free(ownership);
        *validate_mask |= DDSI_QP_OWNERSHIP;
      }
      if ((ignore_ent || (kind == DDS_WRITER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_OWNERSHIP_STRENGTH)
      {
        char *ownership_strength;
        ret = ddsrt_asprintf(&ownership_strength, QOS_POLICY_OWNERSHIPSTRENGTH_FMT, qos->ownership_strength.value);
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, ownership_strength);
        ddsrt_free(tmp);
        ddsrt_free(ownership_strength);
        *validate_mask |= DDSI_QP_OWNERSHIP_STRENGTH;
      }
      if ((ignore_ent || (kind == DDS_PUBLISHER_QOS || kind == DDS_SUBSCRIBER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_PARTITION)
      {
        if (qos->partition.n > 0)
        {
          char *part_elems = ddsrt_strdup("");
          for (uint32_t i = 0; i < qos->partition.n; i++) {
            char *tmp = part_elems;
            ret = ddsrt_asprintf(&part_elems, "%s" QOS_PARTITION_ELEMENT, part_elems, qos->partition.strs[i]);
            CHECK_RET_OK(ret);
            ddsrt_free(tmp);
          }
          CHECK_RET_OK(ret);
          char *partition;
          ret = ddsrt_asprintf(&partition, QOS_POLIC_PARTITION_FMT, part_elems);
          ddsrt_free(part_elems);
          CHECK_RET_OK(ret);
          char *tmp = sysdef_qos;
          ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, partition);
          ddsrt_free(tmp);
          ddsrt_free(partition);
          *validate_mask |= DDSI_QP_PARTITION;
        }
      }
      if ((ignore_ent || (kind == DDS_PUBLISHER_QOS || kind == DDS_SUBSCRIBER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_PRESENTATION)
      {
        char *access_scope_kind;
        if (qos->presentation.access_scope == DDS_PRESENTATION_INSTANCE)
          ret =  ddsrt_asprintf(&access_scope_kind, QOS_ACCESS_SCOPE_KIND(INSTANCE_PRESENTATION_QOS));
        else if (qos->presentation.access_scope == DDS_PRESENTATION_TOPIC)
          ret =  ddsrt_asprintf(&access_scope_kind, QOS_ACCESS_SCOPE_KIND(TOPIC_PRESENTATION_QOS));
        else
          ret =  ddsrt_asprintf(&access_scope_kind, QOS_ACCESS_SCOPE_KIND(GROUP_PRESENTATION_QOS));
        CHECK_RET_OK(ret);
        char *coherent_access;
        if (qos->presentation.coherent_access)
          ret = ddsrt_asprintf(&coherent_access, "%s", QOS_COHERENT_ACCESS(true));
        else
          ret = ddsrt_asprintf(&coherent_access, "%s", QOS_COHERENT_ACCESS(false));
        CHECK_RET_OK(ret);
        char *ordered_access;
        if (qos->presentation.ordered_access)
          ret = ddsrt_asprintf(&ordered_access, "%s", QOS_ORDERED_ACCESS(true));
        else
          ret = ddsrt_asprintf(&ordered_access, "%s", QOS_ORDERED_ACCESS(false));
        CHECK_RET_OK(ret);
        char *presentation;
        ret = ddsrt_asprintf(&presentation, QOS_POLICY_PRESENTATION_FMT, access_scope_kind, coherent_access, ordered_access);
        ddsrt_free(access_scope_kind);
        ddsrt_free(coherent_access);
        ddsrt_free(ordered_access);
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, presentation);
        ddsrt_free(tmp);
        ddsrt_free(presentation);
        *validate_mask |= DDSI_QP_PRESENTATION;
      }
      if ((ignore_ent || (kind == DDS_TOPIC_QOS || kind == DDS_READER_QOS || kind == DDS_WRITER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_RELIABILITY)
      {
        char *max_blocking_time;
        if (qos->reliability.max_blocking_time == DDS_INFINITY)
        {
          if (conf->reliability_unit == sec)
            ret = ddsrt_asprintf(&max_blocking_time, QOS_RELIABILITY_DURATION(QOS_DURATION_FMT_STR(sec)),
                                 QOS_DURATION_INFINITY_SEC);
          else
            ret = ddsrt_asprintf(&max_blocking_time, QOS_RELIABILITY_DURATION(QOS_DURATION_FMT_STR(nanosec)),
                                 QOS_DURATION_INFINITY_NSEC);
        } else {
          if (conf->reliability_unit == sec)
            ret = ddsrt_asprintf(&max_blocking_time, QOS_RELIABILITY_DURATION(QOS_DURATION_FMT(sec)),
                                 static_cast<long long>(qos->reliability.max_blocking_time/DDS_NSECS_IN_SEC));
          else
            ret = ddsrt_asprintf(&max_blocking_time, QOS_RELIABILITY_DURATION(QOS_DURATION_FMT(nanosec)),
                                 static_cast<long long>(qos->reliability.max_blocking_time));
        }
        CHECK_RET_OK(ret);
        char *reliability_kind;
        if (qos->reliability.kind == DDS_RELIABILITY_BEST_EFFORT)
          ret = ddsrt_asprintf(&reliability_kind, "%s", QOS_RELIABILITY_KIND(BEST_EFFORT_RELIABILITY_QOS));
        else
          ret = ddsrt_asprintf(&reliability_kind, "%s", QOS_RELIABILITY_KIND(RELIABLE_RELIABILITY_QOS));
        CHECK_RET_OK(ret);
        char *reliability;
        ret = ddsrt_asprintf(&reliability, QOS_POLICY_RELIABILITY_FMT, max_blocking_time, reliability_kind);
        ddsrt_free(max_blocking_time);
        ddsrt_free(reliability_kind);
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, reliability);
        ddsrt_free(tmp);
        ddsrt_free(reliability);
        *validate_mask |= DDSI_QP_RELIABILITY;
      }
      if ((ignore_ent || (kind == DDS_TOPIC_QOS || kind == DDS_READER_QOS || kind == DDS_WRITER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_RESOURCE_LIMITS)
      {
        int32_t ms,mi,mspi;
        ms = qos->resource_limits.max_samples;
        mi = qos->resource_limits.max_instances;
        mspi = qos->resource_limits.max_samples_per_instance;
        char *ms_f,*mi_f,*mspi_f;
        (void) ddsrt_asprintf(&ms_f,(ms<0? QOS_RESOURCE_LIMITS_MS(QOS_LENGTH_UNLIMITED): QOS_RESOURCE_LIMITS_MS("%d")), ms);
        (void) ddsrt_asprintf(&mi_f,(mi<0? QOS_RESOURCE_LIMITS_MI(QOS_LENGTH_UNLIMITED): QOS_RESOURCE_LIMITS_MI("%d")), mi);
        (void) ddsrt_asprintf(&mspi_f,(mspi<0? QOS_RESOURCE_LIMITS_MSPI(QOS_LENGTH_UNLIMITED): QOS_RESOURCE_LIMITS_MSPI("%d")), mspi);
        char *resource_limits;
        ret = ddsrt_asprintf(&resource_limits, QOS_POLICY_RESOURCE_LIMITS_FMT, ms_f, mi_f, mspi_f);
        ddsrt_free(ms_f);ddsrt_free(mi_f);ddsrt_free(mspi_f);
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, resource_limits);
        ddsrt_free(tmp);
        ddsrt_free(resource_limits);
        *validate_mask |= DDSI_QP_RESOURCE_LIMITS;
      }
      if ((ignore_ent || (kind == DDS_READER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_TIME_BASED_FILTER)
      {
        char *time_based_filter;
        if (qos->time_based_filter.minimum_separation == DDS_INFINITY)
        {
          if (conf->time_based_filter_unit == sec)
            ret = ddsrt_asprintf(&time_based_filter, QOS_POLICY_TIMEBASEDFILTER_FMT(QOS_DURATION_FMT_STR(sec)),
                                 QOS_DURATION_INFINITY_SEC);
          else
            ret = ddsrt_asprintf(&time_based_filter, QOS_POLICY_TIMEBASEDFILTER_FMT(QOS_DURATION_FMT_STR(nanosec)),
                                 QOS_DURATION_INFINITY_NSEC);
        } else {
          if (conf->time_based_filter_unit == sec)
            ret = ddsrt_asprintf(&time_based_filter, QOS_POLICY_TIMEBASEDFILTER_FMT(QOS_DURATION_FMT(sec)),
                                 static_cast<long long>(qos->time_based_filter.minimum_separation/DDS_NSECS_IN_SEC));
          else
            ret = ddsrt_asprintf(&time_based_filter, QOS_POLICY_TIMEBASEDFILTER_FMT(QOS_DURATION_FMT(nanosec)),
                                 static_cast<long long>(qos->time_based_filter.minimum_separation));
        }
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, time_based_filter);
        ddsrt_free(tmp);
        ddsrt_free(time_based_filter);
        *validate_mask |= DDSI_QP_TIME_BASED_FILTER;
      }
      if ((ignore_ent || (kind == DDS_TOPIC_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_TOPIC_DATA)
      {
        if (qos->topic_data.length > 0)
        {
          unsigned char *data_buff;
          size_t len = b64_encode(qos->topic_data.value, qos->topic_data.length, &data_buff);

          char *data = ddsrt_strdup("");
          for (uint32_t i = 0; i < len; i++) {
            char *tmp = data;
            ret = ddsrt_asprintf(&data, "%s%c", data, data_buff[i]);
            ddsrt_free(tmp);
            CHECK_RET_OK(ret);
          }
          char *topic_data;
          ret = ddsrt_asprintf(&topic_data, QOS_POLICY_TOPICDATA_FMT, data);
          ddsrt_free(data_buff);
          ddsrt_free(data);
          CHECK_RET_OK(ret);
          char *tmp = sysdef_qos;
          ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, topic_data);
          ddsrt_free(tmp);
          ddsrt_free(topic_data);
          *validate_mask |= DDSI_QP_TOPIC_DATA;
        }
      }
      if ((ignore_ent || (kind == DDS_TOPIC_QOS || kind == DDS_WRITER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_TRANSPORT_PRIORITY)
      {
        char *priority;
        ret = ddsrt_asprintf(&priority, QOS_POLICY_TRANSPORTPRIORITY_FMT, qos->transport_priority.value);
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, priority);
        ddsrt_free(tmp);
        ddsrt_free(priority);
        *validate_mask |= DDSI_QP_TRANSPORT_PRIORITY;
      }
      if ((ignore_ent || (kind == DDS_PARTICIPANT_QOS || kind == DDS_READER_QOS || kind == DDS_WRITER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_USER_DATA)
      {
        if (qos->user_data.length > 0)
        {
          unsigned char *data_buff;
          size_t len = b64_encode(qos->user_data.value, qos->user_data.length, &data_buff);

          char *data = ddsrt_strdup("");
          for (uint32_t i = 0; i < len; i++) {
            char *tmp = data;
            ret = ddsrt_asprintf(&data, "%s%c", data, data_buff[i]);
            ddsrt_free(tmp);
            CHECK_RET_OK(ret);
          }
          char *user_data;
          ret = ddsrt_asprintf(&user_data, QOS_POLICY_USERDATA_FMT, data);
          ddsrt_free(data_buff);
          ddsrt_free(data);
          CHECK_RET_OK(ret);
          char *tmp = sysdef_qos;
          ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, user_data);
          ddsrt_free(tmp);
          ddsrt_free(user_data);
          *validate_mask |= DDSI_QP_USER_DATA;
        }
      }
      if ((ignore_ent || (kind == DDS_READER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_ADLINK_READER_DATA_LIFECYCLE)
      {
        char *nowriter_delay;
        if (qos->reader_data_lifecycle.autopurge_nowriter_samples_delay == DDS_INFINITY)
        {
          if (conf->reader_data_lifecycle_nowriter_unit == sec)
            ret = ddsrt_asprintf(&nowriter_delay, QOS_NOWRITER_DELAY(QOS_DURATION_FMT_STR(sec)),
                                 QOS_DURATION_INFINITY_SEC);
          else
            ret = ddsrt_asprintf(&nowriter_delay, QOS_NOWRITER_DELAY(QOS_DURATION_FMT_STR(nanosec)),
                                 QOS_DURATION_INFINITY_NSEC);
        } else {
          if (conf->reader_data_lifecycle_nowriter_unit == sec)
            ret = ddsrt_asprintf(&nowriter_delay, QOS_NOWRITER_DELAY(QOS_DURATION_FMT(sec)),
                                 static_cast<long long>(qos->reader_data_lifecycle.autopurge_nowriter_samples_delay/DDS_NSECS_IN_SEC));
          else
            ret = ddsrt_asprintf(&nowriter_delay, QOS_NOWRITER_DELAY(QOS_DURATION_FMT(nanosec)),
                                 static_cast<long long>(qos->reader_data_lifecycle.autopurge_nowriter_samples_delay));
        }
        CHECK_RET_OK(ret);
        char *disposed_delay;
        if (qos->reader_data_lifecycle.autopurge_disposed_samples_delay == DDS_INFINITY)
        {
          if (conf->reader_data_lifecycle_disposed_unit == sec)
            ret = ddsrt_asprintf(&disposed_delay, QOS_DISPOSED_DELAY(QOS_DURATION_FMT_STR(sec)),
                                 QOS_DURATION_INFINITY_SEC);
          else
            ret = ddsrt_asprintf(&disposed_delay, QOS_DISPOSED_DELAY(QOS_DURATION_FMT_STR(nanosec)),
                                 QOS_DURATION_INFINITY_NSEC);
        } else {
          if (conf->reader_data_lifecycle_disposed_unit == sec)
            ret = ddsrt_asprintf(&disposed_delay, QOS_DISPOSED_DELAY(QOS_DURATION_FMT(sec)),
                                 static_cast<long long>(qos->reader_data_lifecycle.autopurge_disposed_samples_delay/DDS_NSECS_IN_SEC));
          else
            ret = ddsrt_asprintf(&disposed_delay, QOS_DISPOSED_DELAY(QOS_DURATION_FMT(nanosec)),
                                 static_cast<long long>(qos->reader_data_lifecycle.autopurge_disposed_samples_delay));
        }
        CHECK_RET_OK(ret);
        char *reader_data_lifecycle;
        ret = ddsrt_asprintf(&reader_data_lifecycle, QOS_POLICY_READERDATALIFECYCLE_FMT, nowriter_delay, disposed_delay);
        ddsrt_free(nowriter_delay);
        ddsrt_free(disposed_delay);
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, reader_data_lifecycle);
        ddsrt_free(tmp);
        ddsrt_free(reader_data_lifecycle);
        *validate_mask |= DDSI_QP_ADLINK_READER_DATA_LIFECYCLE;
      }
      if ((ignore_ent || (kind == DDS_WRITER_QOS)) &&
          (ret >= 0) && qos->present & DDSI_QP_ADLINK_WRITER_DATA_LIFECYCLE)
      {
        char *writer_data_lifecycle;
        if (qos->writer_data_lifecycle.autodispose_unregistered_instances)
          ret = ddsrt_asprintf(&writer_data_lifecycle, "%s", QOS_POLICY_WRITERDATA_LIFECYCLE_FMT(true));
        else
          ret = ddsrt_asprintf(&writer_data_lifecycle, "%s", QOS_POLICY_WRITERDATA_LIFECYCLE_FMT(false));
        CHECK_RET_OK(ret);
        char *tmp = sysdef_qos;
        ret = ddsrt_asprintf(&sysdef_qos,   QOS_FORMAT  "%s\n" QOS_FORMAT "%s", sysdef_qos, writer_data_lifecycle);
        ddsrt_free(tmp);
        ddsrt_free(writer_data_lifecycle);
        *validate_mask |= DDSI_QP_ADLINK_WRITER_DATA_LIFECYCLE;
      }

      *out = sysdef_qos;
    fail:
      return ret;
    }

#undef QOS_FORMAT

    static dds_return_t 
    get_single_configuration(dds_qos_t *qos, sysdef_qos_conf_t *conf, dds_qos_kind_t kind, char **out_conf, uint64_t *validate_mask)
    {
      dds_return_t ret = DDS_RETCODE_OK;
      char *qos_conf = NULL;
      ret = qos_to_conf(qos, conf, &qos_conf, kind, validate_mask, false);
      const char *def;
      if (ret >= 0)
      {
        switch(kind)
        {
          case DDS_PARTICIPANT_QOS:
            def = DEF(LIB(lib1,PRO(pro00,ENT("%s",domain_participant))));
            break;
          case DDS_PUBLISHER_QOS:
            def = DEF(LIB(lib1,PRO(pro00,ENT("%s",publisher))));
            break;
          case DDS_SUBSCRIBER_QOS:
            def = DEF(LIB(lib1,PRO(pro00,ENT("%s",subscriber))));
            break;
          case DDS_TOPIC_QOS:
            def = DEF(LIB(lib1,PRO(pro00,ENT("%s",topic))));
            break;
          case DDS_READER_QOS:
            def = DEF(LIB(lib1,PRO(pro00,ENT("%s",datareader))));
            break;
          case DDS_WRITER_QOS:
            def = DEF(LIB(lib1,PRO(pro00,ENT("%s",datawriter))));
            break;
          default:
            ddsrt_free(qos_conf);
            return DDS_RETCODE_ERROR;
        }
        ret = ddsrt_asprintf(out_conf, def, qos_conf);
        ddsrt_free(qos_conf);
      }

      return ret;
    }


    void doTest(dds_qos_kind_t kind)
    {
        dds::domain::DomainParticipant dp(org::eclipse::cyclonedds::domain::default_id());
        dds::pub::Publisher pub(dp);
        dds::sub::Subscriber sub(dp);
        dds_qos_t *qos = NULL;
        dds::core::ByteSeq bs;
        dds::core::StringSeq pList;
        dds_return_t result;
        sysdef_qos_conf_t dur_conf = {sec,sec,sec,sec,sec,sec,sec,sec,sec};
        char *full_configuration = NULL;
        uint64_t validate_mask = 0;

        for (unsigned char c = 'a'; c <= 'd'; c++) bs.push_back(c);
        pList.push_back("abcd");
        pList.push_back("efgh");
        pList.push_back("ijkl");
        switch(kind)
        {
        case DDS_PARTICIPANT_QOS:
            pQos = dds::domain::DomainParticipant::default_participant_qos()
                << dds::core::policy::UserData(bs)
                << dds::core::policy::EntityFactory::AutoEnable();
            qos = pQos.delegate().ddsc_qos();
            break;
        case DDS_PUBLISHER_QOS:
            pubQos = dp.default_publisher_qos()
                << dds::core::policy::GroupData(bs)
                << dds::core::policy::Partition(pList)
                << dds::core::policy::Presentation::InstanceAccessScope(false, false)
                << dds::core::policy::EntityFactory::AutoEnable();
            qos = pubQos.delegate().ddsc_qos();
            break;
        case DDS_SUBSCRIBER_QOS:
            subQos = dp.default_subscriber_qos()
                << dds::core::policy::GroupData(bs)
                << dds::core::policy::Partition(pList)
                << dds::core::policy::Presentation::InstanceAccessScope(false, false)
                << dds::core::policy::EntityFactory::AutoEnable();
            qos = subQos.delegate().ddsc_qos();
            break;
        case DDS_TOPIC_QOS:
            tQos = dp.default_topic_qos()
                << dds::core::policy::TopicData(bs)
                << dds::core::policy::Durability::Volatile()
                << dds::core::policy::Deadline(dds::core::Duration(1,0))
                << dds::core::policy::LatencyBudget(dds::core::Duration(1,0))
                << dds::core::policy::Ownership::Exclusive()
                << dds::core::policy::Liveliness::Automatic(dds::core::Duration::infinite())
                << dds::core::policy::Reliability::Reliable(dds::core::Duration(1, 0))
                << dds::core::policy::TransportPriority(1000)
                << dds::core::policy::Lifespan(dds::core::Duration(1, 0))
                << dds::core::policy::DestinationOrder::SourceTimestamp()
                << dds::core::policy::History::KeepLast(1)
                << dds::core::policy::ResourceLimits(1, 1, 1)
                << dds::core::policy::DurabilityService(dds::core::Duration(1, 0), dds::core::policy::HistoryKind::KEEP_ALL, -1, 1, 1, 1);
            qos = tQos.delegate().ddsc_qos();
            break;
        case DDS_READER_QOS:
            rQos = sub.default_datareader_qos()
                << dds::core::policy::UserData(bs)
                << dds::core::policy::Durability::Volatile()
                << dds::core::policy::Deadline(dds::core::Duration(1,0))
                << dds::core::policy::LatencyBudget(dds::core::Duration(1,0))
                << dds::core::policy::Ownership::Exclusive()
                << dds::core::policy::Liveliness::Automatic(dds::core::Duration::infinite())
                << dds::core::policy::Reliability::Reliable(dds::core::Duration(1, 0))
                << dds::core::policy::DestinationOrder::SourceTimestamp()
                << dds::core::policy::History::KeepLast(1)
                << dds::core::policy::ResourceLimits(1, 1, 1)
                << dds::core::policy::TimeBasedFilter(dds::core::Duration(1, 0))
                << dds::core::policy::ReaderDataLifecycle(dds::core::Duration(1,0), dds::core::Duration(1,0));
            qos = rQos.delegate().ddsc_qos();
            break;
        case DDS_WRITER_QOS:
            wQos = pub.default_datawriter_qos()
                << dds::core::policy::UserData(bs)
                << dds::core::policy::Durability::Volatile()
                << dds::core::policy::Deadline(dds::core::Duration(1,0))
                << dds::core::policy::LatencyBudget(dds::core::Duration(1,0))
                << dds::core::policy::Ownership::Exclusive()
                << dds::core::policy::OwnershipStrength(100)
                << dds::core::policy::Liveliness::Automatic(dds::core::Duration::infinite())
                << dds::core::policy::Reliability::Reliable(dds::core::Duration(1, 0))
                << dds::core::policy::DestinationOrder::SourceTimestamp()
                << dds::core::policy::History::KeepLast(1)
                << dds::core::policy::ResourceLimits(1, 1, 1)
                << dds::core::policy::WriterDataLifecycle::AutoDisposeUnregisteredInstances();
            qos = wQos.delegate().ddsc_qos();
            break;
        default:
            assert(0);
            break;
        }
        result = get_single_configuration(qos, &dur_conf, kind, &full_configuration, &validate_mask);
        ASSERT_TRUE(result >= 0);
        dds::core::QosProvider qp(full_configuration);
        switch(kind)
        {
        case DDS_PARTICIPANT_QOS:
        {
            dds::domain::qos::DomainParticipantQos qpQos = qp.participant_qos("lib1::pro00");
            ASSERT_TRUE(pQos == qpQos);
            break;
        }
        case DDS_PUBLISHER_QOS:
        {
            dds::pub::qos::PublisherQos qpQos = qp.publisher_qos("lib1::pro00");
            ASSERT_TRUE(pubQos == qpQos);
            break;
        }
        case DDS_SUBSCRIBER_QOS:
        {
            dds::sub::qos::SubscriberQos qpQos = qp.subscriber_qos("lib1::pro00");
            ASSERT_TRUE(subQos == qpQos);
            break;
        }
        case DDS_TOPIC_QOS:
        {
            dds::topic::qos::TopicQos qpQos = qp.topic_qos("lib1::pro00");
            ASSERT_TRUE(tQos == qpQos);
            break;
        }
        case DDS_READER_QOS:
        {
            dds::sub::qos::DataReaderQos qpQos = qp.datareader_qos("lib1::pro00");
            ASSERT_TRUE(rQos == qpQos);
            break;
        }
        case DDS_WRITER_QOS:
        {
            dds::pub::qos::DataWriterQos qpQos = qp.datawriter_qos("lib1::pro00");
            ASSERT_TRUE(wQos == qpQos);
            break;
        }
        }
        dds_delete_qos(qos);
        ddsrt_free(full_configuration);
    }
    QosProvider(/*dds_qos_kind_t kind*/)
    {
    }
};

/**
 * Tests
 */

TEST_F(QosProvider, participantQos)
{
    this->doTest(DDS_PARTICIPANT_QOS);
}

TEST_F(QosProvider, publisherQos)
{
    this->doTest(DDS_PUBLISHER_QOS);
}

TEST_F(QosProvider, subscriberQos)
{
    this->doTest(DDS_SUBSCRIBER_QOS);
}

TEST_F(QosProvider, dataWriterQos)
{
    this->doTest(DDS_WRITER_QOS);
}

TEST_F(QosProvider, dataReaderQos)
{
    this->doTest(DDS_READER_QOS);
}

TEST_F(QosProvider, topicQos)
{
    this->doTest(DDS_TOPIC_QOS);
}

