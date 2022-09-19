/**
*  Copyright(c) 2022 ZettaScale Technology and others
*
*   This program and the accompanying materials are made available under the
*   terms of the Eclipse Public License v. 2.0 which is available at
*   http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
*   v. 1.0 which is available at
*   http://www.eclipse.org/org/documents/edl-v10.php.
*
*   SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
*/

#include "dds/dds.hpp"
#include "RoundTrip.hpp"

#include <functional>
#include <csignal>

static bool done = false;
static bool use_listener = false;
static bool timedOut = false;

static unsigned long timeOut = 0;

using namespace org::eclipse::cyclonedds;

static void sigint (int sig)
{
  (void)sig;
  done = true;
  std::cout << std::endl << std::flush;
}

static bool match_readers_and_writers(
  dds::sub::DataReader<RoundTripModule::DataType> &rd,
  dds::pub::DataWriter<RoundTripModule::DataType> &wr,
  dds::core::Duration &timeout)
{
  dds::core::cond::WaitSet waitset;

  dds::core::cond::StatusCondition wsc(wr), rsc(rd);
  wsc.enabled_statuses(dds::core::status::StatusMask::publication_matched());
  rsc.enabled_statuses(dds::core::status::StatusMask::subscription_matched());

  printf ("# Waiting for readers and writers to match up\n");
  try {
    waitset.attach_condition(wsc);
    waitset.wait(timeout);

    waitset.detach_condition(wsc);
    waitset.attach_condition(rsc);
    waitset.wait(timeout);
  } catch (const dds::core::TimeoutError &) {
    return false;
  }
  return true;
}

class RoundTripListener: public dds::sub::DataReaderListener<RoundTripModule::DataType>
{
  public:
  using callback_func = std::function<bool(dds::sub::DataReader<RoundTripModule::DataType>&, dds::pub::DataWriter<RoundTripModule::DataType>&)>;
  RoundTripListener() = delete;
  RoundTripListener(dds::pub::DataWriter<RoundTripModule::DataType> &wr, const callback_func &f):
    dds::sub::DataReaderListener<RoundTripModule::DataType>(), _wr(wr), _f(f) { ; }
  /*implementation of virtual functions*/

  /*only on_data_available does anything*/
  void on_data_available(dds::sub::DataReader<RoundTripModule::DataType>& rd) {
    (void)_f(rd, _wr);
  }

  /*all others are just dummies*/
  void on_requested_deadline_missed(
      dds::sub::DataReader<RoundTripModule::DataType>&,
      const dds::core::status::RequestedDeadlineMissedStatus&) { }

  void on_requested_incompatible_qos(
      dds::sub::DataReader<RoundTripModule::DataType>&,
      const dds::core::status::RequestedIncompatibleQosStatus&) { }

  void on_sample_rejected(
      dds::sub::DataReader<RoundTripModule::DataType>&,
      const dds::core::status::SampleRejectedStatus&) { }

  void on_liveliness_changed(
      dds::sub::DataReader<RoundTripModule::DataType>&,
      const dds::core::status::LivelinessChangedStatus&) { }

  void on_subscription_matched(
      dds::sub::DataReader<RoundTripModule::DataType>&,
      const dds::core::status::SubscriptionMatchedStatus&) { }

  void on_sample_lost(
      dds::sub::DataReader<RoundTripModule::DataType>&,
      const dds::core::status::SampleLostStatus&) { }

  private:
    dds::pub::DataWriter<RoundTripModule::DataType> &_wr;
    callback_func _f;
};
