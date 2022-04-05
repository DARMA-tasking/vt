/*
//@HEADER
// *****************************************************************************
//
//                           proposed_reassignment.cc
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

#include "vt/vrt/collection/balance/model/proposed_reassignment.h"
#include "vt/context/context.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

ProposedReassignment::ProposedReassignment(
  std::shared_ptr<balance::LoadModel> base,
  std::shared_ptr<const Reassignment> reassignment
) : ComposedModel(base)
  , reassignment_(reassignment)
{
  vtAssert(reassignment_->node_ == vt::theContext()->getNode(),
           "ProposedReassignment model needs to be applied to the present node's data");

  // Check invariants?

  // depart should be a subset of present

  // subtract depart to allow for self-migration
  // arrive ^ (present \ depart) == 0
}

ObjectIterator ProposedReassignment::begin()
{
  return {
    std::make_unique<ConcatenatedIterator>(
      ObjectIterator{
        std::make_unique<DualLoadMapObjectIterator>(
          reassignment_->arrive_.begin(), reassignment_->arrive_.end()
        )
     },
     ObjectIterator{
       std::make_unique<FilterIterator>(
         ComposedModel::begin(),
         [this](ElementIDStruct elm) {
           return reassignment_->depart_.find(elm) == reassignment_->depart_.end();
         }
       )
    }
  )};
}

int ProposedReassignment::getNumObjects()
{
  int base = ComposedModel::getNumObjects();
  int departing = reassignment_->depart_.size();
  int arriving = reassignment_->arrive_.size();

  // This would handle self-migration without a problem
  return base - departing + arriving;
}

TimeType
ProposedReassignment::getLoadMetric(ElementIDStruct object, PhaseOffset when) {
  auto a = reassignment_->arrive_.find(object);
  if (a != reassignment_->arrive_.end()) {
    return std::get<0>(a->second).get(when);
  }

  // Check this *after* arrivals to handle hypothetical self-migration
  vtAssert(reassignment_->depart_.find(object) == reassignment_->depart_.end(),
           "Departing object should not appear as a load query subject");

  return ComposedModel::getLoadMetric(object, when);
}

TimeType ProposedReassignment::getRawLoad(ElementIDStruct object, PhaseOffset when)
{
  auto a = reassignment_->arrive_.find(object);
  if (a != reassignment_->arrive_.end()) {
    return std::get<1>(a->second).get(when);
  }

  // Check this *after* arrivals to handle hypothetical self-migration
  vtAssert(reassignment_->depart_.find(object) == reassignment_->depart_.end(),
           "Departing object should not appear as a load query subject");

  return ComposedModel::getRawLoad(object, when);
}

}}}}
