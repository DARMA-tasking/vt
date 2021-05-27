/*
//@HEADER
// *****************************************************************************
//
//                                termination.cc
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
#include "vt/termination/termination.h"
#include "vt/termination/term_common.h"
#include "vt/messaging/active.h"
#include "vt/collective/collective_ops.h"
#include "vt/scheduler/scheduler.h"
#include "vt/epoch/epoch_headers.h"
#include "vt/termination/dijkstra-scholten/ds_headers.h"
#include "vt/termination/dijkstra-scholten/comm.h"
#include "vt/termination/dijkstra-scholten/ds.h"
#include "vt/configs/arguments/app_config.h"
#include "vt/configs/debug/debug_colorize.h"
#include "vt/collective/collective_alg.h"
#include "vt/pipe/pipe_headers.h"

#include <memory>

namespace vt { namespace term {

TerminationDetector::TerminationDetector()
  : collective::tree::Tree(collective::tree::tree_cons_tag_t),
  any_epoch_state_(any_epoch_sentinel, false, true, getNumChildren()),
  hang_(no_epoch, true, false, getNumChildren())
{ }

/*static*/ void TerminationDetector::makeRootedHandler(TermMsg* msg) {
  theTerm()->makeRootedHan(msg->new_epoch, false);
}

/*static*/ void
TerminationDetector::propagateEpochHandler(TermCounterMsg* msg) {
  theTerm()->propagateEpochExternal(msg->epoch, msg->prod, msg->cons);
}

/*static*/ void TerminationDetector::epochTerminatedHandler(TermMsg* msg) {
  theTerm()->epochTerminated(msg->new_epoch, CallFromEnum::NonRoot);
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

TermCounterType TerminationDetector::getNumUnits() const {
  return any_epoch_state_.g_cons2;
}

void TerminationDetector::setLocalTerminated(
  bool const local_terminated, bool const no_propagate
) {
  vt_debug_print(
    verbose, term,
    "setLocalTerminated: is_term={}, no_propagate={}\n",
    local_terminated, no_propagate
  );

  any_epoch_state_.notifyLocalTerminated(local_terminated);

  if (local_terminated && !no_propagate) {
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

  vt_debug_print(
    verbose, term,
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
  vt_debug_print(
    verbose, termds,
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
  vt_debug_print(
    normal, term,
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
  if (any_epoch_state_.readySubmitParent()) {
    propagateEpoch(any_epoch_state_);
  }

  if (hang_.readySubmitParent()) {
    propagateEpoch(hang_);
  }

  for (auto&& state : epoch_state_) {
    if (state.second.readySubmitParent()) {
      propagateEpoch(state.second);
    }
  }
}

void TerminationDetector::propagateEpochExternalState(
  TermStateType& state, TermCounterType const& prod, TermCounterType const& cons
) {
  vt_debug_print(
    verbose, term,
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
  vt_debug_print(
    verbose, term,
    "propagateEpochExternal: epoch={:x}, prod={}, cons={}\n",
    epoch, prod, cons
  );

  if (epoch == any_epoch_sentinel) {
    propagateEpochExternalState(any_epoch_state_, prod, cons);
  } else if (epoch == no_epoch) {
    // Dispatch to special hang detection epoch, demarcated as "no_epoch"
    propagateEpochExternalState(hang_, prod, cons);
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

std::shared_ptr<TerminationDetector::EpochGraph> TerminationDetector::makeGraph() {
  // Start at the global epoch root;
  if (any_epoch_state_.isTerminated()) {
    return nullptr;
  } else {
    // We can't traverse the graph from source to sink naturally because the
    // graph is stored inverted, with predecessors only have pointers to their
    // successors (not vice versa). Thus, we will extract all predecessors and
    // then build the graph recursively by joining epoch nodes with their
    // successor in the graph data structure

    // Collect live epochs, both collective and rooted (excluding rooted ones
    // that did not originate on this node)
    std::unordered_map<EpochType, std::shared_ptr<EpochGraph>> live_epochs;
    std::string const glabel = "Global";
    auto root = std::make_shared<EpochGraph>(any_epoch_state_.getEpoch(), glabel);
    // Collect non-rooted epochs, just collective, excluding DS or other rooted
    // epochs (info about them is localized on the creation node)
    auto const this_node = theContext()->getNode();
    for (auto const& elm : epoch_state_) {
      auto const ep = elm.first;
      bool const rooted = epoch::EpochManip::isRooted(ep);
      if (not rooted or (rooted and epoch::EpochManip::node(ep) == this_node)) {
        if (not isEpochTerminated(elm.first)) {
          auto label = elm.second.getLabel();
          live_epochs[ep] = std::make_shared<EpochGraph>(ep, label);
        }
      }
    }
    for (auto const& elm : term_) {
      // Only include DS epochs that are created here. Other nodes do not have
      // proper successor info about the rooted, DS epochs
      if (epoch::EpochManip::node(elm.first) == this_node) {
        if (not isEpochTerminated(elm.first)) {
          auto label = elm.second.getLabel();
          live_epochs[elm.first] = std::make_shared<EpochGraph>(
            elm.first, label
          );
        }
      }
    }

    for (auto& live : live_epochs) {
      auto const ep = live.first;
      auto successors = getEpochDep(ep)->getSuccessors();
      if (successors.size() == 0) {
        // No successors implies that this epoch is a direct descendent from the
        // root
        root->addSuccessor(live.second);
      } else {
        for (auto&& p : successors) {
          auto pt = live_epochs.find(p);
          if (pt != live_epochs.end()) {
            pt->second->addSuccessor(live.second);
          } else {
            vtAssert(false, "Successor epoch has terminated before its child!");
          }
        }
      }
    }

    return root;
  }
}


bool TerminationDetector::propagateEpoch(TermStateType& state) {
  bool const& is_ready = state.readySubmitParent();
  bool const& is_root = isRoot();
  auto const& parent = getParent();

  vt_debug_print(
    verbose, term,
    "propagateEpoch: epoch={:x}, l_prod={}, l_cons={}, event_count={}, "
    "children={}, is_ready={}\n",
    state.getEpoch(), state.l_prod, state.l_cons, state.getRecvChildCount(),
    state.getNumChildren(), print_bool(is_ready)
  );

  if (is_ready) {
    bool is_term = false;

    // Update the global counters for a given epoch
    state.g_prod1 += state.l_prod;
    state.g_cons1 += state.l_cons;

    vt_debug_print(
      verbose, term,
      "propagateEpoch: epoch={:x}, l_prod={}, l_cons={}, "
      "g_prod1={}, g_cons1={}, event_count={}, children={}\n",
      state.getEpoch(), state.l_prod, state.l_cons, state.g_prod1, state.g_cons1,
      state.getRecvChildCount(), state.getNumChildren()
    );

    if (not is_root) {
      auto msg = makeMessage<TermCounterMsg>(
        state.getEpoch(), state.g_prod1, state.g_cons1
      );
      theMsg()->markAsTermMessage(msg);

      vt_debug_print(
        verbose, term,
        "propagateEpoch: sending to parent: {}, msg={}, epoch={:x}, wave={}\n",
        parent, print_ptr(msg.get()), state.getEpoch(), state.getCurWave()
      );

      theMsg()->sendMsg<TermCounterMsg, propagateEpochHandler>(parent, msg);
    } else /*if (is_root) */ {
      is_term =
        state.g_prod1 == state.g_cons1 and
        state.g_prod2 == state.g_cons2 and
        state.g_prod1 == state.g_prod2;

      // four-counter method implementation
      vt_debug_print(
        normal, term,
        "propagateEpoch [root]: epoch={:x}, g_prod1={}, g_cons1={}, "
        "g_prod2={}, g_cons2={}, detected_term={}\n",
        state.getEpoch(), state.g_prod1, state.g_cons1, state.g_prod2,
        state.g_cons2, is_term
      );

      if (not theConfig()->vt_no_detect_hang) {
        // Hang detection has confirmed a fatal hang---abort!
        if (is_term and state.getEpoch() == no_epoch) {
          vt_print(
            term,
            "Detected hang: write graph to file={}\n",
            theConfig()->vt_epoch_graph_on_hang
          );
          if (theConfig()->vt_epoch_graph_on_hang) {
            startEpochGraphBuild();
            // After spawning the build, spin until the file gets written out so
            // vtAbort does not exit too early
            theSched()->runSchedulerWhile([this]{
              return not has_printed_epoch_graph or not theSched()->isIdle();
            });
          }
          vtAbort("Detected hang indicating no further progress is possible");
        }
      }

      if (is_term) {
        auto msg = makeMessage<TermMsg>(state.getEpoch());
        theMsg()->markAsTermMessage(msg);
        theMsg()->broadcastMsg<TermMsg, epochTerminatedHandler>(msg, false);

        vt_debug_print(
          terse, term,
          "propagateEpoch [root]: epoch={:x}, g_prod1={}, g_cons1={}, "
          "g_prod2={}, g_cons2={}: termination detected\n",
          state.getEpoch(), state.g_prod1, state.g_cons1, state.g_prod2,
          state.g_cons2
        );

        state.setTerminated();

        epochTerminated(state.getEpoch(), CallFromEnum::Root);
      } else {
        if (state.g_prod2 == state.g_prod1 and state.g_cons2 == state.g_cons1) {
          state.constant_count++;
        } else {
          state.constant_count = 0;
        }

        if (state.constant_count > 0) {
          countsConstant(state);
        }

        state.g_prod2 = state.g_prod1;
        state.g_cons2 = state.g_cons1;
        state.g_prod1 = state.g_cons1 = 0;
        state.setCurWave(state.getCurWave() + 1);

        vt_debug_print(
          verbose, term,
          "propagateEpoch [root]: epoch={:x}, wave={}, continue\n",
          state.getEpoch(), state.getCurWave()
        );

        auto msg = makeMessage<TermMsg>(state.getEpoch(), state.getCurWave());
        theMsg()->markAsTermMessage(msg);
        theMsg()->broadcastMsg<TermMsg, epochContinueHandler>(msg, false);
      }
    }

    if (not is_term) {
      vt_debug_print(
        verbose, term,
        "propagateEpoch: epoch={:x}, is_root={}: reset counters\n",
        state.getEpoch(), print_bool(is_root)
      );

      // reset counters
      state.g_prod1 = state.g_cons1 = 0;
      state.submitToParent(is_root);
    }
  }

  return is_ready;
}

void TerminationDetector::countsConstant(TermStateType& state) {
  bool enter = not theConfig()->vt_no_detect_hang or theConfig()->vt_print_no_progress;
  if (enter) {
    bool is_global_epoch = state.getEpoch() == any_epoch_sentinel;
    bool is_hang_detector = state.getEpoch() == no_epoch;

    auto reset           = ::vt::debug::reset();
    auto bred            = ::vt::debug::bred();
    auto magenta         = ::vt::debug::magenta();

    if (is_hang_detector) {
      auto f1 = fmt::format(
        "{}Progress has stalled, but hang detection implies messages are in"
        " flight!{}\n",
        bred, reset
      );
      vt_print(term, "{}", f1);
      return;
    }

    if (
      state.constant_count >= theConfig()->vt_hang_freq and
      state.constant_count %  theConfig()->vt_hang_freq == 0
    ) {
      if (
        state.num_print_constant == 0 or
        std::log(static_cast<double>(state.constant_count)) >
        state.num_print_constant
      ) {
        vt_debug_print(
          normal, term,
          "countsConstant: epoch={:x}, state.constant_count={}\n",
          state.getEpoch(), state.constant_count
        );

        if (theConfig()->vt_print_no_progress) {
          auto const current  = state.getEpoch();
          bool const is_rooted = epoch::EpochManip::isRooted(current);
          bool const useDS = epoch::EpochManip::category(current) ==
            epoch::eEpochCategory::DijkstraScholtenEpoch;

          auto f1 = fmt::format(
            "{}Termination counts constant (no progress) for:{} {}traversals={} "
            "epoch={:x} produced={}{} {}consumed={}{} rooted={}, ds={}\n",
            bred, reset,
            magenta, state.constant_count, state.getEpoch(), state.g_prod1,
            reset, magenta, state.g_cons1, reset, is_rooted, useDS
          );
          vt_print(term, "{}", f1);
        }

        state.num_print_constant++;

        if (state.num_print_constant == 10) {
          if (is_global_epoch) {
            // Start running final check to see if we are hung for sure
            if (not theConfig()->vt_no_detect_hang) {
              auto msg = makeMessage<HangCheckMsg>();
              theMsg()->markAsTermMessage(msg.get());
              theMsg()->broadcastMsg<HangCheckMsg, hangCheckHandler>(msg, false);
              hangCheckHandler(nullptr);
            }
          }
        }
      }
    }
  }

}

void TerminationDetector::startEpochGraphBuild() {
  // Broadcast to build local EpochGraph(s), merge the graphs, and output to
  // file
  if (theConfig()->vt_epoch_graph_on_hang) {
    auto msg = makeMessage<BuildGraphMsg>();
    theMsg()->markAsTermMessage(msg.get());
    theMsg()->broadcastMsg<BuildGraphMsg, buildLocalGraphHandler>(msg, false);
    buildLocalGraphHandler(nullptr);
  }
}

/*static*/ void TerminationDetector::hangCheckHandler(HangCheckMsg* msg) {
  fmt::print("{}:hangCheckHandler\n",theContext()->getNode());
  theTerm()->hang_.activateEpoch();
}

/*static*/ void TerminationDetector::buildLocalGraphHandler(BuildGraphMsg*) {
  using MsgType  = EpochGraphMsg;
  using ReduceOp = collective::PlusOp<EpochGraph>;

  vt_debug_print(
    verbose, term,
    "buildLocalGraphHandler: building local epoch graph\n"
  );

  /*
   * Make the local epoch graph on this node
   */
  auto graph = theTerm()->makeGraph();

  /*
   * Check for any cycles in the graph. If cycles are detected (will always
   * cause a hang) `detectCycles` will abort and print the cycle that was found.
   */
  graph->detectCycles();

  /*
   * Generate the DOT file to output to file, reduce to create a global view of
   * the epoch graph
   */
  auto str = graph->outputDOT();
  graph->writeToFile(str);
  auto msg = makeMessage<MsgType>(graph);
  NodeType root = 0;
  auto cb = vt::theCB()->makeSend<MsgType, epochGraphBuiltHandler>(root);

  auto r = theTerm()->reducer();
  r->reduce<ReduceOp>(root, msg.get(), cb);

  if (theContext()->getNode() != root) {
    theTerm()->has_printed_epoch_graph = true;
  }
}

/*static*/ void TerminationDetector::epochGraphBuiltHandler(EpochGraphMsg* msg) {
  vt_debug_print(
    verbose, term,
    "epochGraphBuiltHandler: collected global, merged graph\n"
  );

  auto graph = msg->getVal();
  auto str = graph.outputDOT();
  graph.writeToFile(str, true);
  theTerm()->has_printed_epoch_graph = true;
}

void TerminationDetector::cleanupEpoch(EpochType const& epoch, CallFromEnum from) {
  vt_debug_print(
    normal, term,
    "cleanupEpoch: epoch={:x}, is_rooted_epoch={}, is_ds={}, isRoot={}\n",
    epoch, isRooted(epoch), isDS(epoch), from == CallFromEnum::Root ? true : false
  );

  if (epoch != any_epoch_sentinel) {
    if (isDS(epoch)) {
      if (from == CallFromEnum::NonRoot) {
        auto ds_term_iter = term_.find(epoch);
        if (ds_term_iter != term_.end()) {
          term_.erase(ds_term_iter);
        }
      } else {
        theSched()->enqueue([epoch]{
          theTerm()->cleanupEpoch(epoch, CallFromEnum::NonRoot);
        });
      }
    } else {
      // For the non-root, epoch_state_ can be cleaned immediately. Otherwise,
      // we might be iterating through state so its not safe to erase
      if (from == CallFromEnum::NonRoot) {
        auto iter = epoch_state_.find(epoch);
        if (iter != epoch_state_.end()) {
          epoch_state_.erase(iter);
        }
      } else {
        // Schedule the cleanup for later, we are in the midst of iterating and
        // can't safely erase it immediately
        theSched()->enqueue([epoch]{
          theTerm()->cleanupEpoch(epoch, CallFromEnum::NonRoot);
        });
      }
    }
    // Clean up ready state since the epoch has terminated
    auto ready_iter = epoch_ready_.find(epoch);
    if (ready_iter != epoch_ready_.end()) {
      epoch_ready_.erase(ready_iter);
    }
  }
}

void TerminationDetector::epochTerminated(EpochType const& epoch, CallFromEnum from) {
  vt_debug_print(
    normal, term,
    "epochTerminated: epoch={:x}, is_rooted_epoch={}, is_ds={}, isRoot={}\n",
    epoch, isRooted(epoch), isDS(epoch), from == CallFromEnum::Root ? true : false
  );

  // Clear all the successor epochs that are nested by this epoch (waiting on it
  // to complete)
  if (epoch != term::any_epoch_sentinel) {
    auto dep = getEpochDep(epoch);
    if (dep != nullptr) {
      dep->clearSuccessors();
    }
  }

  // Trigger actions associated with epoch
  triggerAllActions(epoch);

  // Update the window for the epoch archetype
  updateResolvedEpochs(epoch);

  // Call cleanup epoch to remove state
  cleanupEpoch(epoch, from);

  // Matching consume on global epoch once a nested epoch terminates
  if (epoch != any_epoch_sentinel) {
    auto const this_node = theContext()->getNode();
    bool const is_rooted = isRooted(epoch);
    bool const is_ds = isDS(epoch);
    bool const this_node_root = epoch::EpochManip::node(epoch) == this_node;
    if (not is_rooted or is_ds or (is_rooted and this_node_root)) {
      consumeOnGlobal(epoch);
    }
  }
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

  vt_debug_print(
    normal, term,
    "inquireTerminated: epoch={:x}, is_rooted={}, root={}, from={}\n",
    epoch, is_rooted, epoch_root_node, from
  );

  addAction(epoch, [=]{
    vt_debug_print(
      normal, term,
      "inquireTerminated: epoch={:x}, from={} ready trigger\n", epoch, from
    );

    bool const is_ready = true;
    auto msg = makeMessage<TermTerminatedReplyMsg>(epoch,is_ready);
    theMsg()->sendMsg<TermTerminatedReplyMsg,replyEpochTerminated>(from, msg);
  });
}

void TerminationDetector::replyTerminated(
  EpochType const& epoch, bool const& is_terminated
) {
  vt_debug_print(
    normal, term,
    "replyTerminated: epoch={:x}, is_terminated={}\n",
    epoch, is_terminated
  );
  vtAssertExpr(is_terminated == true);

  // Remove the entry for the pending status of this remote epoch
  auto iter = epoch_wait_status_.find(epoch);
  if (iter != epoch_wait_status_.end()) {
    epoch_wait_status_.erase(iter);
  }

  epochTerminated(epoch, CallFromEnum::NonRoot);
}

void TerminationDetector::updateResolvedEpochs(EpochType const& epoch) {
  if (epoch != any_epoch_sentinel) {
    auto const first_term = theEpoch()->getTerminatedWindow(epoch)->getFirst();
    auto const last_term = theEpoch()->getTerminatedWindow(epoch)->getLast();
    vt_debug_print(
      normal, term,
      "updateResolvedEpoch: epoch={:x}, rooted={}, "
      "collective: first={:x}, last={:x}\n",
      epoch, isRooted(epoch), first_term, last_term
    );

    theEpoch()->getTerminatedWindow(epoch)->setEpochTerminated(epoch);
  }
}

bool TerminationDetector::isEpochTerminated(EpochType epoch) {
  return testEpochTerminated(epoch) == TermStatusEnum::Terminated;
}

TermStatusEnum TerminationDetector::testEpochTerminated(EpochType epoch) {
  TermStatusEnum status = TermStatusEnum::Pending;
  auto const& is_rooted_epoch = epoch::EpochManip::isRooted(epoch);

  if (theEpoch()->getTerminatedWindow(epoch)->isTerminated(epoch)) {
    status = TermStatusEnum::Terminated;
  } else if (is_rooted_epoch) {
    auto const& this_node = theContext()->getNode();
    auto const& root = epoch::EpochManip::node(epoch);
    if (root == this_node) {
      /*
       * The idea here is that if this is executed on the root, it must have
       * valid info on whether the rooted live or terminated
       */
      auto window = theEpoch()->getTerminatedWindow(epoch);
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
        theMsg()->sendMsg<TermTerminatedMsg,inquireEpochTerminated>(root, msg);
        epoch_wait_status_.insert(epoch);
      }
      status = TermStatusEnum::Remote;
    }
  } else {
    auto window = theEpoch()->getTerminatedWindow(epoch);
    auto const& is_terminated = window->isTerminated(epoch);
    if (is_terminated) {
      status = TermStatusEnum::Terminated;
    }
  }

  vt_debug_print(
    normal, term,
    "testEpochTerminated: epoch={:x}, pending={}, terminated={}, remote={}\n",
    epoch, status == TermStatusEnum::Pending, status == TermStatusEnum::Terminated,
    status == TermStatusEnum::Remote
  );

  return status;
}

void TerminationDetector::epochContinue(
  EpochType const& epoch, TermWaveType const& wave
) {
  vt_debug_print(
    normal, term,
    "epochContinue: epoch={:x}, wave={}\n",
    epoch, wave
  );

  if (epoch == any_epoch_sentinel) {
    any_epoch_state_.receiveContinueSignal(wave);
  } else if (epoch == no_epoch) {
    hang_.receiveContinueSignal(wave);
  } else {
    auto epoch_iter = epoch_state_.find(epoch);
    if (epoch_iter != epoch_state_.end()) {
      epoch_iter->second.receiveContinueSignal(wave);
    }
  }

  theTerm()->maybePropagate();
}

EpochDependency* TerminationDetector::getEpochDep(EpochType epoch) {
  if (isDS(epoch)) {
    auto term = getDSTerm(epoch);
    return static_cast<EpochDependency*>(term);
  } else {
    auto& state = findOrCreateState(epoch, false);
    auto term = static_cast<EpochDependency*>(&state);
    return term;
  }
}

void TerminationDetector::addDependency(
  EpochType predecessor, EpochType successor
) {
  vt_debug_print(
    normal, term,
    "addDependency: successor={:x}, predecessor={:x}, cur_epoch={:x}\n",
    successor, predecessor, theMsg()->getEpoch()
  );

  if (not isEpochTerminated(predecessor)) {
    /*
     * Dependency optimization:
     *
     * Say that the current dependency structure looks like this:
     *   where
     *     succ successors are {c1, c2}
     *     pred successors are {c1, c3}
     *
     *                       succ    pred
     *                       /  \    /  \
     *                      c1  c2  c1  c3
     *
     * Now that we have added a new dependency, pred -> succ, pred's c1
     * dependency is carried through the transitive dependency. Thus, we
     * transform this graph (LHS) to a simpler graph (RHS):
     *
     *                pred                    pred
     *                / | \                   /  \
     *               c1 c2 succ      ====>   c2  succ
     *                     /  \                  /  \
     *                    c1  c3                c1  c3
     *
     */
    auto pred = getEpochDep(predecessor);
    auto succ_successors = getEpochDep(successor)->getSuccessors();
    pred->removeIntersection(succ_successors);
    pred->addSuccessor(successor);
  }
}

void TerminationDetector::removeEpochStateDependency(EpochType ep) {
  if (not isDS(ep)) {
    if (findOrCreateState(ep, true).decrementDependency() == 0) {
      // Call propagate because this might have activated an epoch begin further
      // propagated
      maybePropagate();
    }
  }
}

void TerminationDetector::addEpochStateDependency(EpochType ep) {
  if (not isDS(ep)) {
    // Increment a dependency so the epoch stops sending messages
    findOrCreateState(ep, true).incrementDependency();
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
  vt_debug_print(
    normal, term,
    "finishedEpoch: epoch={:x}, finished={}\n",
    epoch, epoch_ready_.find(epoch) != epoch_ready_.end()
  );

  finishNoActivateEpoch(epoch);
  activateEpoch(epoch);

  vt_debug_print(
    verbose, term,
    "finishedEpoch: (after consume) epoch={:x}\n",
    epoch
  );
}

EpochType TerminationDetector::makeEpochRootedWave(
  ParentEpochCapture successor, std::string const& label
) {
  auto const no_cat = epoch::eEpochCategory::NoCategoryEpoch;
  auto const epoch = theEpoch()->getNextRootedEpoch(no_cat);
  initializeRootedWaveEpoch(epoch, successor, label);
  return epoch;

}

void TerminationDetector::initializeRootedWaveEpoch(
  EpochType const epoch, ParentEpochCapture successor,
  std::string const& label
) {
  vt_debug_print(
    terse, term,
    "makeEpochRootedWave: root={}, epoch={:x}, successor={:x},"
    "label={}\n",
    theContext()->getNode(), epoch, successor, label
  );

  /*
   *  Broadcast new rooted epoch to all other nodes to start processing this
   *  epoch
   */
  auto msg = makeMessage<TermMsg>(epoch);
  theMsg()->markAsTermMessage(msg);
  theMsg()->broadcastMsg<TermMsg,makeRootedHandler>(msg, false);

  /*
   *  Setup the new rooted epoch locally on the root node (this node)
   */
  makeRootedHan(epoch, true, label);

  if (successor.valid()) {
    addDependency(epoch, successor);
  }
}

EpochType TerminationDetector::makeEpochRootedDS(
  ParentEpochCapture successor, std::string const& label
) {
  auto const ds_cat = epoch::eEpochCategory::DijkstraScholtenEpoch;
  auto const epoch = theEpoch()->getNextRootedEpoch(ds_cat);
  initializeRootedDSEpoch(epoch, successor, label);
  return epoch;
}

void TerminationDetector::initializeRootedDSEpoch(
  EpochType const epoch, ParentEpochCapture successor,
  std::string const& label
) {
  vtAssert(term_.find(epoch) == term_.end(), "New epoch must not exist");

  // Create DS term where this node is the root
  auto ds = getDSTerm(epoch, true);
  ds->setLabel(label);
  produce(epoch,1);
  produceOnGlobal(epoch);

  if (successor.valid()) {
    addDependency(epoch, successor);
  }

  vt_debug_print(
    terse, term,
    "makeEpochRootedDS: successor={:x}, epoch={:x}, label={}\n",
    successor, epoch, label
  );
}

EpochType TerminationDetector::makeEpochRooted(
  UseDS use_ds, ParentEpochCapture successor
) {
  return makeEpochRooted("", use_ds, successor);
}

EpochType TerminationDetector::makeEpochRooted(
  std::string const& label, UseDS use_ds, ParentEpochCapture successor
) {
  /*
   *  This method should only be called by the root node for the rooted epoch
   *  identifier, which is distinct and has the node embedded in it to
   *  distinguish it from all other epochs
   */

  vt_debug_print(
    normal, term,
    "makeEpochRooted: root={}, use_ds={}, successor={:x}, label={}\n",
    theContext()->getNode(), use_ds, successor, label
  );

  bool const force_use_ds = vt::theConfig()->vt_term_rooted_use_ds;
  bool const force_use_wave = vt::theConfig()->vt_term_rooted_use_wave;

  // Both force options should never be turned on
  vtAssertExpr(not (force_use_ds and force_use_wave));

  if ((use_ds or force_use_ds) and not force_use_wave) {
    return makeEpochRootedDS(successor, label);
  } else {
    return makeEpochRootedWave(successor, label);
  }
}

void TerminationDetector::initializeRootedEpoch(
  EpochType const epoch, std::string const& label, UseDS use_ds,
  ParentEpochCapture successor
) {
  if (use_ds) {
    initializeRootedDSEpoch(epoch, successor, label);
  } else {
    initializeRootedWaveEpoch(epoch, successor, label);
  }
}

EpochType TerminationDetector::makeEpochCollective(
  ParentEpochCapture successor
) {
  vt_debug_print(
    normal, term,
    "makeEpochCollective: no label\n"
  );

  return makeEpochCollective("", successor);
}

EpochType TerminationDetector::makeEpochCollective(
  std::string const& label, ParentEpochCapture successor
) {
  auto const epoch = theEpoch()->getNextCollectiveEpoch();
  initializeCollectiveEpoch(epoch, label, successor);
  return epoch;
}

void TerminationDetector::produceOnGlobal(EpochType ep) {
  vt_debug_print(
    term, node,
    "produceOnGlobal: ep={:x}\n", ep
  );

  produce(any_epoch_sentinel, 1);
}

void TerminationDetector::consumeOnGlobal(EpochType ep) {
  vt_debug_print(
    term, node,
    "consumeOnGlobal: ep={:x}\n", ep
  );

  consume(any_epoch_sentinel, 1);
}

void TerminationDetector::initializeCollectiveEpoch(
  EpochType const epoch, std::string const& label,
  ParentEpochCapture successor
) {
  vt_debug_print(
    terse, term,
    "makeEpochCollective: epoch={:x}, successor={:x}, label={}\n",
    epoch, successor, label
  );

  produce(epoch,1);
  produceOnGlobal(epoch);

  setupNewEpoch(epoch, label);

  if (successor.valid()) {
    addDependency(epoch, successor);
  }
}

EpochType TerminationDetector::makeEpoch(
  std::string const& label, bool is_coll, UseDS use_ds,
  ParentEpochCapture successor
) {
  return is_coll ?
    makeEpochCollective(label, successor) :
    makeEpochRooted(label, use_ds, successor);
}

void TerminationDetector::activateEpoch(EpochType const& epoch) {
  if (!isDS(epoch)) {
    vt_debug_print(
      normal, term,
      "activateEpoch: epoch={:x}\n", epoch
    );

    auto const this_node = theContext()->getNode();
    if (isRooted(epoch) and epoch::EpochManip::node(epoch) == this_node) {
      produceOnGlobal(epoch);
    }

    auto& state = findOrCreateState(epoch, true);
    state.activateEpoch();
    state.notifyLocalTerminated();
    if (state.readySubmitParent()) {
      propagateEpoch(state);
    }
  }
}

void TerminationDetector::makeRootedHan(
  EpochType const& epoch, bool is_root, std::string const& label
) {
  bool const is_ready = !is_root;

  theEpoch()->getTerminatedWindow(epoch)->activateEpoch(epoch);

  auto& state = findOrCreateState(epoch, is_ready);
  state.setLabel(label);

  vt_debug_print(
    normal, term,
    "makeRootedHan: epoch={:x}, is_root={}\n", epoch, is_root
  );

  produce(epoch,1);

  // The user calls finishedEpoch on the root for a non-DS rooted epoch
  if (!is_root) {
    finishedEpoch(epoch);
  }
}

void TerminationDetector::setupNewEpoch(
  EpochType const& epoch, std::string const& label
) {
  auto epoch_iter = epoch_state_.find(epoch);

  bool const found = epoch_iter != epoch_state_.end();

  vt_debug_print(
    normal, term,
    "setupNewEpoch: epoch={:x}, found={}, count={}\n",
    epoch, print_bool(found),
    (found ? epoch_iter->second.getRecvChildCount() : -1)
  );

  auto& state = findOrCreateState(epoch, false);
  state.notifyLocalTerminated();
  state.setLabel(label);
}

std::size_t TerminationDetector::getNumTerminatedCollectiveEpochs() const {
  auto const window = theEpoch()->getTerminatedWindow(any_epoch_sentinel);
  return window->getTotalTerminated();
}

}} // end namespace vt::term
