/*
//@HEADER
// *****************************************************************************
//
//                            collection_chain_set.h
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

#if !defined INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_SET_H
#define INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_SET_H

#include "vt/config.h"
#include "vt/messaging/dependent_send_chain.h"

#include <unordered_map>
#include <unordered_set>

namespace vt { namespace messaging {

/** \file */

/**
 * \enum ChainSetLayout
 *
 * \brief Used to specify the layout for automatically managing dependency
 * chains for a given collection.
 */
enum ChainSetLayout {
  Local,                     /**< Track dependencies where element is located */
  Home                       /**< Track dependencies on the home node */
};

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
template <typename Index>
class CollectionChainSet final {
  public:
  CollectionChainSet() = default;
  CollectionChainSet(const CollectionChainSet&) = delete;
  CollectionChainSet(CollectionChainSet&&) = delete;

  /**
   * \brief Construct with a collection proxy to track indices on each node
   * depending on the layout specified. Tracking dependencies on the local node
   * may be better for performance as it may reduce the number of messages that
   * must be sent.
   *
   * \note This constructor is a collective invocation.
   *
   * \param[in] proxy the collection proxy
   * \param[in] layout the location the chain set tracks each element
   */
  template <typename ProxyT, typename IndexT = typename ProxyT::IndexType>
  explicit CollectionChainSet(ProxyT proxy, ChainSetLayout layout = Local);

  ~CollectionChainSet() {
    if (deallocator_) {
      deallocator_();
    }
  }

private:
  /**
   * \internal \struct IdxMsg
   *
   * \brief Message that contains an index sent to remove or add remotely
   */
  struct IdxMsg : vt::Message {
    explicit IdxMsg(Index in_idx) : idx_(in_idx) {}
    Index idx_;
  };

  /**
   * \internal brief Add an index remotely
   *
   * \param[in] msg index message
   */
  void addIndexHan(IdxMsg* msg) {
    addIndex(msg->idx_);
  }

  /**
   * \internal \brief Remove an index remotely
   *
   * \param[in] msg index message
   */
  void removeIndexHan(IdxMsg* msg) {
    removeIndex(msg->idx_);
  }

public:
  /**
   * \brief Add an index to the set
   *
   * Creates a new, empty \c DependentSendChain for the given index
   *
   * \param[in] idx the index to add
   */
  void addIndex(Index idx) {
    vtAssert(
      chains_.find(idx) == chains_.end(),
      "Cannot add an already-present chain");
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
    vtAssert(
      iter->second.isTerminated(), "Cannot remove a chain with pending work");

    chains_.erase(iter);
  }

  /**
   * \brief The next step to execute on all the chain indices in this
   * collection chain set
   *
   * Goes through every chain index and enqueues the action at the end of the
   * current chain when the preceding steps terminate. Creates a new rooted
   * epoch for this step to contain/track completion of all the causally related
   * messages.
   *
   * \param[in] label Label for the epoch created for debugging
   * \param[in] step_action The action to perform as a function that returns a
   * \c PendingSend
   */
  void nextStep(
    std::string const& label, std::function<PendingSend(Index)> step_action) {
    for (auto& entry : chains_) {
      auto& idx = entry.first;
      auto& chain = entry.second;

      // The parameter `true` here tells VT to use an efficient rooted DS-epoch
      // by default. This can still be overridden by command-line flags
      EpochType new_epoch =
        theTerm()->makeEpochRooted(label, term::UseDS{true});
      vt::theMsg()->pushEpoch(new_epoch);

      chain.add(new_epoch, step_action(idx));

      vt::theMsg()->popEpoch(new_epoch);
      theTerm()->finishedEpoch(new_epoch);
    }
  }

  /**
   * \brief The next step to execute on all the chain indices in this
   * collection chain set
   *
   * Goes through every chain index and enqueues the action at the end of the
   * current chain when the preceding steps terminate. Creates a new rooted
   * epoch for this step to contain/track completion of all the causally related
   * messages.
   *
   * \param[in] step_action The action to perform as a function that returns a
   * \c PendingSend
   */
  void nextStep(std::function<PendingSend(Index)> step_action) {
    return nextStep("", step_action);
  }

  /**
   * \brief The next collective step to execute for each index that is added
   * to the CollectionChainSet on each node.
   *
   * Should be used for steps with internal recursive communication and global
   * inter-dependence. Creates a global (on the communicator), collective epoch
   * to track all the casually related messages and collectively wait for
   * termination of all of the recursive sends.
   *
   * \param[in] label Label for the epoch created for debugging
   * \param[in] step_action the next step to execute, returning a \c PendingSend
   */
  void nextStepCollective(
    std::string const& label, std::function<PendingSend(Index)> step_action) {
    auto epoch = theTerm()->makeEpochCollective(label);
    vt::theMsg()->pushEpoch(epoch);

    for (auto& entry : chains_) {
      auto& idx = entry.first;
      auto& chain = entry.second;
      chain.add(epoch, step_action(idx));
    }

    vt::theMsg()->popEpoch(epoch);
    theTerm()->finishedEpoch(epoch);
  }

  /**
   * \brief The next collective step to execute for each index that is added
   * to the CollectionChainSet on each node.
   *
   * Should be used for steps with internal recursive communication and global
   * inter-dependence. Creates a global (on the communicator), collective epoch
   * to track all the casually related messages and collectively wait for
   * termination of all of the recursive sends.
   *
   * \param[in] step_action the next step to execute, returning a \c PendingSend
   */
  void nextStepCollective(std::function<PendingSend(Index)> step_action) {
    return nextStepCollective("", step_action);
  }

  /**
   * \brief The next collective step of both CollectionChainSets
   * to execute over all shared indices of the CollectionChainSets over all
   * nodes.
   *
   * This function ensures that the step is dependent on the previous step
   * of both chainsets a and b. Additionally any additional steps in each
   * chainset will occur after the merged step.
   *
   * \pre Each index in CollectionChainset a must exist in CollectionChainset b
   *
   * \param[in] a the first chainset
   * \param[in] b the second chainset
   * \param[in] step_action the next step to be executed, dependent on the
   *            previous step of chainsets a and b
   */
  static void mergeStepCollective(
    CollectionChainSet& a, CollectionChainSet& b,
    std::function<PendingSend(Index)> step_action) {
    mergeStepCollective("", a, b, step_action);
  }

  /**
   * \brief The next collective step of both CollectionChainSets
   * to execute over all shared indices of the CollectionChainSets over all
   * nodes.
   *
   * This function ensures that the step is dependent on the previous step
   * of both chainsets a and b. Additionally any additional steps in each
   * chainset will occur after the merged step.
   *
   * \pre Each index in CollectionChainset a must exist in CollectionChainset b
   *
   * \param[in] label the label for the step
   * \param[in] a the first chainset
   * \param[in] b the second chainset
   * \param[in] step_action the next step to be executed, dependent on the
   *            previous step of chainsets a and b
   */
  static void mergeStepCollective(
    std::string const& label, CollectionChainSet& a, CollectionChainSet& b,
    std::function<PendingSend(Index)> step_action) {
    auto epoch = theTerm()->makeEpochCollective(label);
    vt::theMsg()->pushEpoch(epoch);

    for (auto& entry : a.chains_) {
      auto& idx = entry.first;
      auto& chaina = entry.second;
      auto chainb_pos = b.chains_.find(entry.first);
      vtAssert(
        chainb_pos != b.chains_.end(),
        fmt::format("index {} must be present in chainset b", entry.first));

      auto& chainb = chainb_pos->second;
      DependentSendChain::mergeChainStep(
        chaina, chainb, epoch, step_action(idx));
    }

    vt::theMsg()->popEpoch(epoch);
    theTerm()->finishedEpoch(epoch);
  }

  /**
   * \brief Indicate that the current phase is complete. Resets the state on
   * each \c DependentSendChain
   */
  void phaseDone() {
    for (auto& entry : chains_) {
      entry.second.done();
    }
  }

  /**
   * \brief Get the set of indices registered with this chain set
   */
  std::unordered_set<Index> getSet() {
    std::unordered_set<Index> index_set;
    for (auto& entry : chains_) {
      index_set.emplace(entry.first);
    }
    return index_set;
  }

  /**
   * \brief Run a lambda immediately on each element in the index set
   */
  void foreach (std::function<void(Index)> fn) {
    for (auto& entry : chains_) {
      fn(entry.first);
    }
  }

private:
  /// Set of \c DependentSendChain managed on this node for indices
  std::unordered_map<Index, DependentSendChain> chains_;
  /// Deallocator that type erases element listener de-registration
  std::function<void()> deallocator_;
};

}} /* end namespace vt::messaging */

#include "vt/messaging/collection_chain_set.impl.h"

#endif /*INCLUDED_VT_MESSAGING_COLLECTION_CHAIN_SET_H*/
