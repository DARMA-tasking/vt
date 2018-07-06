
#include "config.h"
#include "termination/termination.h"
#include "termination/term_common.h"
#include "messaging/active.h"
#include "collective/collective_ops.h"
#include "scheduler/scheduler.h"
#include "epoch/epoch_headers.h"

namespace vt { namespace term {

TerminationDetector::TerminationDetector()
  : collective::tree::Tree(collective::tree::tree_cons_tag_t),
  any_epoch_state_(any_epoch_sentinel, false, true, getNumChildren())
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

  assert(epoch != any_epoch_sentinel and "Should not be any epoch");

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
    "produceConsumeState: epoch={}, event_count={}, l_prod={}, l_cons={}, "
    "num_units={}, produce={}\n",
    state.getEpoch(), state.getRecvChildCount(), state.l_prod, state.l_cons, num_units,
    print_bool(produce)
  );

  if (state.readySubmitParent()) {
    propagateEpoch(state);
  }
}

void TerminationDetector::produceConsume(
  EpochType const& epoch, TermCounterType const& num_units, bool produce
) {
  debug_print(
    term, node,
    "produceConsume: epoch={}, num_units={}, produce={}\n",
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

  for (auto&& e : epoch_state_) {
    if (e.second.readySubmitParent()) {
      propagateEpoch(e.second);
    }
  }
}

void TerminationDetector::propagateEpochExternalState(
  TermStateType& state, TermCounterType const& prod, TermCounterType const& cons
) {
  debug_print(
    term, node,
    "propagateEpochExternalState: epoch={}, prod={}, cons={}, "
    "event_count={}\n",
    state.getEpoch(), prod, cons, state.getRecvChildCount()
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
    "propagateEpochExternal: epoch={}, prod={}, cons={}\n",
    epoch, prod, cons
  );

  if (epoch == any_epoch_sentinel) {
    propagateEpochExternalState(any_epoch_state_, prod, cons);
  } else {
    auto& state = findOrCreateState(epoch, false);
    propagateEpochExternalState(state, prod, cons);
  }
}

bool TerminationDetector::propagateEpoch(TermStateType& state) {
  bool const& is_ready = state.readySubmitParent();
  bool const& is_root_ = isRoot();
  auto const& parent_ = getParent();

  debug_print(
    term, node,
    "propagateEpoch: epoch={}, l_prod={}, l_cons={}, event_count={}, "
    "children={}, is_ready={}\n",
    state.getEpoch(), state.l_prod, state.l_cons, state.getRecvChildCount(),
    state.getNumChildren(), print_bool(is_ready)
  );

  if (is_ready) {
    state.g_prod1 += state.l_prod;
    state.g_cons1 += state.l_cons;

    debug_print(
      term, node,
      "propagateEpoch: epoch={}, l_prod={}, l_cons={}, "
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
        "propagateEpoch: sending to parent: {}, msg={}, epoch={}, wave={}\n",
        parent_, print_ptr(msg), state.getEpoch(), state.getCurWave()
      );

    } else /*if (is_root) */ {
      bool const& is_term = state.g_prod1 == state.g_cons1 and
        state.g_prod2 == state.g_cons2 and state.g_prod1 == state.g_prod2;

      // four-counter method implementation
      debug_print(
        term, node,
        "propagateEpoch [root]: epoch={}, g_prod1={}, g_cons1={}, "
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
        state.g_prod2 = state.g_prod1;
        state.g_cons2 = state.g_cons1;
        state.g_prod1 = state.g_cons1 = 0;
        state.setCurWave(state.getCurWave() + 1);

        debug_print(
          term, node,
          "propagateEpoch [root]: epoch={}, wave={}, continue\n",
          state.getEpoch(), state.getCurWave()
        );

        auto msg = makeSharedMessage<TermMsg>(state.getEpoch(), state.getCurWave());
        theMsg()->setTermMessage(msg);
        theMsg()->broadcastMsg<TermMsg, epochContinueHandler>(msg);
      }
    }

    debug_print(
      term, node,
      "propagateEpoch: epoch={}, is_root={}: reset counters\n",
      state.getEpoch(), print_bool(is_root_)
    );

    // reset counters
    state.g_prod1 = state.g_cons1 = 0;
    state.submitToParent(is_root_);

    if (state.isTerminated()) {
      cleanupEpoch(state.getEpoch());
    }
  }

  return is_ready;
}

void TerminationDetector::cleanupEpoch(EpochType const& epoch) {
  if (epoch != any_epoch_sentinel) {
    auto epoch_iter = epoch_state_.find(epoch);
    assert(epoch_iter != epoch_state_.end() and "Must exist");
    epoch_state_.erase(epoch_iter);
  }
}

void TerminationDetector::epochFinished(
  EpochType const& epoch, bool const cleanup
) {
  auto const& is_rooted_epoch = epoch::EpochManip::isRooted(epoch);

  debug_print(
    term, node,
    "epochFinished: epoch={}, is_rooted_epoch={}\n",
    epoch, is_rooted_epoch
  );

  triggerAllActions(epoch,epoch_state_);

  if (cleanup) {
    cleanupEpoch(epoch);
  }

  /*
   *  Rooted epochs are not tracked in the window because they are not purely
   *  sequential
   */
  if (!is_rooted_epoch) {
    // close the epoch window
    if (first_resolved_epoch_ == epoch) {
      first_resolved_epoch_++;
    }
  }
}

void TerminationDetector::epochContinue(
  EpochType const& epoch, TermWaveType const& wave
) {
  debug_print(
    term, node,
    "epochContinue: epoch={}, any={}, wave={}\n",
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

EpochType TerminationDetector::newEpochCollective() {
  auto const& epoch = epoch::EpochManip::makeNewEpoch();
  auto const from_child = false;
  propagateNewEpoch(epoch, from_child);
  return epoch;
}

EpochType TerminationDetector::newEpochRooted() {
  auto const& rooted_epoch = epoch::EpochManip::makeNewRootedEpoch();
  /*
   *  This method should only be called by the root node for the rooted epoch
   *  identifier, which is distinct and has the node embedded in it to
   *  distinguish it from all other epochs
   */
  rootMakeEpoch(rooted_epoch);
  return rooted_epoch;
}

EpochType TerminationDetector::makeEpochRooted() {
  return makeEpoch(false);
}

EpochType TerminationDetector::makeEpochCollective() {
  return makeEpoch(true);
}

EpochType TerminationDetector::makeEpoch(bool const is_collective) {
  if (is_collective) {
    return newEpochCollective();
  } else {
    return newEpochRooted();
  }
}

EpochType TerminationDetector::newEpoch() {
  return newEpochCollective();
}

void TerminationDetector::rootMakeEpoch(EpochType const& epoch) {
  debug_print(
    term, node,
    "rootMakeEpoch: root={}, epoch={}\n",
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
}

void TerminationDetector::activateEpoch(EpochType const& epoch) {
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

  debug_print(
    term, node,
    "makeRootedEpoch: epoch={}, is_root={}\n", epoch, is_root
  );

  if (!is_root) {
    state.activateEpoch();
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
    "setupNewEpoch: new_epoch={}, found={}, count={}\n",
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
  if (first_resolved_epoch_ == no_epoch) {
    assert(last_resolved_epoch_ == no_epoch);
    first_resolved_epoch_ = 0;
    last_resolved_epoch_ = new_epoch;
  } else {
    last_resolved_epoch_ = std::max(new_epoch, last_resolved_epoch_);
  }

  auto& state = findOrCreateState(new_epoch, true);
  state.activateEpoch();

  debug_print(
    term, node,
    "readyNewEpoch: epoch={}: first_resolved_epoch={}, last_resolved_epoch={}\n",
    new_epoch, first_resolved_epoch_, last_resolved_epoch_
  );

  if (state.readySubmitParent()) {
    propagateEpoch(state);
  }
}

}} // end namespace vt::term
