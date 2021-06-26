/*
//@HEADER
// *****************************************************************************
//
//                              composed_model.cc
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

#include "vt/vrt/collection/balance/model/composed_model.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

void ComposedModel::setLoads(std::unordered_map<PhaseType, LoadMapType> const* proc_load,
                             std::unordered_map<PhaseType, SubphaseLoadMapType> const* proc_subphase_load,
                             std::unordered_map<PhaseType, CommMapType> const* proc_comm) {
  base_->setLoads(proc_load, proc_subphase_load, proc_comm);
}

void ComposedModel::updateLoads(PhaseType last_completed_phase) {
  base_->updateLoads(last_completed_phase);
}

TimeType ComposedModel::getWork(ElementIDStruct object, PhaseOffset when) {
  return base_->getWork(object, when);
}

unsigned int ComposedModel::getNumPastPhasesNeeded(unsigned int look_back)
{
  return base_->getNumPastPhasesNeeded(look_back);
}

ObjectIterator ComposedModel::begin() {
  return base_->begin();
}

ObjectIterator ComposedModel::end() {
  return base_->end();
}

int ComposedModel::getNumObjects() {
  return base_->getNumObjects();
}

unsigned int ComposedModel::getNumCompletedPhases() {
  return base_->getNumCompletedPhases();
}

int ComposedModel::getNumSubphases() {
  return base_->getNumSubphases();
}

}}}}
