/*
//@HEADER
// *****************************************************************************
//
//                                 papi_data.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/context/runnable_context/papi_data.h"

namespace vt { namespace ctx {

PAPIData::PAPIData()
  : EventSet(PAPI_NULL), retval(0), 
    start_real_cycles(0), end_real_cycles(0), start_real_usec(0), end_real_usec(0),
    start_virt_cycles(0), end_virt_cycles(0), start_virt_usec(0), end_virt_usec(0) 
{
  const char* env_p = getenv("VT_EVENTS");

  if (env_p == nullptr) {
    events["PAPI_TOT_INS"] = 0;
  } else {
    std::string env_str(env_p);
    std::stringstream ss(env_str);
    std::string item;
    while (std::getline(ss, item, ',')) {
      events[item] = 0;
    }
  }

  retval = PAPI_create_eventset(&EventSet);
  if (retval != PAPI_OK)
    handle_error("PAPIData Constructor: Couldn't create an event set: ");

  // Add instructions before multiplexing (needed https://github.com/icl-utk-edu/papi/wiki/PAPI-Multiplexing)
  retval = PAPI_add_event(EventSet, PAPI_TOT_INS);
  if (retval != PAPI_OK)
    handle_error("PAPIData Constructor: Couldn't add instructions to event set: ");

  retval = PAPI_set_multiplex(EventSet);
  if (retval != PAPI_OK) {
    handle_error("PAPIData Constructor: Couldn't convert event set to multiplexed: ");
  }

  for (const auto& event : events) {
    // Skip adding instructions to the event set since it was already
    // added before making the event set multiplexed.
    if (event.first == "PAPI_TOT_INS") {
      continue;
    }
    int native = 0x0;
    retval = PAPI_event_name_to_code(event.first.c_str(), &native);
    if (retval != PAPI_OK) {
      handle_error(fmt::format("Couldn't event_name_to_code for {}: ", event.first.c_str()));
    }
    retval = PAPI_add_event(EventSet, native);
    if (retval != PAPI_OK) {
      handle_error(fmt::format("Couldn't add {} to the PAPI Event Set: ", event.first.c_str()));
    }
  }
}

void PAPIData::handle_error(const std::string &info) const {
  vtAbort(fmt::format("{}: PAPI error {}: {}\n", info, retval, PAPI_strerror(retval)));
}

void PAPIData::start() {
  retval = PAPI_start(EventSet);
  if (retval != PAPI_OK)
    handle_error("PAPIData start: Starting counting events in the Event Set: ");

  start_real_cycles = PAPI_get_real_cyc();
  start_real_usec = PAPI_get_real_usec();
  start_virt_cycles = PAPI_get_virt_cyc();
  start_virt_usec = PAPI_get_virt_usec();
}

void PAPIData::stop() {
  std::vector<long long> aligned_values(events.size(), 0);

  retval = PAPI_stop(EventSet, aligned_values.data());
  if (retval != PAPI_OK) {
    handle_error("PAPIData stop: Stopping the counting of events in the Event Set: ");
  }

  size_t i = 0;
  for (auto& event : events) {
    long long papi_value = aligned_values[i++];

    if (papi_value < 0) {
      vtAbort(fmt::format("Unexpected negative value for event {}: {}", event.first, papi_value));
    }

    event.second = static_cast<uint64_t>(papi_value);
  }

  end_real_cycles = PAPI_get_real_cyc();
  end_real_usec = PAPI_get_real_usec();
  end_virt_cycles = PAPI_get_virt_cyc();
  end_virt_usec = PAPI_get_virt_usec();
}
}} /* end namespace vt::ctx */
