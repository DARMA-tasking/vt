
#include "active.h"
#include "collective.h"
#include "scheduler.h"
#include "termination.h"

namespace vt { namespace term {

/*static*/ void TerminationDetector::propagate_new_epoch_handler(TermMsg* msg) {
  the_term->propagate_new_epoch(msg->new_epoch);
}

/*static*/ void TerminationDetector::ready_epoch_handler(TermMsg* msg) {
  the_term->ready_new_epoch(msg->new_epoch);
}

/*static*/ void
TerminationDetector::propagate_epoch_handler(TermCounterMsg* msg) {
  the_term->propagate_epoch_external(msg->epoch, msg->prod, msg->cons);
}

/*static*/ void TerminationDetector::epoch_finished_handler(TermMsg* msg) {
  the_term->epoch_finished(msg->new_epoch);
}

/*static*/ void TerminationDetector::epoch_continue_handler(TermMsg* msg) {
  the_term->epoch_continue(msg->new_epoch);
}

/*static*/ void TerminationDetector::register_default_termination_action() {
  the_term->attach_global_term_action([] {
    debug_print(
      term, node,
      "running registered default termination\n",
    );

    CollectiveOps::finalize_context();
    CollectiveOps::finalize_runtime();
  });
}

void TerminationDetector::produce_consume(
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
      propagate_epoch(epoch, epoch_iter->second);
    }
  }

  if (any_epoch_state_.propagate) {
    propagate_epoch(epoch, any_epoch_state_);
  }
}

void TerminationDetector::maybe_propagate() {
  if (any_epoch_state_.propagate) {
    propagate_epoch(no_epoch, any_epoch_state_);
  }

  for (auto&& e : epoch_state_) {
    if (e.second.propagate) {
      propagate_epoch(e.first, e.second);
    }
  }
}

void TerminationDetector::propagate_epoch_external(
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
      propagate_epoch(epoch, epoch_iter->second);
    }
  }

  any_epoch_state_.recv_event_count += 1;
  if (any_epoch_state_.propagate) {
    propagate_epoch(epoch, any_epoch_state_);
  }
}

bool TerminationDetector::propagate_epoch(
  EpochType const& epoch, TermStateType& state
) {
  setup_tree();

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

      the_msg->set_term_message(msg);
      the_msg->send_msg<TermCounterMsg, propagate_epoch_handler>(
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
        the_msg->set_term_message(msg);
        the_msg->broadcast_msg<TermMsg, epoch_finished_handler>(
          msg, [=] { delete msg; }
        );

        epoch_finished(epoch);
      } else {
        state.g_prod2 = state.g_prod1;
        state.g_cons2 = state.g_cons1;
        state.g_prod1 = state.g_cons1 = 0;

        auto msg = new TermMsg(epoch);
        the_msg->set_term_message(msg);
        the_msg->broadcast_msg<TermMsg, epoch_continue_handler>(
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

void TerminationDetector::epoch_finished(EpochType const& epoch) {
  debug_print(
    term, node,
    "%d: epoch_finished: epoch=%d\n",
    my_node, epoch
  );

  trigger_all_actions(epoch);

  // close the epoch window
  if (first_resolved_epoch_ == epoch) {
    first_resolved_epoch_++;
  }
}

void TerminationDetector::epoch_continue(EpochType const& epoch) {
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

  // the_sched->register_trigger_once(sched::SchedulerEvent::BeginIdle, []{
  the_term->maybe_propagate();
  //});
}

void TerminationDetector::trigger_all_epoch_actions(EpochType const& epoch) {
  auto action_iter = epoch_actions_.find(epoch);
  if (action_iter != epoch_actions_.end()) {
    for (auto&& action : action_iter->second) {
      action();
    }
    epoch_actions_.erase(action_iter);
  }
}

void TerminationDetector::trigger_all_actions(EpochType const& epoch) {
  if (epoch == no_epoch) {
    for (auto&& state : epoch_state_) {
      trigger_all_epoch_actions(state.first);
    }

    for (auto&& action : global_term_actions_) {
      action();
    }

    global_term_actions_.clear();
  } else {
    trigger_all_epoch_actions(epoch);
  }
}

EpochType TerminationDetector::new_epoch() {
  if (cur_epoch_ == no_epoch) {
    cur_epoch_ = first_epoch;
  }

  EpochType const cur = cur_epoch_;
  cur_epoch_++;

  propagate_new_epoch(cur);

  return cur;
}

void TerminationDetector::attach_global_term_action(ActionType action) {
  global_term_actions_.emplace_back(action);
}

void TerminationDetector::attach_epoch_term_action(
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

void TerminationDetector::setup_new_epoch(EpochType const& new_epoch) {
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

void TerminationDetector::propagate_new_epoch(EpochType const& new_epoch) {
  setup_tree();

  setup_new_epoch(new_epoch);

  auto epoch_iter = epoch_state_.find(new_epoch);
  auto const& event_count = epoch_iter->second.recv_event_count;

  bool const& is_ready = event_count == num_children_ + 1;

  if (is_ready and not is_root_) {
    // propagate up the tree
    auto msg = new TermMsg(new_epoch);
    the_msg->set_term_message(msg);

    the_msg->send_msg<TermMsg, propagate_new_epoch_handler>(
      parent_, msg, [=] { delete msg; }
    );
  } else if (is_ready and is_root_) {
    // broadcast ready to all
    auto msg = new TermMsg(new_epoch);
    the_msg->set_term_message(msg);

    the_msg->broadcast_msg<TermMsg, ready_epoch_handler>(
      msg, [=] { delete msg; }
    );

    ready_new_epoch(new_epoch);
  }
}

void TerminationDetector::ready_new_epoch(EpochType const& new_epoch) {
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
