/*
//@HEADER
// *****************************************************************************
//
//                              per_collection.cc
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

#include "vt/vrt/collection/balance/model/per_collection.h"
#include "vt/vrt/collection/balance/node_lb_data.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

PerCollection::PerCollection(std::shared_ptr<LoadModel> base)
  : ComposedModel(base)
{ }

void PerCollection::addModel(CollectionID proxy, std::shared_ptr<LoadModel> model)
{
  models_[proxy] = model;
}

void PerCollection::setLoads(std::unordered_map<PhaseType, LoadMapType> const* proc_load,
                             std::unordered_map<PhaseType, CommMapType> const* proc_comm) {
  for (auto& m : models_)
    m.second->setLoads(proc_load, proc_comm);
  ComposedModel::setLoads(proc_load, proc_comm);
}

void PerCollection::updateLoads(PhaseType last_completed_phase) {
  for (auto& m : models_)
    m.second->updateLoads(last_completed_phase);
  ComposedModel::updateLoads(last_completed_phase);
}

LoadType
PerCollection::getModeledLoad(ElementIDStruct object, PhaseOffset when) const {
  // See if some specific model has been given for the object in question
  auto mi = models_.find(theNodeLBData()->getCollectionProxyForElement(object));
  if (mi != models_.end())
    return mi->second->getModeledLoad(object, when);

  // Otherwise, default to the given base model
  return ComposedModel::getModeledLoad(object, when);
}

bool PerCollection::hasRawLoad() const {
  // Only return true if all possible paths lead to true
  bool has_raw_load = true;

  for (auto it = models_.begin(); it != models_.end(); ++it) {
    has_raw_load = has_raw_load and it->second->hasRawLoad();
  }

  return has_raw_load and ComposedModel::hasRawLoad();
}

LoadType PerCollection::getRawLoad(ElementIDStruct object, PhaseOffset when) const {
  // See if some specific model has been given for the object in question
  auto mi = models_.find(theNodeLBData()->getCollectionProxyForElement(object));
  if (mi != models_.end())
    return mi->second->getRawLoad(object, when);

  // Otherwise, default to the given base model
  return ComposedModel::getRawLoad(object, when);
}

unsigned int PerCollection::getNumPastPhasesNeeded(unsigned int look_back) const
{
  unsigned int needed = ComposedModel::getNumPastPhasesNeeded(look_back);
  for (auto& m : models_)
    needed = std::max(needed, m.second->getNumPastPhasesNeeded(look_back));
  return needed;
}

}}}}
