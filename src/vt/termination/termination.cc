/*
//@HEADER
// ************************************************************************
//
//                          termination.cc
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


#include "vt/config.h"
#include "vt/termination/termination.h"
#include "vt/termination/term_common.h"
#include "vt/termination/term_window.h"
#include "vt/messaging/active.h"
#include "vt/collective/collective_ops.h"
#include "vt/scheduler/scheduler.h"
#include "vt/epoch/epoch_headers.h"
#include "vt/termination/dijkstra-scholten/ds_headers.h"
#include "vt/termination/dijkstra-scholten/comm.h"
#include "vt/termination/dijkstra-scholten/ds.h"
#include "vt/configs/arguments/args.h"
#include "vt/configs/debug/debug_colorize.h"

#include <memory>

namespace vt { namespace term {

TerminationDetector::TerminationDetector()
  : collective::tree::Tree(collective::tree::tree_cons_tag_t),
  any_epoch_state_(any_epoch_sentinel, false, true, getNumChildren()),
  epoch_coll_(std::make_unique<EpochWindow>())
{ }

/*static*/ void TerminationDetector::makeRootedHandler(TermMsg* msg) {
  theTerm()->makeRootedHan(msg->new_epoch, false);
}

/*static*/ void
TerminationDetector::propagateEpochHandler(TermCounterMsg* msg) {
  theTerm()->propagateEpochExternal(msg->epoch, msg->prod, msg->cons);
}

/*static*/ void TerminationDetector::epochTerminatedHandler(TermMsg* msg) {
  theTerm()->epochTerminated(msg->new_epoch);
}

/*static*/ void TerminationDetector::epochContinueHandler(TermMsg* msg) {
  theTerm()->epochContinue(msg->new_epoch, msg->wave);
}

/*static*/ void TerminationDetector::inquireEpochTerminated(
  TermTerminatedMsg* msg
) {
  theTerm()->inquireTerminated(msg->getEpoch(),msg->getFromNode());
}

/*static*/ void TerminationDetector::replyEpochTerminated(
  TermTerminatedReplyMsg* msg
) {
  theTerm()->replyTerminated(msg->getEpoch(),msg->isTerminated());
}

EpochType TerminationDetector::getArchetype(EpochType const& epoch) const {
  auto epoch_arch = epoch;
  epoch::EpochManip::setSeq(epoch_arch,0);
  return epoch_arch;
}

EpochWindow* TerminationDetector::getWindow(EpochType const& epoch) {
  auto const is_rooted = epoch::EpochManip::isRooted(epoch);
  if (is_rooted) {
    auto const& arch_epoch = getArchetype(epoch);
    auto iter = epoch_arch_.find(arch_epoch);
    if (iter == epoch_arch_.end()) {
      epoch_arch_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(arch_epoch),
        std::forward_as_tuple(std::make_unique<EpochWindow>(true))
      );
      iter = epoch_arch_.find(arch_epoch);
      iter->second->initialize(epoch);
    }
    return iter->second.get();
  } else {
    vtAssertExpr(epoch_coll_ != nullptr);
    return epoch_coll_.get();
  }
}

TermCounterType TerminationDetector::getNumUnits() const {
  return any_epoch_state_.g_cons2;
}

void TerminationDetector::setLocalTerminated(
  bool const local_terminated, bool const no_local
) {
  debug_print(
    term, node,
    "setLocalTerminated: is_term={}, no_local_workers={}\n",
    print_bool(local_terminated), print_bool(no_local)
  );

  any_epoch_state_.notifyLocalTerminated(local_terminated);

  if (local_terminated && !no_local) {
    theTerm()->maybePropagate();
  }
}

TerminationDetector::TermStateType&
TerminationDetector::findOrCreateState(EpochType const& epoch, bool is_ready) {
  auto const& num_children_ = getNumChildren();

  bool const local_term = is_ready;
  bool const epoch_active = is_ready;

  vtAssert(epoch != any_epoch_sentinel, "Should not be any epoch");

  auto epoch_iter = epoch_state_.find(epoch);

  if (epoch_iter == epoch_state_.end()) {
    epoch_state_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(epoch),
      std::forward_as_tuple(
        TermStateType{epoch, epoch_active, local_term, num_children_}
      )
    );
    epoch_iter = epoch_state_.find(epoch);
  }

  return epoch_iter->second;
}

void TerminationDetector::produceConsumeState(
  TermStateType& state, TermCounterType const num_units, bool produce,
  NodeType node
) {
  auto& counter = produce ? state.l_prod : state.l_cons;
  counter += num_units;

  debug_print_verbose(
    term, node,
    "produceConsumeState: epoch={:x}, event_count={}, l_prod={}, l_cons={}, "
    "num_units={}, produce={}, node={}\n",
    state.getEpoch(), state.getRecvChildCount(), state.l_prod, state.l_cons, num_units,
    print_bool(produce), node
  );

  if (state.readySubmitParent()) {
    propagateEpoch(state);
  }
}

TerminationDetector::TermStateDSType*
TerminationDetector::getDSTerm(EpochType epoch, bool is_root) {
  debug_print_verbose(
    termds, node,
    "getDSTerm: epoch={:x}, is_rooted={}, is_ds={}\n",
    epoch, isRooted(epoch), isDS(epoch)
  );
  if (isDS(epoch)) {
    auto iter = term_.find(epoch);
    if (iter == term_.end()) {
      auto const this_node = theContext()->getNode();
      term_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(epoch),
        std::forward_as_tuple(
          TerminatorType{epoch,is_root,this_node}
        )
      );
      iter = term_.find(epoch);
      vtAssert(iter != term_.end(), "Must exist");
    }
    return &iter->second;
  } else {
    return nullptr;
  }
}

void TerminationDetector::produceConsume(
  EpochType epoch, TermCounterType num_units, bool produce, NodeType node
) {
  debug_print(
    term, node,
    "produceConsume: epoch={:x}, rooted={}, ds={}, count={}, produce={}, "
    "node={}\n",
    epoch, isRooted(epoch), isDS(epoch), num_units, produce, node
  );

  // If a node is not passed, use the current node (self-prod/cons)
  if (node == uninitialized_destination) {
    node = theContext()->getNode();
  }

  produceConsumeState(any_epoch_state_, num_units, produce, node);

  if (epoch != any_epoch_sentinel) {
    if (isDS(epoch)) {
      auto ds_term = getDSTerm(epoch);
      if (produce) {
        ds_term->msgSent(node,num_units);
      } else {
        ds_term->msgProcessed(node,num_units);
      }
    } else {
      auto& state = findOrCreateState(epoch, false);
      produceConsumeState(state, num_units, produce, node);
    }
  }
}

void TerminationDetector::maybePropagate() {
  bool const ready = any_epoch_state_.readySubmitParent();

  if (ready) {
    propagateEpoch(any_epoch_state_);
  }

  for (auto&& iter = epoch_state_.begin(); iter != epoch_state_.end(); ) {
    auto &state = iter->second;
    bool clean_epoch = false;
    if (state.readySubmitParent()) {
      propagateEpoch(state);
      if (state.isTerminated() && iter->first != any_epoch_sentinel) {
        clean_epoch = true;
      }
    }
    if (clean_epoch) {
      iter = epoch_state_.erase(iter);
    } else {
      ++iter;
    }
  }
}

void TerminationDetector::propagateEpochExternalState(
  TermStateType& state, TermCounterType const& prod, TermCounterType const& cons
) {
  debug_print_verbose(
    term, node,
    "propagateEpochExternalState: epoch={:x}, prod={}, cons={}, "
    "event_count={}, wave={}\n",
    state.getEpoch(), prod, cons, state.getRecvChildCount(), state.getCurWave()
  );

  state.g_prod1 += prod;
  state.g_cons1 += cons;

  state.notifyChildReceive();

  if (state.readySubmitParent()) {
    propagateEpoch(state);
  }
}

void TerminationDetector::propagateEpochExternal(
  EpochType const& epoch, TermCounterType const& prod,
  TermCounterType const& cons
) {
  debug_print_verbose(
    term, node,
    "propagateEpochExternal: epoch={:x}, prod={}, cons={}\n",
    epoch, prod, cons
  );

  if (epoch == any_epoch_sentinel) {
    propagateEpochExternalState(any_epoch_state_, prod, cons);
  } else {
    auto& state = findOrCreateState(epoch, false);
    propagateEpochExternalState(state, prod, cons);
  }
}

void TerminationDetector::resetGlobalTerm() {
  any_epoch_state_ = TermState(
    any_epoch_sentinel, false, true, getNumChildren()
  );
}

void TerminationDetector::freeEpoch(EpochType const& epoch) {
  // Clean up epoch ready std::unordered_set
  {
    auto iter = epoch_ready_.find(epoch);
    if (iter != epoch_ready_.end()) {
      epoch_ready_.erase(iter);
    }
  }

  auto window = getWindow(epoch);
  window->clean(epoch);

  // Clean up any actions lambdas associated with epoch
  {
    auto iter = epoch_actions_.find(epoch);
    if (iter != epoch_actions_.end()) {
      epoch_actions_.erase(iter);
    }
  }

  // Clean up local epoch state associated with epoch
  {
    auto iter = epoch_state_.find(epoch);
    if (iter != epoch_state_.end()) {
      epoch_state_.erase(iter);
    }
  }
}

bool TerminationDetector::propagateEpoch(TermStateType& state) {
  bool const& is_ready = state.readySubmitParent();
  bool const& is_root = isRoot();
  auto const& parent = getParent();

  debug_print_verbose(
    term, node,
    "propagateEpoch: epoch={:x}, l_prod={}, l_cons={}, event_count={}, "
    "children={}, is_ready={}\n",
    state.getEpoch(), state.l_prod, state.l_cons, state.getRecvChildCount(),
    state.getNumChildren(), print_bool(is_ready)
  );

  if (is_ready) {
    state.g_prod1 += state.l_prod;
    state.g_cons1 += state.l_cons;

    debug_print_verbose(
      term, node,
      "propagateEpoch: epoch={:x}, l_prod={}, l_cons={}, "
      "g_prod1={}, g_cons1={}, event_count={}, children={}\n",
      state.getEpoch(), state.l_prod, state.l_cons, state.g_prod1, state.g_cons1,
      state.getRecvChildCount(), state.getNumChildren()
    );

    if (not is_root) {
      auto msg = makeSharedMessage<TermCounterMsg>(
        state.getEpoch(), state.g_prod1, state.g_cons1
      );
      theMsg()->setTermMessage(msg);
      theMsg()->sendMsg<TermCounterMsg, propagateEpochHandler>(parent, msg);

      debug_print_verbose(
        term, node,
        "propagateEpoch: sending to parent: {}, msg={}, epoch={:x}, wave={}\n",
        parent, print_ptr(msg), state.getEpoch(), state.getCurWave()
      );

    } else /*if (is_root) */ {
      bool const& is_term =
        state.g_prod1 == state.g_cons1 and
        state.g_prod2 == state.g_cons2 and
        state.g_prod1 == state.g_prod2;

      // four-counter method implementation
      debug_print(
        term, node,
        "propagateEpoch [root]: epoch={:x}, g_prod1={}, g_cons1={}, "
        "g_prod2={}, g_cons2={}, detected_term={}\n",
        state.getEpoch(), state.g_prod1, state.g_cons1, state.g_prod2,
        state.g_cons2, is_term
      );

      if (is_term) {
        auto msg = makeSharedMessage<TermMsg>(state.getEpoch());
        theMsg()->setTermMessage(msg);
        theMsg()->broadcastMsg<TermMsg, epochTerminatedHandler>(msg);

        state.setTerminated();

        epochTerminated(state.getEpoch());
      } else {
        if (!ArgType::vt_no_detect_hang) {
          // Counts are the same as previous iteration
          if (state.g_prod2 == state.g_prod1 && state.g_cons2 == state.g_cons1) {
            state.constant_count++;
          } else {
            state.constant_count = 0;
          }

          if (
            state.constant_count >= ArgType::vt_hang_freq and
            state.constant_count %  ArgType::vt_hang_freq == 0
          ) {
            if (
              state.num_print_constant == 0 or
              std::log(static_cast<double>(state.constant_count)) >
              state.num_print_constant
            ) {
              auto node            = ::vt::debug::preNode();
              auto vt_pre          = ::vt::debug::vtPre();
              auto node_str        = ::vt::debug::proc(node);
              auto prefix          = vt_pre + node_str + " ";
              auto reset           = ::vt::debug::reset();
              auto bred            = ::vt::debug::bred();
              auto magenta         = ::vt::debug::magenta();

              // debug additional infos
              auto const& current  = state.getEpoch();
              bool const is_rooted = epoch::EpochManip::isRooted(current);
              bool const has_categ = epoch::EpochManip::hasCategory(current);
              bool const useDS = has_categ
                and epoch::EpochManip::category(current) ==
                    epoch::eEpochCategory::DijkstraScholtenEpoch;

              auto f1 = fmt::format(
                "{}Termination hang detected:{} {}traversals={} epoch={:x} "
                "produced={}{} {}consumed={}{} rooted={}, ds={}\n",
                bred, reset,
                magenta, state.constant_count, state.getEpoch(), state.g_prod1,
                reset, magenta, state.g_cons1, reset, is_rooted, useDS
              );
              vt_print(term, "{}", f1);
              state.num_print_constant++;

              #if !backend_check_enabled(production)
                if (state.num_print_constant > 10) {
                  vtAbort(
                    "Hang detected (consumed != produced) for k tree "
                    "traversals"
                  );
                }
              #endif
            }
          }
        }

        state.g_prod2 = state.g_prod1;
        state.g_cons2 = state.g_cons1;
        state.g_prod1 = state.g_cons1 = 0;
        state.setCurWave(state.getCurWave() + 1);

        debug_print_verbose(
          term, node,
          "propagateEpoch [root]: epoch={:x}, wave={}, continue\n",
          state.getEpoch(), state.getCurWave()
        );

        auto msg = makeSharedMessage<TermMsg>(state.getEpoch(), state.getCurWave());
        theMsg()->setTermMessage(msg);
        theMsg()->broadcastMsg<TermMsg, epochContinueHandler>(msg);
      }
    }

    debug_print_verbose(
      term, node,
      "propagateEpoch: epoch={:x}, is_root={}: reset counters\n",
      state.getEpoch(), print_bool(is_root)
    );

    // reset counters
    state.g_prod1 = state.g_cons1 = 0;
    state.submitToParent(is_root);
  }

  return is_ready;
}

void TerminationDetector::cleanupEpoch(EpochType const& epoch) {
  if (epoch != any_epoch_sentinel) {
    if (isDS(epoch)) {
      auto ds_term_iter = term_.find(epoch);
      if (ds_term_iter != term_.end()) {
        term_.erase(ds_term_iter);
      }
    } else {
      auto epoch_iter = epoch_state_.find(epoch);
      if (epoch_iter != epoch_state_.end()) {
        epoch_state_.erase(epoch_iter);
      }
    }
  }
}

void TerminationDetector::epochTerminated(EpochType const& epoch) {
  debug_print(
    term, node,
    "epochTerminated: epoch={:x}, is_rooted_epoch={}, is_ds={}\n",
    epoch, isRooted(epoch), isDS(epoch)
  );

  // Clear all the children epochs that are nested by this epoch (waiting on it
  // to complete)
  if (isDS(epoch)) {
    getDSTerm(epoch)->clearParents();
  } else {
    if (epoch != term::any_epoch_sentinel) {
      vtAssertExpr(epoch_state_.find(epoch) != epoch_state_.end());
      findOrCreateState(epoch, false).clearParents();
    } else {
      // Although in theory the term::any_epoch_sentinel could track all other
      // epochs as children, it does not need for correctness (and this would be
      // expensive)
    }
  }

  // Trigger actions associated with epoch
  triggerAllActions(epoch);

  // Update the window for the epoch archetype
  updateResolvedEpochs(epoch);

  // Cleanup epoch meta-data
  cleanupEpoch(epoch);
}

void TerminationDetector::inquireTerminated(
  EpochType const& epoch, NodeType const& from
) {
  auto const& is_rooted = epoch::EpochManip::isRooted(epoch);
  auto const& epoch_root_node = epoch::EpochManip::node(epoch);
  auto const& this_node = theContext()->getNode();

  vtAssertInfo(
    !is_rooted || epoch_root_node == this_node,
    "Must be not rooted or this is root node",
    is_rooted, epoch_root_node, epoch, from
  );

  debug_print(
    term, node,
    "inquireTerminated: epoch={:x}, is_rooted={}, root={}, from={}\n",
    epoch, is_rooted, epoch_root_node, from
  );

  addAction(epoch, [=]{
    debug_print(
      term, node,
      "inquireTerminated: epoch={:x}, from={} ready trigger\n", epoch, from
    );

    bool const is_ready = true;
    auto msg = makeMessage<TermTerminatedReplyMsg>(epoch,is_ready);
    theMsg()->sendMsg<TermTerminatedReplyMsg,replyEpochTerminated>(from,msg.get());
  });
}

void TerminationDetector::replyTerminated(
  EpochType const& epoch, bool const& is_terminated
) {
  debug_print(
    term, node,
    "replyTerminated: epoch={:x}, is_terminated={}\n",
    epoch, is_terminated
  );
  vtAssertExpr(is_terminated == true);

  // Remove the entry for the pending status of this remote epoch
  auto iter = epoch_wait_status_.find(epoch);
  if (iter != epoch_wait_status_.end()) {
    epoch_wait_status_.erase(iter);
  }

  epochTerminated(epoch);
}

void TerminationDetector::updateResolvedEpochs(EpochType const& epoch) {
  if (epoch != any_epoch_sentinel) {
    debug_print(
      term, node,
      "updateResolvedEpoch: epoch={:x}, rooted={}, "
      "collective: first={:x}, last={:x}\n",
      epoch, isRooted(epoch), epoch_coll_->getFirst(), epoch_coll_->getLast()
    );

    getWindow(epoch)->closeEpoch(epoch);
  }
}

TermStatusEnum TerminationDetector::testEpochTerminated(EpochType epoch) {
  TermStatusEnum status = TermStatusEnum::Pending;
  auto const& is_rooted_epoch = epoch::EpochManip::isRooted(epoch);

  if (is_rooted_epoch) {
    auto const& this_node = theContext()->getNode();
    auto const& root = epoch::EpochManip::node(epoch);
    if (root == this_node) {
      /*
       * The idea here is that if this is executed on the root, it must have
       * valid info on whether the rooted live or terminated
       */
      auto window = getWindow(epoch);
      auto is_terminated = window->isTerminated(epoch);
      if (is_terminated) {
        status = TermStatusEnum::Terminated;
      }
    } else {
      auto iter = epoch_wait_status_.find(epoch);
      if (iter == epoch_wait_status_.end()) {
        /*
         * Send a message to the root node to find out whether this epoch is
         * terminated or not
         */
        auto msg = makeMessage<TermTerminatedMsg>(epoch,this_node);
        theMsg()->sendMsg<TermTerminatedMsg,inquireEpochTerminated>(root,msg.get());
        epoch_wait_status_.insert(epoch);
      }
      status = TermStatusEnum::Remote;
    }
  } else {
    auto const& is_terminated = epoch_coll_->isTerminated(epoch);
    if (is_terminated) {
      status = TermStatusEnum::Terminated;
    }
  }

  debug_print(
    term, node,
    "testEpochTerminated: epoch={:x}, pending={}, terminated={}, remote={}\n",
    epoch, status == TermStatusEnum::Pending, status == TermStatusEnum::Terminated,
    status == TermStatusEnum::Remote
  );

  return status;
}

void TerminationDetector::epochContinue(
  EpochType const& epoch, TermWaveType const& wave
) {
  debug_print(
    term, node,
    "epochContinue: epoch={:x}, wave={}\n",
    epoch, wave
  );

  if (epoch == any_epoch_sentinel) {
    any_epoch_state_.receiveContinueSignal(wave);
  } else {
    auto epoch_iter = epoch_state_.find(epoch);
    if (epoch_iter != epoch_state_.end()) {
      epoch_iter->second.receiveContinueSignal(wave);
    }
  }

  theTerm()->maybePropagate();
}

void TerminationDetector::linkChildEpoch(EpochType child, EpochType parent) {
  // Add the current active epoch in the messenger as a parent epoch so the
  // current epoch does not detect termination until the new epoch terminations
  auto const parent_epoch = parent != no_epoch ? parent : theMsg()->getEpoch();
  bool const has_parent =
    parent_epoch != no_epoch && parent_epoch != term::any_epoch_sentinel;

  debug_print(
    term, node,
    "linkChildEpoch: has_parent={}, in parent={:x}, cur={:x}, child={:x}\n",
    has_parent, parent, theMsg()->getEpoch(), child
  );

  if (has_parent) {
    if (isDS(child)) {
      getDSTerm(child)->addParentEpoch(parent_epoch);
    } else {
      auto& state = findOrCreateState(child, false);
      state.addParentEpoch(parent_epoch);
    }
  }
}

void TerminationDetector::finishNoActivateEpoch(EpochType const& epoch) {
  auto ready_iter = epoch_ready_.find(epoch);
  if (ready_iter == epoch_ready_.end()) {
    epoch_ready_.emplace(epoch);
    consume(epoch,1);
  }
}

void TerminationDetector::finishedEpoch(EpochType const& epoch) {
  debug_print(
    term, node,
    "finishedEpoch: epoch={:x}, finished={}\n",
    epoch, epoch_ready_.find(epoch) != epoch_ready_.end()
  );

  finishNoActivateEpoch(epoch);
  activateEpoch(epoch);

  debug_print(
    term, node,
    "finishedEpoch: (after consume) epoch={:x}\n",
    epoch
  );
}

EpochType TerminationDetector::makeEpochRootedNorm(bool child, EpochType parent) {
  auto const epoch = epoch::EpochManip::makeNewRootedEpoch();

  debug_print(
    term, node,
    "makeEpochRootedNorm: root={}, child={}, epoch={:x}, parent={:x}\n",
    theContext()->getNode(), child, epoch, parent
  );

  /*
   *  Broadcast new rooted epoch to all other nodes to start processing this
   *  epoch
   */
  auto msg = makeSharedMessage<TermMsg>(epoch);
  theMsg()->setTermMessage(msg);
  theMsg()->broadcastMsg<TermMsg,makeRootedHandler>(msg);

  /*
   *  Setup the new rooted epoch locally on the root node (this node)
   */
  makeRootedHan(epoch,true);

  if (child) {
    linkChildEpoch(epoch,parent);
  }

  return epoch;

}

EpochType TerminationDetector::makeEpochRootedDS(bool child, EpochType parent) {
  auto const ds_cat = epoch::eEpochCategory::DijkstraScholtenEpoch;
  auto const epoch = epoch::EpochManip::makeNewRootedEpoch(false, ds_cat);

  vtAssert(term_.find(epoch) == term_.end(), "New epoch must not exist");

  // Create DS term where this node is the root
  getDSTerm(epoch, true);
  getWindow(epoch)->addEpoch(epoch);
  produce(epoch,1);

  if (child) {
    linkChildEpoch(epoch,parent);
  }

  debug_print(
    term, node,
    "makeEpochRootedDS: child={}, parent={:x}, epoch={:x}\n",
    child, parent, epoch
  );

  return epoch;
}

EpochType TerminationDetector::makeEpochRooted(
  bool useDS, bool child, EpochType parent
) {
  /*
   *  This method should only be called by the root node for the rooted epoch
   *  identifier, which is distinct and has the node embedded in it to
   *  distinguish it from all other epochs
   */

  debug_print(
    term, node,
    "makeEpochRooted: root={}, is_ds={}, child={}, parent={:x}\n",
    theContext()->getNode(), useDS, child, parent
  );

  if (useDS) {
    return makeEpochRootedDS(child,parent);
  } else {
    return makeEpochRootedNorm(child,parent);
  }
}

EpochType TerminationDetector::makeEpochCollective(
  bool child, EpochType parent
) {
  auto const epoch = epoch::EpochManip::makeNewEpoch();

  debug_print(
    term, node,
    "makeEpochCollective: epoch={:x}, child={}, parent={:x}\n",
    epoch, child, parent
  );

  getWindow(epoch)->addEpoch(epoch);
  produce(epoch,1);
  setupNewEpoch(epoch);

  if (child) {
    linkChildEpoch(epoch,parent);
  }

  return epoch;
}

EpochType TerminationDetector::makeEpoch(
  bool is_coll, bool useDS, bool child, EpochType parent
) {
  return is_coll ?
    makeEpochCollective(child,parent) :
    makeEpochRooted(useDS,child,parent);
}

void TerminationDetector::activateEpoch(EpochType const& epoch) {
  if (!isDS(epoch)) {
    debug_print(
      term, node,
      "activateEpoch: epoch={:x}\n", epoch
    );

    auto& state = findOrCreateState(epoch, true);
    state.activateEpoch();
    state.notifyLocalTerminated();
    if (state.readySubmitParent()) {
      propagateEpoch(state);
    }
  }
}

void TerminationDetector::makeRootedHan(EpochType const& epoch, bool is_root) {
  bool const is_ready = !is_root;

  findOrCreateState(epoch, is_ready);
  getWindow(epoch)->addEpoch(epoch);

  debug_print(
    term, node,
    "makeRootedHan: epoch={:x}, is_root={}\n", epoch, is_root
  );

  produce(epoch,1);

  // The user calls finishedEpoch on the root for a non-DS rooted epoch
  if (!is_root) {
    finishedEpoch(epoch);
  }
}

void TerminationDetector::setupNewEpoch(EpochType const& epoch) {
  auto epoch_iter = epoch_state_.find(epoch);

  bool const found = epoch_iter != epoch_state_.end();

  debug_print(
    term, node,
    "setupNewEpoch: epoch={:x}, found={}, count={}\n",
    epoch, print_bool(found),
    (found ? epoch_iter->second.getRecvChildCount() : -1)
  );

  auto& state = findOrCreateState(epoch, false);
  state.notifyLocalTerminated();
}

}} // end namespace vt::term
