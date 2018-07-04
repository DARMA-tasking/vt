
#include "term_common.h"
#include "termination.h"
#include "messaging/active.h"
#include "collective/collective_ops.h"
#include "scheduler/scheduler.h"

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

/*static*/ void TerminationDetector::registerDefaultTerminationAction(
  ActionType default_action
) {
  debug_print(
    term, node,
    "registering default termination action\n",
  );

  theTerm()->attachGlobalTermAction(default_action);
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
  debug_print(
    term, node,
    "epochFinished: epoch={}\n", epoch
  );

  triggerAllActions(epoch);

  if (cleanup) {
    cleanupEpoch(epoch);
  }

  // close the epoch window
  if (first_resolved_epoch_ == epoch) {
    first_resolved_epoch_++;
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

void TerminationDetector::triggerAllEpochActions(EpochType const& epoch) {
  auto action_iter = epoch_actions_.find(epoch);
  if (action_iter != epoch_actions_.end()) {
    auto const& size = action_iter->second.size();
    for (auto&& action : action_iter->second) {
      action();
    }
    epoch_actions_.erase(action_iter);
    consume(any_epoch_sentinel, size);
  }
}

void TerminationDetector::triggerAllActions(EpochType const& epoch) {
  if (epoch == any_epoch_sentinel) {
    for (auto&& state : epoch_state_) {
      triggerAllEpochActions(state.first);
    }

    for (auto&& action : global_term_actions_) {
      action();
    }

    global_term_actions_.clear();
  } else {
    triggerAllEpochActions(epoch);
  }
}

EpochType TerminationDetector::newEpochCollective() {
  if (cur_epoch_ == no_epoch) {
    auto const& is_rooted = false;
    cur_rooted_epoch_ = epoch::EpochManip::makeEpoch(first_epoch, is_rooted);
  }

  EpochType const cur = cur_epoch_;
  cur_epoch_++;

  auto const from_child = false;
  propagateNewEpoch(cur, from_child);

  return cur;
}

EpochType TerminationDetector::newEpochRooted() {
  if (cur_rooted_epoch_ == no_epoch) {
    auto const& is_rooted = true;
    auto const& root_node = theContext()->getNode();
    cur_rooted_epoch_ = epoch::EpochManip::makeEpoch(
      first_epoch, is_rooted, root_node
    );
  } else {
    cur_rooted_epoch_++;
  }

  EpochType const cur = cur_rooted_epoch_;
  auto const from_child = false;
  propagateNewEpoch(cur, from_child);

  return cur;
}

EpochType TerminationDetector::newEpoch() {
  return newEpochCollective();
}

void TerminationDetector::attachGlobalTermAction(ActionType action) {
  global_term_actions_.emplace_back(action);
}

void TerminationDetector::forceGlobalTermAction(ActionType action) {
  global_term_actions_.clear();
  global_term_actions_.emplace_back(action);
}

void TerminationDetector::attachEpochTermAction(
  EpochType const& epoch, ActionType action
) {
  auto epoch_iter = epoch_actions_.find(epoch);
  if (epoch_iter == epoch_actions_.end()) {
    epoch_actions_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(epoch),
      std::forward_as_tuple(ActionContainerType{action})
    );
  } else {
    epoch_iter->second.emplace_back(action);
  }
  // inhibit global termination from being reached when an epoch has a
  // registered action
  produce();
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
