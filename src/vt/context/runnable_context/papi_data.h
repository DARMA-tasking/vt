/*
//@HEADER
// *****************************************************************************
//
//                                  papi_data.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_DATA_PAPI_DATA_H
#define INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_DATA_PAPI_DATA_H

#include <papi.h>

namespace vt { namespace ctx {

/**
 * \struct PAPIData
 *
 * \brief Structure for storing Performance API (PAPI) related data structures
 */
struct PAPIData {
  int EventSet = PAPI_NULL;
  int retval = 0;
  uint64_t start_real_cycles = 0, end_real_cycles = 0, start_real_usec = 0, end_real_usec = 0;
  uint64_t start_virt_cycles = 0, end_virt_cycles = 0, start_virt_usec = 0, end_virt_usec = 0;
  std::vector<std::string> native_events = {
    "fp_arith_inst_retired.scalar_double",
    "cache-misses",
    "page-faults"
  };
  uint64_t values[4];

  PAPIData()
  {
    std::fill(std::begin(values), std::end(values), 0);
    /* Create an EventSet */
    retval = PAPI_create_eventset(&(EventSet));
    if (retval != PAPI_OK)
        handle_error("PAPIData constructor: Couldn't create an event set: ");

    /* Add Total Instructions to our EventSet,
    must be done such that we can call PAPI_set_multiplex */
    int native = 0x0;
    retval = PAPI_event_name_to_code("instructions", &native);
    retval = PAPI_add_event(EventSet, native);
    if (retval != PAPI_OK)
        handle_error("PAPIData constructor: Couldn't add instructions to event set: ");

    /* Convert the EventSet to a multiplexed EventSet */
    retval = PAPI_set_multiplex(EventSet);
    if (retval != PAPI_OK)
        handle_error("PAPIData constructor: Couldn't convert event set to multiplexed: ");

    for (const auto& event_name : native_events) {
      native = 0x0;
      retval = PAPI_event_name_to_code(event_name.c_str(), &native);
      if (retval != PAPI_OK) {
        printf("PAPIData constructor: Couldn't event_name_to_code for %s: PAPI error %d: %s\n",event_name.c_str(), retval, PAPI_strerror(retval));
        exit(1);
      }
      retval = PAPI_add_event(EventSet, native);
      if (retval != PAPI_OK) {
        printf("PAPIData constructor: Couldn't add %s to the PAPI Event Set: PAPI error %d: %s\n",event_name.c_str(), retval, PAPI_strerror(retval));
        exit(1);
      }
    }
    // we added instructions on its own to the event set for multiplex initialization purposes;
    // here we add it to the list of measured events for later usage
    native_events.insert(native_events.begin(), "instructions");
  }

  void handle_error (std::string info) const
  {
    printf("%s: PAPI error %d: %s\n", info.c_str(), retval, PAPI_strerror(retval));
    exit(1);
  }

  void start()
  {
    retval = PAPI_start(EventSet);
    if (retval != PAPI_OK)
      handle_error("PAPIData start: Starting counting events in the Event Set: ");

    start_real_cycles = PAPI_get_real_cyc();
    start_real_usec = PAPI_get_real_usec();
    start_virt_cycles = PAPI_get_virt_cyc();
    start_virt_usec = PAPI_get_virt_usec();
  }

  void stop()
  {
    retval = PAPI_stop(EventSet, reinterpret_cast<long long*>(values));
    if (retval != PAPI_OK)
      handle_error("PAPIData stop: Stopping the counting of events in the Event Set: ");

    end_real_cycles = PAPI_get_real_cyc();
    end_real_usec = PAPI_get_real_usec();
    end_virt_cycles = PAPI_get_virt_cyc();
    end_virt_usec = PAPI_get_virt_usec();
  }


};

}} /* end namespace vt::ctx */

#endif /*INCLUDED_VT_CONTEXT_RUNNABLE_CONTEXT_LB_DATA_PAPI_DATA_H*/
