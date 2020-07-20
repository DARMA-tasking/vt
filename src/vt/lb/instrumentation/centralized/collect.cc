/*
//@HEADER
// *****************************************************************************
//
//                                  collect.cc
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

#include "vt/config.h"
#include "vt/lb/instrumentation/centralized/collect.h"
#include "vt/lb/instrumentation/centralized/collect_msg.h"
#include "vt/lb/lb_types.h"
#include "vt/lb/lb_types_internal.h"
#include "vt/lb/instrumentation/entity.h"
#include "vt/lb/instrumentation/database.h"
#include "vt/collective/collective_alg.h"

#include <unordered_map>
#include <vector>
#include <cassert>

namespace vt { namespace lb { namespace instrumentation {

/*static*/ LBPhaseType CentralCollect::cur_lb_phase_ = fst_phase;
/*static*/ NodeType CentralCollect::collect_root_ = 0;

/*static*/ void CentralCollect::combine(CollectMsg* msg1, CollectMsg* msg2) {
  vtAssert(msg1->phase_ == msg2->phase_, "Phases must be identical");

  // Runtime validity check to ensure that nodes are unique
  #if vt_check_enabled(runtime_checks) || 1
  for (auto&& elm1 : msg1->entries_) {
    for (auto&& elm2 : msg2->entries_) {
      vtAssert(
        elm1.first != elm2.first,
        "CollectMsg combine must have unique entries"
      );
    }
  }
  #endif

  msg1->entries_.insert(msg2->entries_.begin(), msg2->entries_.end());
}

/*static*/ void CentralCollect::collectFinished(
  LBPhaseType const& phase, ProcContainerType const& entries
) {
  vt_debug_print(
    lb, node,
    "collectFinished: phase={}, size={}\n", phase, entries.size()
  );
}

/*static*/ void CentralCollect::centralizedCollect(CollectMsg* msg) {
  if (msg->isRoot()) {
    return collectFinished(msg->phase_, msg->entries_);
  } else {
    CollectMsg* fst_msg = msg;
    CollectMsg* cur_msg = msg->getNext<CollectMsg>();
    while (cur_msg != nullptr) {
      // Combine msgs
      CentralCollect::combine(fst_msg, cur_msg);
      cur_msg = cur_msg->getNext<CollectMsg>();
    }
  }
}

/*static*/ void CentralCollect::reduceCurrentPhase() {
  auto const& phase = CentralCollect::currentPhase();
  CentralCollect::nextPhase();
  return startReduce(phase);
}

/*static*/ MsgSharedPtr<CollectMsg> CentralCollect::collectStats(
  LBPhaseType const& phase
) {
  auto const& node = theContext()->getNode();
  auto msg = makeMessage<CollectMsg>(phase);
  auto node_cont_iter = msg->entries_.find(node);
  vtAssert(
    node_cont_iter == msg->entries_.end(),
    "Entries must not exist for this node"
  );
  msg->entries_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(node),
    std::forward_as_tuple(ContainerType{})
  );
  node_cont_iter = msg->entries_.find(node);
  vtAssert(
    node_cont_iter != msg->entries_.end(),
    "Entries must exist here for this node"
  );
  auto const& entity_list = Entity::entities_;
  for (auto&& elm : entity_list) {
    auto const& entity = elm.first;
    auto const& db = elm.second;
    auto phase_iter = db.phase_timings_.find(phase);
    if (phase_iter != db.phase_timings_.end()) {
      auto msg_entry_iter = node_cont_iter->second.find(entity);
      if (msg_entry_iter == node_cont_iter->second.end()) {
        node_cont_iter->second.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(entity),
          std::forward_as_tuple(EntryListType{})
        );
      }
      msg_entry_iter->second = phase_iter->second;
    }
  }
  return msg;
}

/*static*/ void CentralCollect::startReduce(LBPhaseType const& phase) {
  auto const& root = CentralCollect::collect_root_;
  auto msg = CentralCollect::collectStats(phase);
  theCollective()->global()->
    reduce<CollectMsg, CentralCollect::centralizedCollect>(
      root, msg.get()
    );
}

/*static*/ LBPhaseType CentralCollect::currentPhase() {
  return CentralCollect::cur_lb_phase_;
}

/*static*/ void CentralCollect::nextPhase() {
  CentralCollect::cur_lb_phase_++;
}

}}} /* end namespace vt::lb::instrumentation */
