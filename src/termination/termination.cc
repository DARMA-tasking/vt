
#include "term_common.h"
#include "termination.h"
#include "messaging/active.h"
#include "collective/collective.h"
#include "scheduler/scheduler.h"

namespace vt { namespace term {

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

void TerminationDetector::setLocalTerminated(bool const local_terminated) {
  any_epoch_state_.notifyLocalTerminated(local_terminated);

  if (local_terminated) {
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
    "produceConsumeState: epoch=%d, event_count=%d, l_prod=%lld, l_cons=%lld, "
    "num_units=%lld, produce=%s\n",
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
    "produceConsume: epoch=%d, num_units=%lld, produce=%s\n",
    epoch, num_units, print_bool(produce)
  );

  produceConsumeState(any_epoch_state_, num_units, produce);

  if (epoch != any_epoch_sentinel) {
    auto& state = findOrCreateState(epoch, false);
    produceConsumeState(state, num_units, produce);
  }
}

void TerminationDetector::maybePropagate() {
  if (any_epoch_state_.readySubmitParent()) {
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
    "propagateEpochExternalState: epoch=%d, prod=%lld, cons=%lld, "
    "event_count=%d\n",
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
    "propagateEpochExternal: epoch=%d, prod=%lld, cons=%lld\n",
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
    "propagateEpoch: epoch=%d, l_prod=%lld, l_cons=%lld, event_count=%d, "
    "children=%d, is_ready=%s\n",
    state.getEpoch(), state.l_prod, state.l_cons, state.getRecvChildCount(),
    state.getNumChildren(), print_bool(is_ready)
  );

  if (is_ready) {
    state.g_prod1 += state.l_prod;
    state.g_cons1 += state.l_cons;

    debug_print(
      term, node,
      "propagateEpoch: epoch=%d, l_prod=%lld, l_cons=%lld, "
      "g_prod1=%lld, g_cons1=%lld, event_count=%d, children=%d\n",
      state.getEpoch(), state.l_prod, state.l_cons, state.g_prod1, state.g_cons1,
      state.getRecvChildCount(), state.getNumChildren()
    );

    if (not is_root_) {
      auto msg = new TermCounterMsg(state.getEpoch(), state.g_prod1, state.g_cons1);
      theMsg()->setTermMessage(msg);
      theMsg()->sendMsg<TermCounterMsg, propagateEpochHandler>(
        parent_, msg, [=] { delete msg; }
      );

      debug_print(
        term, node,
        "propagateEpoch: sending to parent: %d, msg=%p, epoch=%d, wave=%lld\n",
        parent_, msg, state.getEpoch(), state.getCurWave()
      );

    } else /*if (is_root) */ {
      bool const& is_term = state.g_prod1 == state.g_cons1 and
        state.g_prod2 == state.g_cons2 and state.g_prod1 == state.g_prod2;

      // four-counter method implementation
      debug_print(
        term, node,
        "propagateEpoch {root}: epoch=%d, g_prod1=%lld, g_cons1=%lld, "
        "g_prod2=%lld, g_cons2=%lld, detected_term=%d\n",
        state.getEpoch(), state.g_prod1, state.g_cons1, state.g_prod2,
        state.g_cons2, is_term
      );

      if (is_term) {
        auto msg = new TermMsg(state.getEpoch());
        theMsg()->setTermMessage(msg);
        theMsg()->broadcastMsg<TermMsg, epochFinishedHandler>(
          msg, [=] { delete msg; }
        );

        state.setTerminated();

        epochFinished(state.getEpoch(), false);
      } else {
        state.g_prod2 = state.g_prod1;
        state.g_cons2 = state.g_cons1;
        state.g_prod1 = state.g_cons1 = 0;
        state.setCurWave(state.getCurWave() + 1);

        debug_print(
          term, node,
          "propagateEpoch {root}: epoch=%d, wave=%lld, continue\n",
          state.getEpoch(), state.getCurWave()
        );

        auto msg = new TermMsg(state.getEpoch(), state.getCurWave());
        theMsg()->setTermMessage(msg);
        theMsg()->broadcastMsg<TermMsg, epochContinueHandler>(
          msg, [=] { delete msg; }
        );
      }
    }

    debug_print(
      term, node,
      "propagateEpoch: epoch=%d, is_root=%s: reset counters\n",
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
    "epochFinished: epoch=%d\n", epoch
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
    "epochContinue: epoch=%d, any=%d, wave=%lld\n",
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

EpochType TerminationDetector::newEpoch() {
  if (cur_epoch_ == no_epoch) {
    cur_epoch_ = first_epoch;
  }

  EpochType const cur = cur_epoch_;
  cur_epoch_++;

  bool const from_child = false;
  propagateNewEpoch(cur, from_child);

  return cur;
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
    "setupNewEpoch: new_epoch=%d, found=%s, count=%d\n",
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
    "propagateNewEpoch: is_ready=%s, is_root_=%s, epoch=%d, event_count=%d, "
    "children=%d\n",
    print_bool(is_ready), print_bool(is_root_), new_epoch,
    state.getRecvChildCount(), state.getNumChildren()
  );

  if (is_ready) {
    if (is_root_) {
      // broadcast ready to all
      auto msg = new TermMsg(new_epoch);
      theMsg()->setTermMessage(msg);

      theMsg()->broadcastMsg<TermMsg, readyEpochHandler>(
        msg, [=] { delete msg; }
      );
    } else {
      // propagate up the tree
      auto msg = new TermMsg(new_epoch);
      theMsg()->setTermMessage(msg);

      theMsg()->sendMsg<TermMsg, propagateNewEpochHandler>(
        parent_, msg, [=] { delete msg; }
      );
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
    "readyNewEpoch: epoch=%d: first_resolved_epoch=%d, last_resolved_epoch=%d\n",
    new_epoch, first_resolved_epoch_, last_resolved_epoch_
  );

  if (state.readySubmitParent()) {
    propagateEpoch(state);
  }
}

}} // end namespace vt::term
