/*
//@HEADER
// *****************************************************************************
//
//                            collection_chain_set.h
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

#if !defined INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_SET_H
#define INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_SET_H

#include "vt/config.h"
#include "vt/messaging/dependent_send_chain.h"

#include <unordered_map>

namespace vt { namespace messaging {

template <class Index>
class CollectionChainSet final {
 public:
  CollectionChainSet() = default;
  CollectionChainSet(const CollectionChainSet&) = delete;
  CollectionChainSet(CollectionChainSet&&) = delete;

  void addIndex(Index idx) {
    vtAssert(chains_.find(idx) == chains_.end(), "Cannot add an already-present chain");
    chains_[idx] = DependentSendChain();
  }

  void removeIndex(Index idx) {
    auto iter = chains_.find(idx);
    vtAssert(iter != chains_.end(), "Cannot remove a non-present chain");
    vtAssert(iter->second.isTerminated(), "Cannot remove a chain with pending work");

    chains_.erase(iter);
  }

  void nextStep(std::string label, std::function<PendingSend(Index)> step_action) {
    for (auto &entry : chains_) {
      auto& idx = entry.first;
      auto& chain = entry.second;

      // The parameter `true` here tells VT to use an efficient rooted DS-epoch
      // by default. This can still be overridden by command-line flags
      EpochType new_epoch = theTerm()->makeEpochRooted(label, term::UseDS{true});
      vt::theMsg()->pushEpoch(new_epoch);

      chain.add(new_epoch, step_action(idx));

      vt::theMsg()->popEpoch(new_epoch);
      theTerm()->finishedEpoch(new_epoch);
    }
  }

  void nextStep(std::function<PendingSend(Index)> step_action) {
    return nextStep("", step_action);
  }

#if 0
  void nextStep(std::function<Action(Index)> step_action) {
    for (auto &entry : chains_) {
      auto& idx = entry.first;
      auto& chain = entry.second;
      chain.add(step_action(idx));
    }
  }

  void nextStepConcurrent(std::vector<std::function<PendingSend(Index)>> step_actions) {
    for (auto &entry : chains_) {
      auto& idx = entry.first;
      auto& chain = entry.second;
      chain.add(step_actions[0](idx));
      for (int i = 1; i < step_actions.size(); ++i)
        chain.addConcurrent(step_actions[i](idx));
    }
  }
#endif


  void nextStepCollective(
    std::string label, std::function<PendingSend(Index)> step_action
  ) {
    auto epoch = theTerm()->makeEpochCollective(label);
    vt::theMsg()->pushEpoch(epoch);

    for (auto &entry : chains_) {
      auto& idx = entry.first;
      auto& chain = entry.second;
      chain.add(epoch, step_action(idx));
    }

    vt::theMsg()->popEpoch(epoch);
    theTerm()->finishedEpoch(epoch);
  }

  void nextStepCollective(std::function<PendingSend(Index)> step_action) {
    return nextStepCollective("", step_action);
  }

  void phaseDone() {
    for (auto &entry : chains_) {
      entry.second.done();
    }
  }

 private:
  std::unordered_map<Index, DependentSendChain> chains_;
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_SET_H*/
