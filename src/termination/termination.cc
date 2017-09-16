
#include "active.h"
#include "collective.h"
#include "scheduler.h"
#include "termination.h"

namespace vt { namespace term {

/*static*/ void TerminationDetector::propagateNewEpochHandler(TermMsg* msg) {
  theTerm->propagateNewEpoch(msg->new_epoch);
}

/*static*/ void TerminationDetector::readyEpochHandler(TermMsg* msg) {
  theTerm->readyNewEpoch(msg->new_epoch);
}

/*static*/ void
TerminationDetector::propagateEpochHandler(TermCounterMsg* msg) {
  theTerm->propagateEpochExternal(msg->epoch, msg->prod, msg->cons);
}

/*static*/ void TerminationDetector::epochFinishedHandler(TermMsg* msg) {
  theTerm->epochFinished(msg->new_epoch);
}

/*static*/ void TerminationDetector::epochContinueHandler(TermMsg* msg) {
  theTerm->epochContinue(msg->new_epoch);
}

/*static*/ void TerminationDetector::registerDefaultTerminationAction() {
  theTerm->attachGlobalTermAction([] {
    debug_print(
      term, node,
      "running registered default termination\n",
    );

    CollectiveOps::setInactiveState();
  });
}

void TerminationDetector::produceConsume(
  EpochType const& epoch, bool produce) {
  if (produce) {
    any_epoch_state_.l_prod++;
  } else {
    any_epoch_state_.l_cons++;
  }

  if (epoch != no_epoch) {
    auto epoch_iter = epoch_state_.find(epoch);

    if (epoch_iter == epoch_state_.end()) {
      epoch_state_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(epoch),
        std::forward_as_tuple(TermStateType{})
      );
      epoch_iter = epoch_state_.find(epoch);
    }

    if (produce) {
      epoch_iter->second.l_prod++;
    } else {
      epoch_iter->second.l_cons++;
    }

    if (epoch_iter->second.propagate) {
      propagateEpoch(epoch, epoch_iter->second);
    }
  }

  if (any_epoch_state_.propagate) {
    propagateEpoch(epoch, any_epoch_state_);
  }
}

void TerminationDetector::maybePropagate() {
  if (any_epoch_state_.propagate) {
    propagateEpoch(no_epoch, any_epoch_state_);
  }

  for (auto&& e : epoch_state_) {
    if (e.second.propagate) {
      propagateEpoch(e.first, e.second);
    }
  }
}

void TerminationDetector::propagateEpochExternal(
  EpochType const& epoch, TermCounterType const& prod,
  TermCounterType const& cons
) {
  debug_print(
    term, node,
    "%d: propagate_epoch_external: epoch=%d, prod=%lld, cons=%lld\n",
    my_node, epoch, prod, cons
  );

  any_epoch_state_.g_prod1 += prod;
  any_epoch_state_.g_cons1 += cons;

  if (epoch != no_epoch) {
    auto epoch_iter = epoch_state_.find(epoch);
    if (epoch_iter == epoch_state_.end()) {
      epoch_state_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(epoch),
        std::forward_as_tuple(TermStateType{})
      );
      epoch_iter = epoch_state_.find(epoch);
    }
    epoch_iter->second.g_prod1 += prod;
    epoch_iter->second.g_cons1 += cons;

    epoch_iter->second.recv_event_count += 1;
    if (epoch_iter->second.propagate) {
      propagateEpoch(epoch, epoch_iter->second);
    }
  }

  any_epoch_state_.recv_event_count += 1;
  if (any_epoch_state_.propagate) {
    propagateEpoch(epoch, any_epoch_state_);
  }
}

bool TerminationDetector::propagateEpoch(
  EpochType const& epoch, TermStateType& state
) {
  setupTree();

  auto const& event_count = state.recv_event_count;

  bool const is_ready = event_count == num_children_ + 1;
  bool prop_continue = false;

  debug_print(
    term, node,
    "%d: propagate_epoch: epoch=%d, l_prod=%lld, l_cons=%lld, ec=%d, nc=%d\n",
    my_node, epoch, state.l_prod, state.l_cons, event_count, num_children
  );

  if (is_ready) {
    assert(state.propagate);

    state.g_prod1 += state.l_prod;
    state.g_cons1 += state.l_cons;

    debug_print(
      term, node,
      "%d: propagate_epoch: epoch=%d, l_prod=%lld, l_cons=%lld, "
      "g_prod1=%lld, g_cons1=%lld, event_count=%d, num_children=%d\n",
      my_node, epoch, state.l_prod, state.l_cons, state.g_prod1, state.g_cons1,
      event_count, num_children
    );

    if (not is_root_) {
      auto msg = new TermCounterMsg(epoch, state.g_prod1, state.g_cons1);

      theMsg->setTermMessage(msg);
      theMsg->sendMsg<TermCounterMsg, propagateEpochHandler>(
        parent_, msg, [=] { delete msg; }
      );

      debug_print(
        term, node, "%d: propagate_epoch: sending to parent: %d\n", my_node,
        parent
      );

    } else /*if (is_root) */ {
      bool const& is_term = state.g_prod1 == state.g_cons1 and
        state.g_prod2 == state.g_cons2 and state.g_prod1 == state.g_prod2;

      // four-counter method implementation
      debug_print(
        term, node,
        "%d: propagate_epoch {root}: epoch=%d, g_prod1=%lld, g_cons1=%lld, "
        "g_prod2=%lld, g_cons2=%lld, detected_term=%d\n",
        my_node, epoch, state.g_prod1, state.g_cons1, state.g_prod2,
        state.g_cons2, is_term
      );

      if (is_term) {
        auto msg = new TermMsg(epoch);
        theMsg->setTermMessage(msg);
        theMsg->broadcastMsg<TermMsg, epochFinishedHandler>(
          msg, [=] { delete msg; }
        );

        epochFinished(epoch);
      } else {
        state.g_prod2 = state.g_prod1;
        state.g_cons2 = state.g_cons1;
        state.g_prod1 = state.g_cons1 = 0;

        auto msg = new TermMsg(epoch);
        theMsg->setTermMessage(msg);
        theMsg->broadcastMsg<TermMsg, epochContinueHandler>(
          msg, [=] { delete msg; }
        );

        prop_continue = true;
      }
    }

    // reset counters
    state.g_prod1 = state.g_cons1 = 0;
    state.recv_event_count = 1;
    state.propagate = prop_continue;
  }

  return is_ready;
}

void TerminationDetector::epochFinished(EpochType const& epoch) {
  debug_print(
    term, node,
    "%d: epoch_finished: epoch=%d\n",
    my_node, epoch
  );

  triggerAllActions(epoch);

  // close the epoch window
  if (first_resolved_epoch_ == epoch) {
    first_resolved_epoch_++;
  }
}

void TerminationDetector::epochContinue(EpochType const& epoch) {
  debug_print(
    term, node,
    "%d: epoch_continue: epoch=%d\n",
    my_node, epoch
  );

  if (epoch == no_epoch) {
    any_epoch_state_.propagate = true;
  } else /*if (epoch != no_epoch)*/ {
    auto epoch_iter = epoch_state_.find(epoch);
    if (epoch_iter != epoch_state_.end()) {
      epoch_iter->second.propagate = true;
    }
  }

  // theSched->register_trigger_once(sched::SchedulerEvent::BeginIdle, []{
  theTerm->maybePropagate();
  //});
}

void TerminationDetector::triggerAllEpochActions(EpochType const& epoch) {
  auto action_iter = epoch_actions_.find(epoch);
  if (action_iter != epoch_actions_.end()) {
    for (auto&& action : action_iter->second) {
      action();
    }
    epoch_actions_.erase(action_iter);
  }
}

void TerminationDetector::triggerAllActions(EpochType const& epoch) {
  if (epoch == no_epoch) {
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

  propagateNewEpoch(cur);

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
  EpochType const& epoch, ActionType action) {
  auto epoch_iter = epoch_actions_.find(epoch);
  if (epoch_iter == epoch_actions_.end()) {
    epoch_actions_.emplace(
      std::piecewise_construct, std::forward_as_tuple(epoch),
      std::forward_as_tuple(ActionContainerType{action}));
  } else {
    epoch_iter->second.emplace_back(action);
  }
}

void TerminationDetector::setupNewEpoch(EpochType const& new_epoch) {
  auto epoch_iter = epoch_state_.find(new_epoch);
  if (epoch_iter == epoch_state_.end()) {
    assert(
      epoch_iter == epoch_state_.end() and
      "Epoch should not have been created yet");
    epoch_state_.emplace(
      std::piecewise_construct, std::forward_as_tuple(new_epoch),
      std::forward_as_tuple(TermStateType{}));
    epoch_iter = epoch_state_.find(new_epoch);
  }

  epoch_iter->second.recv_event_count += 1;
}

void TerminationDetector::propagateNewEpoch(EpochType const& new_epoch) {
  setupTree();

  setupNewEpoch(new_epoch);

  auto epoch_iter = epoch_state_.find(new_epoch);
  auto const& event_count = epoch_iter->second.recv_event_count;

  bool const& is_ready = event_count == num_children_ + 1;

  if (is_ready and not is_root_) {
    // propagate up the tree
    auto msg = new TermMsg(new_epoch);
    theMsg->setTermMessage(msg);

    theMsg->sendMsg<TermMsg, propagateNewEpochHandler>(
      parent_, msg, [=] { delete msg; }
    );
  } else if (is_ready and is_root_) {
    // broadcast ready to all
    auto msg = new TermMsg(new_epoch);
    theMsg->setTermMessage(msg);

    theMsg->broadcastMsg<TermMsg, readyEpochHandler>(
      msg, [=] { delete msg; }
    );

    readyNewEpoch(new_epoch);
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
}
}
} // end namespace vt::term
