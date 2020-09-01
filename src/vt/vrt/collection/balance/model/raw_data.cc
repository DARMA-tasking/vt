/*
//@HEADER
// *****************************************************************************
//
//                           raw_data.cc
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


#include "vt/vrt/collection/balance/model/raw_data.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

void RawData::updateLoads(PhaseType last_completed_phase) {
  last_completed_phase_ = last_completed_phase;
}

void RawData::setLoads(std::unordered_map<PhaseType, LoadMapType> const* proc_load,
                       std::unordered_map<PhaseType, SubphaseLoadMapType> const* proc_subphase_load,
                       std::unordered_map<PhaseType, CommMapType> const* proc_comm)
{
  proc_load_ = proc_load;
  proc_subphase_load_ = proc_subphase_load;
  proc_comm_ = proc_comm;
}

ObjectIterator RawData::begin() {
  return ObjectIterator(proc_load_->at(last_completed_phase_).cbegin());
}

ObjectIterator RawData::end() {
  return ObjectIterator(proc_load_->at(last_completed_phase_).cend());
}

int RawData::getNumObjects() {
  return end() - begin();
}

int RawData::getNumCompletedPhases() {
  return last_completed_phase_ + 1;
}

int RawData::getNumSubphases() {
  const auto& last_phase = proc_subphase_load_->at(last_completed_phase_);
  const auto& an_object = *last_phase.begin();
  const auto& subphases = an_object.second;
  return subphases.size();
}

TimeType RawData::getWork(ElementIDType object, PhaseOffset offset)
{
  vtAssert(offset.phases < 0,
	   "RawData makes no predictions. Compose with NaivePersistence or some longer-range forecasting model as needed");

  auto phase = getNumCompletedPhases() + offset.phases;
  if (offset.subphase == PhaseOffset::WHOLE_PHASE)
    return proc_load_->at(phase).at(object);
  else
    return proc_subphase_load_->at(phase).at(object).at(offset.subphase);
}

int RawData::getNumPastPhasesNeeded(int look_back)
{
  return look_back;
}

}}}}
