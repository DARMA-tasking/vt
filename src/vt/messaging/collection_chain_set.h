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

/** \file */

/**
 * \struct CollectionChainSet collection_chain_set.h vt/messaging/collection_chain_set.h
 *
 * \brief A set of chains to maintain a sequence for a set of collection
 * elements that may be local or remote.
 *
 * Manages a set of chains (sequences) on a set of elements where this is
 * constructed. It may sequence objects that reside on this node or a remote
 * node (either is valid). This enables the user to enqueue sequences of tasks
 * on each object and coordinate data dependencies.
 */
template <class Index>
class CollectionChainSet final {
 public:
  CollectionChainSet() = default;
  CollectionChainSet(const CollectionChainSet&) = delete;
  CollectionChainSet(CollectionChainSet&&) = delete;

  /**
   * \brief Add an index to the set
   *
   * Creates a new, empty \c DependentSendChain for the given index
   *
   * \param[in] idx the index to add
   */
  void addIndex(Index idx) {
    vtAssert(chains_.find(idx) == chains_.end(), "Cannot add an already-present chain");
    chains_[idx] = DependentSendChain();
  }

  /**
   * \brief Remove an index from the set
   *
   * The chain for that index being removed must be terminated (at the end of
   * the chain). This may be called during migration or when a collection
   * element's control sequencing is no longer being tracked (on this node) with
   * this chain set
   *
   * \param[in] idx the index to remove
   */
  void removeIndex(Index idx) {
    auto iter = chains_.find(idx);
    vtAssert(iter != chains_.end(), "Cannot remove a non-present chain");
    vtAssert(iter->second.isTerminated(), "Cannot remove a chain with pending work");

    chains_.erase(iter);
  }

  /**
   * \brief The next step to execute on all the chains resident in this
   * collection chain set
   *
   * Goes through every resident chain and enqueues the action at the end of the
   * current chain when the preceding steps terminate. Creates a new rooted
   * epoch for this step to contain/track completion of all the causally related
   * messages.
   *
   * \param[in] label Label for the epoch created for debugging
   * \param[in] step_action The action to perform as a function that returns a
   * \c PendingSend
   */
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

  /**
   * \brief The next step to execute on all the chains resident in this
   * collection chain set
   *
   * \param[in] step_action The action to perform as a function that returns a
   * \c PendingSend
   */
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

  /**
   * \brief The next collective step to execute across all resident elements
   * across all nodes.
   *
   * Should be used for steps with internal recursive communication and global
   * inter-dependence. Creates a global (on the communicator), collective epoch
   * to track all the casually related messages and collectively wait for
   * termination of all of the recursive sends..
   *
   * \param[in] label Label for the epoch created for debugging
   * \param[in] step_action the next step to execute, returning a \c PendingSend
   */
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

  /**
   * \brief The next collective step to execute across all resident elements
   * across all nodes.
   *
   * \param[in] step_action the next step to execute, returning a \c PendingSend
   */
  void nextStepCollective(std::function<PendingSend(Index)> step_action) {
    return nextStepCollective("", step_action);
  }

  /**
   * \brief Indicate that the current phase is complete. Resets the state on
   * each \c DependentSendChain
   */
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
