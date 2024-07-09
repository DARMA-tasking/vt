/*
//@HEADER
// *****************************************************************************
//
//                                  lb_data.cc
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

#include "vt/context/runnable_context/lb_data.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace ctx {

void LBData::start(TimeType time) {
  // record start time
  if (should_instrument_) {
    lb_data_->start(time);
  }

  /* -- PAPI START -- */
  /* Start counting events in the Event Set */
  papi_retval_ = PAPI_start(EventSet_);
  if (papi_retval_ != PAPI_OK)
    handle_papi_error(papi_retval_, "LBData start: Starting counting events in the Event Set: ");

  start_real_cycles_ = PAPI_get_real_cyc();
  start_real_usec_ = PAPI_get_real_usec();
  start_virt_cycles_ = PAPI_get_virt_cyc();
  start_virt_usec_ = PAPI_get_virt_usec();

  /* ---------------- */
}

void LBData::finish(TimeType time) {
  // record end time
  if (should_instrument_) {
    lb_data_->stop(time);
  }

  /* -- PAPI STOP -- */
  /* Stop the counting of events in the Event Set */
  papi_retval_ = PAPI_stop(EventSet_, papi_values_);
  if (papi_retval_ != PAPI_OK)
    handle_papi_error(papi_retval_, "LBData finish: Stoping the counting of events in the Event Set: ");

  end_real_cycles_ = PAPI_get_real_cyc();
  end_real_usec_ = PAPI_get_real_usec();
  end_virt_cycles_ = PAPI_get_virt_cyc();
  end_virt_usec_ = PAPI_get_virt_usec();

  /* -------------- */
}

void LBData::send(elm::ElementIDStruct dest, MsgSizeType bytes) {
  if (should_instrument_) {
    lb_data_->sendToEntity(dest, cur_elm_id_, bytes);
  }
}

void LBData::suspend(TimeType time) {
  finish(time);
}

void LBData::resume(TimeType time) {
  start(time);
}

typename LBData::ElementIDStruct const& LBData::getCurrentElementID() const {
  return cur_elm_id_;
}

std::unordered_map<std::string, double> LBData::getPAPIMetrics() const {
  std::unordered_map<std::string, double> papi_metrics = {};
  for (size_t i = 0; i < native_events_.size(); i++) {
    papi_metrics[native_events_[i]] = papi_values_[i];
  }
  return papi_metrics;
}

}} /* end namespace vt::ctx */
