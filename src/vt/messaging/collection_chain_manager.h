/*
//@HEADER
// ************************************************************************
//
//                   collection_chain_manager.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_MANAGER_H
#define INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_MANAGER_H

#include "vt/config.h"
#include "vt/messaging/dependent_send_chain.h"

#include <unordered_map>

namespace vt { namespace messaging {

template <class Index>
class CollectionChainManager final {
 public:
  CollectionChainManager() = default;
  CollectionChainManager(const CollectionChainManager&) = delete;
  CollectionChainManager(CollectionChainManager&&) = delete;

  void addIndex(Index idx) {
    chains.emplace(idx, {});
  }

  void nextStep(std::function<PendingSend(Index)> step_action) {
    for (auto &entry : chains_) {
      auto& idx = entry.first;
      auto& chain = entry.second;
      chain.add(step_action(idx));
    }
  }

  // for a step with internal recursive communication and global inter-dependence
  void nextStepCollective(std::function<PendingSend(Index)> step_action) {
    auto epoch = theTerm()->makeCollectiveEpoch();

    for (auto &entry : chains_) {
      auto& idx = entry.first;
      auto& chain = entry.second;
      chain.add(epoch, step_action(idx));
    }

    theTerm()->finishedEpoch(epoch);
  }

 private:
  std::unordered_map<Index, DependentSendChain> chains_;
};

}} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_MANAGER_H*/
