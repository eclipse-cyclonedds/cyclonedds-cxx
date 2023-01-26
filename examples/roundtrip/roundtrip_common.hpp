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

  waitset.attach_condition(wsc);
  waitset.attach_condition(rsc);

  std::cout << "# Waiting for readers and writers to match up\n" << std::flush;
  try {

    while (0 == rd.subscription_matched_status().current_count() ||
           0 == wr.publication_matched_status().current_count()) {
      auto conds = waitset.wait(timeout);
      for (const auto & c:conds) {
        if (wsc == c)
          waitset.detach_condition(wsc);
        else if (rsc == c)
          waitset.detach_condition(rsc);
      }
    }

    std::cout << "# Reader and writer have matched.\n" << std::flush;
    return true;
  } catch (const dds::core::TimeoutError &) {
    std::cout << "\nTimeout occurred during matching readers and writers.\n" << std::flush;
  } catch (const dds::core::Exception &e) {
    std::cout << "\nThe following error: \"" << e.what() << "\" was encountered during matching of readers and writers.\n" << std::flush;
  } catch (...) {
    std::cout << "\nA generic error was encountered during matching of readers and writers.\n" << std::flush;
  }
  return false;
}

class RoundTripListener: public dds::sub::NoOpDataReaderListener<RoundTripModule::DataType>
{
  public:
  using callback_func = std::function<bool(dds::sub::DataReader<RoundTripModule::DataType>&, dds::pub::DataWriter<RoundTripModule::DataType>&)>;
  RoundTripListener() = delete;
  RoundTripListener(dds::pub::DataWriter<RoundTripModule::DataType> &wr, const callback_func &f):
    dds::sub::DataReaderListener<RoundTripModule::DataType>(), _wr(wr), _f(f) { ; }

  /*implementation of function overrides*/
  void on_data_available(dds::sub::DataReader<RoundTripModule::DataType>& rd) {
    (void)_f(rd, _wr);
  }

  private:
    dds::pub::DataWriter<RoundTripModule::DataType> &_wr;
    callback_func _f;
};
