/*
//@HEADER
// *****************************************************************************
//
//                           comm_overhead.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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


#include "vt/vrt/collection/balance/model/comm_overhead.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

CommOverhead::CommOverhead(
  std::shared_ptr<balance::LoadModel> base, TimeType in_per_msg_weight,
  TimeType in_per_byte_weight
) : ComposedModel(base),
    per_msg_weight_(in_per_msg_weight),
    per_byte_weight_(in_per_byte_weight)
{ }

void CommOverhead::setLoads(std::vector<LoadMapType> const* proc_load,
			    std::vector<SubphaseLoadMapType> const* proc_subphase_load,
			    std::vector<CommMapType> const* proc_comm) {
  proc_comm_ = proc_comm;
  ComposedModel::setLoads(proc_load, proc_subphase_load, proc_comm);
}

TimeType CommOverhead::getWork(ElementIDType object, PhaseOffset offset) {
  auto work = ComposedModel::getWork(object, offset);

  auto phase = getNumCompletedPhases() + offset.phases;
  auto& comm = proc_comm_->at(phase);

  TimeType overhead = 0.;
  for (auto&& c : comm) {
    // find messages that go off-node and are sent to this object
    if (c.first.offNode() and c.first.toObjTemp() == object) {
      overhead += per_msg_weight_ * c.second.messages;
      overhead += per_byte_weight_ * c.second.bytes;
    }
  }

  if (offset.subphase == PhaseOffset::WHOLE_PHASE) {
    return work + overhead;
  } else {
    // @todo: we don't record comm costs for each subphase---split it proportionally
    auto whole_phase_work = ComposedModel::getWork(object, PhaseOffset{offset.phases, PhaseOffset::WHOLE_PHASE});
    return work + overhead * ( static_cast<double>(work)/whole_phase_work );
  }
}


}}}}
