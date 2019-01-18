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

/*static*/ void TerminationDetector::propagateNewEpochHandler(TermMsg* msg) {
  bool const from_child = true;
  theTerm()->propagateNewEpoch(msg->new_epoch, from_child);
}

/*static*/ void TerminationDetector::readyEpochHandler(TermMsg* msg) {
  theTerm()->readyNewEpoch(msg->new_epoch);
}

/*static*/ void TerminationDetector::makeRootedEpoch(TermMsg* msg) {
  bool const is_root = false;
  theTerm()->makeRootedEpoch(msg->new_epoch, is_root);
}

/*static*/ void
TerminationDetector::propagateEpochHandler(TermCounterMsg* msg) {
  theTerm()->propagateEpochExternal(msg->epoch, msg->prod, msg->cons);
}

/*static*/ void TerminationDetector::epochFinishedHandler(TermMsg* msg) {
  theTerm()->epochFinished(msg->new_epoch, true);
}

/*static*/ void TerminationDetector::epochContinueHandler(TermMsg* msg) {
  theTerm()->epochContinue(msg->new_epoch, msg->wave);
}

/*static*/ void TerminationDetector::inquireEpochFinished(
  TermFinishedMsg* msg
) {
  theTerm()->inquireFinished(msg->getEpoch(),msg->getFromNode());
}

/*static*/ void TerminationDetector::replyEpochFinished(
  TermFinishedReplyMsg* msg
) {
  theTerm()->replyFinished(msg->getEpoch(),msg->isFinished());
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
  TermStateType& state, TermCounterType const& num_units, bool produce
) {
  auto& counter = produce ? state.l_prod : state.l_cons;
  counter += num_units;

  debug_print(
    term, node,
    "produceConsumeState: epoch={:x}, event_count={}, l_prod={}, l_cons={}, "
    "num_units={}, produce={}\n",
    state.getEpoch(), state.getRecvChildCount(), state.l_prod, state.l_cons, num_units,
    print_bool(produce)
  );

  if (state.readySubmitParent()) {
    propagateEpoch(state);
  }
}

void TerminationDetector::genProd(EpochType const& epoch) {
  vtAssertExpr(epoch != no_epoch);
  if (epoch == term::any_epoch_sentinel) {
    produce(epoch,1);
  } else {
    auto ptr = getDSTerm(epoch);
    if (ptr) {
      vtAbort("Failure: cannot perform a general produce for DS");
    } else {
      produce(epoch,1);
    }
  }
}

void TerminationDetector::genCons(EpochType const& epoch) {
  vtAssertExpr(epoch != no_epoch);
  if (epoch == term::any_epoch_sentinel) {
    consume(epoch,1);
  } else {
    auto ptr = getDSTerm(epoch);
    if (ptr) {
      vtAbort("Failure: cannot perform a general consume for DS");
    } else {
      consume(epoch,1);
    }
  }
}

void TerminationDetector::send(NodeType const& node, EpochType const& epoch) {
  auto ptr = getDSTerm(epoch);
  if (ptr) {
    debug_print(
      termds, node,
      "send: (DS) epoch={:x}, successor={}\n",
      epoch, node
    );
    ptr->msgSent(node);
  }
}

void TerminationDetector::recv(NodeType const& node, EpochType const& epoch) {
  auto ptr = getDSTerm(epoch);
  if (ptr) {
    debug_print(
      termds, node,
      "recv: (DS) epoch={:x}, predecessor={}\n",
      epoch, node
    );
    ptr->msgProcessed(node);
  }
}

TerminationDetector::TermStateDSType*
TerminationDetector::getDSTerm(EpochType const& epoch) {
  if (epoch != no_epoch) {
    auto const is_rooted = epoch::EpochManip::isRooted(epoch);
    debug_print(
      termds, node,
      "getDSTerm: epoch={:x}, no_epoch={:x}, any_epoch_sentinel={:x}, "
      "is_rooted={}\n",
      epoch, no_epoch, any_epoch_sentinel, is_rooted
    );
    if (epoch != any_epoch_sentinel && is_rooted) {
      auto const ds_epoch = epoch::eEpochCategory::DijkstraScholtenEpoch;
      auto const epoch_category = epoch::EpochManip::category(epoch);
      auto const is_ds = epoch_category == ds_epoch;
      debug_print(
        termds, node,
        "getDSTerm: epoch={:x}, category={}, ds_epoch={:x}, "
        "is_rooted={}, is_ds={}\n",
        epoch, epoch_category, ds_epoch, is_rooted, is_ds
      );
      if (is_ds) {
        auto iter = term_.find(epoch);
        if (iter == term_.end()) {
          auto const this_node = theContext()->getNode();
          term_.emplace(
            std::piecewise_construct,
            std::forward_as_tuple(epoch),
            std::forward_as_tuple(
              TerminatorType{epoch,false,this_node}
            )
          );
          iter = term_.find(epoch);
          vtAssert(iter != term_.end(), "Must exist");
        }
        return &iter->second;
      }
    }
  }
  return nullptr;
}

void TerminationDetector::produceConsume(
  EpochType const& epoch, TermCounterType const& num_units, bool produce
) {
  debug_print(
    term, node,
    "produceConsume: epoch={:x}, num_units={}, produce={}\n",
    epoch, num_units, print_bool(produce)
  );

  produceConsumeState(any_epoch_state_, num_units, produce);

  if (epoch != any_epoch_sentinel) {
    auto& state = findOrCreateState(epoch, false);
    produceConsumeState(state, num_units, produce);
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
  debug_print(
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
  debug_print(
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
  bool const& is_root_ = isRoot();
  auto const& parent_ = getParent();

  debug_print(
    term, node,
    "propagateEpoch: epoch={:x}, l_prod={}, l_cons={}, event_count={}, "
    "children={}, is_ready={}\n",
    state.getEpoch(), state.l_prod, state.l_cons, state.getRecvChildCount(),
    state.getNumChildren(), print_bool(is_ready)
  );

  if (is_ready) {
    state.g_prod1 += state.l_prod;
    state.g_cons1 += state.l_cons;

    debug_print(
      term, node,
      "propagateEpoch: epoch={:x}, l_prod={}, l_cons={}, "
      "g_prod1={}, g_cons1={}, event_count={}, children={}\n",
      state.getEpoch(), state.l_prod, state.l_cons, state.g_prod1, state.g_cons1,
      state.getRecvChildCount(), state.getNumChildren()
    );

    if (not is_root_) {
      auto msg = makeSharedMessage<TermCounterMsg>(
        state.getEpoch(), state.g_prod1, state.g_cons1
      );
      theMsg()->setTermMessage(msg);
      theMsg()->sendMsg<TermCounterMsg, propagateEpochHandler>(parent_, msg);

      debug_print(
        term, node,
        "propagateEpoch: sending to parent: {}, msg={}, epoch={:x}, wave={}\n",
        parent_, print_ptr(msg), state.getEpoch(), state.getCurWave()
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
        theMsg()->broadcastMsg<TermMsg, epochFinishedHandler>(msg);

        state.setTerminated();

        epochFinished(state.getEpoch(), false);
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

              auto f1 = fmt::format(
                "{}Termination hang detected:{} {}traversals={} epoch={:x} "
                "produced={}{} {}consumed={}{}\n",
                bred, reset,
                magenta, state.constant_count, state.getEpoch(), state.g_prod1,
                reset, magenta, state.g_cons1, reset
              );
              vt_print(term, "{}", f1);
              state.num_print_constant++;

              #if !backend_check_enabled(production)
                if (state.num_print_constant > 10) {
                  vtAbort(
                    "Hang detected (consumed != produced) for k tree "
                    "traversals, where k=", state.constant_count
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

        debug_print(
          term, node,
          "propagateEpoch [root]: epoch={:x}, wave={}, continue\n",
          state.getEpoch(), state.getCurWave()
        );

        auto msg = makeSharedMessage<TermMsg>(state.getEpoch(), state.getCurWave());
        theMsg()->setTermMessage(msg);
        theMsg()->broadcastMsg<TermMsg, epochContinueHandler>(msg);
      }
    }

    debug_print(
      term, node,
      "propagateEpoch: epoch={:x}, is_root={}: reset counters\n",
      state.getEpoch(), print_bool(is_root_)
    );

    // reset counters
    state.g_prod1 = state.g_cons1 = 0;
    state.submitToParent(is_root_);
  }

  return is_ready;
}

void TerminationDetector::cleanupEpoch(EpochType const& epoch) {
  if (epoch != any_epoch_sentinel) {
    auto epoch_iter = epoch_state_.find(epoch);
    if (epoch_iter != epoch_state_.end()) {
      epoch_state_.erase(epoch_iter);
    }
  }
}

void TerminationDetector::epochFinished(
  EpochType const& epoch, bool const cleanup
) {
  auto const& is_rooted_epoch = epoch::EpochManip::isRooted(epoch);

  debug_print(
    term, node,
    "epochFinished: epoch={:x}, is_rooted_epoch={}\n",
    epoch, is_rooted_epoch
  );

  // Clear all the children epochs that are nested by this epoch (waiting on it
  // to complete)
  auto const ds_epoch = epoch::eEpochCategory::DijkstraScholtenEpoch;
  auto const epoch_category = epoch::EpochManip::category(epoch);
  auto const is_ds = epoch_category == ds_epoch;
  if (!is_rooted_epoch || (is_rooted_epoch && !is_ds)) {
    if (epoch != term::any_epoch_sentinel) {
      auto iter = epoch_state_.find(epoch);
      vtAssertExprInfo(
        iter != epoch_state_.end(), epoch, cleanup, is_rooted_epoch
      );
      if (iter != epoch_state_.end()) {
        iter->second.clearChildren();
      }
    } else {
      // Although in theory the term::any_epoch_sentinel could track all other
      // epochs as children, it does not need for correctness (and this would be
      // expensive)
    }
  } else {
    vtAssertExpr(is_ds);
    vtAssertExpr(ds_epoch == epoch_category);
    auto ptr = getDSTerm(epoch);
    vtAssertExpr(ptr != nullptr);
    if (ptr) {
      ptr->clearChildren();
    }
  }

  triggerAllActions(epoch,epoch_state_);

  if (cleanup) {
    cleanupEpoch(epoch);
  }

  updateResolvedEpochs(epoch, is_rooted_epoch);
}

void TerminationDetector::inquireFinished(
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

  auto const& finished = testEpochFinished(epoch,nullptr);
  auto const& is_ready = finished == TermStatusEnum::Finished;

  vtAssertExprInfo(finished != TermStatusEnum::Remote, epoch, from);

  debug_print(
    term, node,
    "inquireFinished: epoch={:x}, is_rooted={}, root={}, is_ready={}, from={}\n",
    epoch, is_rooted, epoch_root_node, is_ready, from
  );

  auto msg = makeMessage<TermFinishedReplyMsg>(epoch,is_ready);
  theMsg()->sendMsg<TermFinishedReplyMsg,replyEpochFinished>(from,msg.get());
}

void TerminationDetector::replyFinished(
  EpochType const& epoch, bool const& is_finished
) {
  debug_print(
    term, node,
    "replyFinished: epoch={:x}, is_finished={}\n",
    epoch, is_finished
  );
}

void TerminationDetector::updateResolvedEpochs(
  EpochType const& epoch, bool const rooted
) {
  debug_print(
    term, node,
    "updateResolvedEpoch: epoch={:x}, rooted={}, "
    "collective: first={:x}, last={:x}\n",
    epoch, rooted, epoch_coll_->getFirst(), epoch_coll_->getLast()
  );

  if (rooted) {
    auto window = getWindow(epoch);
    window->closeEpoch(epoch);
  } else {
    epoch_coll_->closeEpoch(epoch);
  }
}

TermStatusEnum TerminationDetector::testEpochFinished(
  EpochType const& epoch, ActionType action
) {
  TermStatusEnum status = TermStatusEnum::Pending;
  auto const& is_rooted_epoch = epoch::EpochManip::isRooted(epoch);

  if (is_rooted_epoch) {
    auto const& this_node = theContext()->getNode();
    auto const& root = epoch::EpochManip::node(epoch);
    if (root == this_node) {
      /*
       * The idea here is that if this is executed on the root, it must have
       * valid info on whether the rooted live or finished
       */
      auto window = getWindow(epoch);
      auto is_finished = window->isFinished(epoch);
      if (is_finished) {
        status = TermStatusEnum::Finished;
      }
    } else {
      /*
       * Send a message to the root node to find out whether this epoch is
       * finished or not
       */
      status = TermStatusEnum::Remote;
      auto msg = makeMessage<TermFinishedMsg>(epoch,this_node);
      theMsg()->sendMsg<TermFinishedMsg,inquireEpochFinished>(root,msg.get());
    }
  } else {
    auto const& is_finished = epoch_coll_->isFinished(epoch);
    if (is_finished) {
      status = TermStatusEnum::Finished;
    }
  }

  debug_print(
    term, node,
    "testEpochFinished: epoch={:x}, pending={}, finished={}, remote={}\n",
    epoch, status == TermStatusEnum::Pending, status == TermStatusEnum::Finished,
    status == TermStatusEnum::Remote
  );

  if (action != nullptr) {
    switch (status) {
    case TermStatusEnum::Finished:
      action();
      break;
    case TermStatusEnum::Remote: {
      auto iter = finished_actions_.find(epoch);
      if (iter == finished_actions_.end()) {
        finished_actions_.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(epoch),
          std::forward_as_tuple(std::vector<ActionType>{action})
        );
      } else {
        iter->second.push_back(action);
      }
      break;
    }
    case TermStatusEnum::Pending:
      // do nothing, action does not get triggered
      break;
    default:
      vtAssertInfo(0, "Should not be reachable", epoch);
      break;
    }
  }

  return status;
}

void TerminationDetector::epochContinue(
  EpochType const& epoch, TermWaveType const& wave
) {
  debug_print(
    term, node,
    "epochContinue: epoch={:x}, any={}, wave={}\n",
    epoch, any_epoch_sentinel, wave
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

EpochType TerminationDetector::newEpochCollective(bool const child) {
  auto const& new_epoch = epoch::EpochManip::makeNewEpoch();
  vtAssertExpr(epoch_coll_ != nullptr);
  epoch_coll_->addEpoch(new_epoch);
  auto const from_child = false;
  produce(new_epoch,1);
  propagateNewEpoch(new_epoch, from_child);

  if (child) {
    linkChildEpoch(new_epoch);
  }

  return new_epoch;
}

void TerminationDetector::linkChildEpoch(EpochType const& epoch) {
  // Add the current active epoch in the messenger as a child epoch so the
  // current epoch does not detect termination until the new epoch terminations
  auto const cur_epoch = theMsg()->getEpoch();
  if (cur_epoch != no_epoch && cur_epoch != term::any_epoch_sentinel) {
    auto epoch_iter = epoch_state_.find(cur_epoch);
    vtAssertExpr(epoch_iter != epoch_state_.end());
    auto& state = epoch_iter->second;
    state.addChildEpoch(cur_epoch);
  }
}

void TerminationDetector::finishedEpoch(EpochType const& epoch) {
  auto ready_iter = epoch_ready_.find(epoch);

  debug_print(
    term, node,
    "finishedEpoch: epoch={:x}, exists={}\n",
    epoch, ready_iter != epoch_ready_.end()
  );

  if (ready_iter == epoch_ready_.end()) {
    epoch_ready_.emplace(epoch);
    consume(epoch,1);
  } else {
    /*
     * Do nothing: the epoch as already been in "finished" state on this node
     */
  }

  // Activate the epoch, which is necessary for a rooted epoch
  activateEpoch(epoch);

  debug_print(
    term, node,
    "finishedEpoch: (after consume) epoch={:x}\n",
    epoch
  );
}

EpochType TerminationDetector::newEpochRooted(
  bool const useDS, bool const child
) {
  /*
   *  This method should only be called by the root node for the rooted epoch
   *  identifier, which is distinct and has the node embedded in it to
   *  distinguish it from all other epochs
   */
  if (useDS) {
    auto const& rooted_epoch = epoch::EpochManip::makeNewRootedEpoch(
      false, epoch::eEpochCategory::DijkstraScholtenEpoch
    );
    auto const this_node = theContext()->getNode();
    auto iter = term_.find(rooted_epoch);
    vtAssertInfo(
      iter == term_.end(), "New epoch must not exist", rooted_epoch, useDS
    );
    term_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(rooted_epoch),
      std::forward_as_tuple(
        TerminatorType{rooted_epoch,true,this_node}
      )
    );
    if (child) {
      iter = term_.find(rooted_epoch);
      vtAssertInfo(
        iter != term_.end(), "New epoch must exist now", rooted_epoch, useDS
      );
      iter->second.addChildEpoch(rooted_epoch);
    }
    epoch_ready_.emplace(rooted_epoch);
    return rooted_epoch;
  } else {
    auto const& rooted_epoch = epoch::EpochManip::makeNewRootedEpoch();
    rootMakeEpoch(rooted_epoch,child);
    return rooted_epoch;
  }
}

EpochType TerminationDetector::makeEpochRooted(bool const useDS) {
  return makeEpoch(false,useDS);
}

EpochType TerminationDetector::makeEpochCollective() {
  return makeEpoch(true);
}

EpochType TerminationDetector::makeEpoch(
  bool const is_collective, bool const useDS
) {
  if (is_collective) {
    auto const& epoch = newEpochCollective();
    getWindow(epoch)->addEpoch(epoch);
    return epoch;
  } else {
    auto const& epoch = newEpochRooted(useDS);
    getWindow(epoch)->addEpoch(epoch);
    return epoch;
  }
}

EpochType TerminationDetector::newEpoch(bool const child) {
  return newEpochCollective(child);
}

void TerminationDetector::rootMakeEpoch(
  EpochType const& epoch, bool const child
) {
  debug_print(
    term, node,
    "rootMakeEpoch: root={}, epoch={:x}\n",
    theContext()->getNode(), epoch
  );

  /*
   *  Broadcast new rooted epoch to all other nodes to start processing this
   *  epoch
   */
  auto msg = makeSharedMessage<TermMsg>(epoch);
  theMsg()->setTermMessage(msg);
  theMsg()->broadcastMsg<TermMsg,makeRootedEpoch>(msg);
  /*
   *  Setup the new rooted epoch locally on the root node (this node)
   */
  bool const is_root = true;
  makeRootedEpoch(epoch,is_root);


  if (child) {
    linkChildEpoch(epoch);
  }
}

void TerminationDetector::activateEpoch(EpochType const& epoch) {
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

void TerminationDetector::makeRootedEpoch(
  EpochType const& epoch, bool const is_root
) {
  bool const is_ready = !is_root;
  auto& state = findOrCreateState(epoch, is_ready);

  getWindow(epoch)->addEpoch(epoch);

  debug_print(
    term, node,
    "makeRootedEpoch: epoch={:x}, is_root={}\n", epoch, is_root
  );

  epoch_ready_.emplace(epoch);

  if (!is_root) {
    activateEpoch(epoch);
  }

  if (is_root && state.noLocalUnits()) {
    /*
     *  Do not submit parent at the root if no units have been produced at the
     *  root: a false positive is possible if termination starts immediately
     *  because it may finish before any unit is produced or consumed with the
     *  new rooted epoch
     */
  } else {
    if (state.readySubmitParent()) {
      propagateEpoch(state);
    }
  }
}

void TerminationDetector::setupNewEpoch(
  EpochType const& new_epoch, bool const from_child
) {
  auto epoch_iter = epoch_state_.find(new_epoch);

  bool const found = epoch_iter != epoch_state_.end();

  debug_print(
    term, node,
    "setupNewEpoch: new_epoch={:x}, found={}, count={}\n",
    new_epoch, print_bool(found),
    (found ? epoch_iter->second.getRecvChildCount() : -1)
  );

  auto& state = findOrCreateState(new_epoch, false);

  if (from_child) {
    state.notifyChildReceive();
  } else {
    state.notifyLocalTerminated();
  }
}

void TerminationDetector::propagateNewEpoch(
  EpochType const& new_epoch, bool const from_child
) {
  setupNewEpoch(new_epoch, from_child);

  auto epoch_iter = epoch_state_.find(new_epoch);
  auto& state = epoch_iter->second;
  bool const& is_root_ = isRoot();
  auto const& parent_ = getParent();
  bool const& is_ready = state.readySubmitParent(false);

  debug_print(
    term, node,
    "propagateNewEpoch: is_ready={}, is_root_={}, epoch={}, event_count={}, "
    "children={}\n",
    print_bool(is_ready), print_bool(is_root_), new_epoch,
    state.getRecvChildCount(), state.getNumChildren()
  );

  if (is_ready) {
    auto msg = makeSharedMessage<TermMsg>(new_epoch);
    theMsg()->setTermMessage(msg);

    if (is_root_) {
      // broadcast ready to all
      theMsg()->broadcastMsg<TermMsg, readyEpochHandler>(msg);
    } else {
      // propagate up the tree
      theMsg()->sendMsg<TermMsg, propagateNewEpochHandler>(parent_, msg);
    }

    state.submitToParent(is_root_, true);

    if (is_root_) {
      readyNewEpoch(new_epoch);
    }
  }
}

void TerminationDetector::readyNewEpoch(EpochType const& new_epoch) {
  auto& state = findOrCreateState(new_epoch, true);
  state.activateEpoch();

  debug_print(
    term, node,
    "readyNewEpoch: epoch={:x}: collective: first={:x}, last={:x}\n",
    new_epoch, epoch_coll_->getFirst(), epoch_coll_->getLast()
  );

  if (state.readySubmitParent()) {
    propagateEpoch(state);
  }
}

}} // end namespace vt::term
