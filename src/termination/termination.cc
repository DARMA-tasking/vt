
#include "termination.h"
#include "active.h"
#include "collective.h"
#include "scheduler.h"

namespace runtime { namespace term {

/*static*/ void
TerminationDetector::propagate_new_epoch_handler(TermMsg* msg) {
  the_term->propagate_new_epoch(msg->new_epoch);
}

/*static*/ void
TerminationDetector::ready_epoch_handler(TermMsg* msg) {
  the_term->ready_new_epoch(msg->new_epoch);
}

/*static*/ void
TerminationDetector::propagate_epoch_handler(TermCounterMsg* msg) {
  the_term->propagate_epoch_external(msg->epoch, msg->prod, msg->cons);
}

/*static*/ void
TerminationDetector::epoch_finished_handler(TermMsg* msg) {
  the_term->epoch_finished(msg->new_epoch);
}

/*static*/ void
TerminationDetector::epoch_continue_handler(TermMsg* msg) {
  the_term->epoch_continue(msg->new_epoch);
}

/*static*/ void
TerminationDetector::register_default_termination_action() {
  the_term->attach_global_term_action([]{
    debug_print(
      term, node,
      "TD: terminating program\n"
    );

    auto const& this_node = the_context->get_node();

    debug_print(
      term, node,
      "%d: running registered default termination\n", this_node
    );

    CollectiveOps::finalize_context();
    CollectiveOps::finalize_runtime();
  });
}

void
TerminationDetector::produce_consume(epoch_t const& epoch, bool produce) {
  if (produce) {
    any_epoch_state.l_prod++;
  } else {
    any_epoch_state.l_cons++;
  }

  if (epoch != no_epoch) {
    auto epoch_iter = epoch_state.find(epoch);

    if (epoch_iter == epoch_state.end()) {
      epoch_state.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(epoch),
        std::forward_as_tuple(term_state_t{})
      );
      epoch_iter = epoch_state.find(epoch);
    }

    if (produce) {
      epoch_iter->second.l_prod++;
    } else{
      epoch_iter->second.l_cons++;
    }

    if (epoch_iter->second.propagate) {
      propagate_epoch(epoch, epoch_iter->second);
    }
  }

  if (any_epoch_state.propagate) {
    propagate_epoch(epoch, any_epoch_state);
  }
}

void
TerminationDetector::maybe_propagate() {
  if (any_epoch_state.propagate) {
    propagate_epoch(no_epoch, any_epoch_state);
  }

  for (auto&& e : epoch_state) {
    if (e.second.propagate) {
      propagate_epoch(e.first, e.second);
    }
  }
}

void
TerminationDetector::propagate_epoch_external(
  epoch_t const& epoch, term_counter_t const& prod, term_counter_t const& cons
) {
  debug_print(
    term, node,
    "%d: propagate_epoch_external: epoch=%d, prod=%lld, cons=%lld\n",
    my_node, epoch, prod, cons
  );

  any_epoch_state.g_prod1 += prod;
  any_epoch_state.g_cons1 += cons;

  if (epoch != no_epoch) {
    auto epoch_iter = epoch_state.find(epoch);
    if (epoch_iter == epoch_state.end()) {
      epoch_state.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(epoch),
        std::forward_as_tuple(term_state_t{})
      );
      epoch_iter = epoch_state.find(epoch);
    }
    epoch_iter->second.g_prod1 += prod;
    epoch_iter->second.g_cons1 += cons;

    epoch_iter->second.recv_event_count += 1;
    if (epoch_iter->second.propagate) {
      propagate_epoch(epoch, epoch_iter->second);
    }
  }

  any_epoch_state.recv_event_count += 1;
  if (any_epoch_state.propagate) {
    propagate_epoch(epoch, any_epoch_state);
  }
}

bool
TerminationDetector::propagate_epoch(
  epoch_t const& epoch, term_state_t& state
) {
  setup_tree();

  auto const& event_count = state.recv_event_count;

  bool const is_ready = event_count == num_children + 1;
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
    fflush(stdout);

    if (not is_root) {
      auto msg = new TermCounterMsg(epoch, state.g_prod1, state.g_cons1);

      the_msg->set_term_message(msg);
      the_msg->send_msg<TermCounterMsg, propagate_epoch_handler>(
        parent, msg, [=]{ delete msg; }
      );

      debug_print(
        term, node,
        "%d: propagate_epoch: sending to parent: %d\n",
        my_node, parent
      );
      fflush(stdout);

    } else /*if (is_root) */ {
      bool const& is_term =
        state.g_prod1 == state.g_cons1 and
        state.g_prod2 == state.g_cons2 and
        state.g_prod1 == state.g_prod2;
      // four-counter method implementation
      debug_print(
        term, node,
        "%d: propagate_epoch {root}: epoch=%d, g_prod1=%lld, g_cons1=%lld, "
        "g_prod2=%lld, g_cons2=%lld, detected_term=%d\n",
        my_node, epoch, state.g_prod1, state.g_cons1, state.g_prod2, state.g_cons2,
        is_term
      );
      fflush(stdout);

      if (is_term) {
        auto msg = new TermMsg(epoch);
        the_msg->set_term_message(msg);
        the_msg->broadcast_msg<TermMsg, epoch_finished_handler>(msg, [=]{
          delete msg;
        });

        epoch_finished(epoch);
      } else {
        state.g_prod2 = state.g_prod1;
        state.g_cons2 = state.g_cons1;
        state.g_prod1 = state.g_cons1 = 0;

        auto msg = new TermMsg(epoch);
        the_msg->set_term_message(msg);
        the_msg->broadcast_msg<TermMsg, epoch_continue_handler>(msg, [=]{
          delete msg;
        });

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

void
TerminationDetector::epoch_finished(epoch_t const& epoch){
  debug_print(
    term, node,
    "%d: epoch_finished: epoch=%d\n", my_node, epoch
  );
  fflush(stdout);

  trigger_all_actions(epoch);

  // close the epoch window
  if (first_resolved_epoch == epoch) {
    first_resolved_epoch++;
  }
}

void
TerminationDetector::epoch_continue(epoch_t const& epoch){
  debug_print(
    term, node,
    "%d: epoch_continue: epoch=%d\n", my_node, epoch
  );

  if (epoch == no_epoch) {
    any_epoch_state.propagate = true;
  } else /*if (epoch != no_epoch)*/ {
    auto epoch_iter = epoch_state.find(epoch);
    if (epoch_iter != epoch_state.end()) {
      epoch_iter->second.propagate = true;
    }
  }

  //the_sched->register_trigger_once(sched::SchedulerEvent::BeginIdle, []{
  the_term->maybe_propagate();
  //});
}

void
TerminationDetector::trigger_all_epoch_actions(epoch_t const& epoch) {
  auto action_iter = epoch_actions.find(epoch);
  if (action_iter != epoch_actions.end()) {
    for (auto&& action : action_iter->second) {
      action();
    }
    epoch_actions.erase(action_iter);
  }
}

void
TerminationDetector::trigger_all_actions(epoch_t const& epoch) {
  if (epoch == no_epoch) {
    for (auto&& state : epoch_state) {
      trigger_all_epoch_actions(state.first);
    }

    for (auto&& action : global_term_actions) {
      action();
    }

    global_term_actions.clear();
  } else {
    trigger_all_epoch_actions(epoch);
  }
}

epoch_t
TerminationDetector::new_epoch() {
  if (cur_epoch == no_epoch) {
    cur_epoch = first_epoch;
  }

  epoch_t const cur = cur_epoch;
  cur_epoch++;

  propagate_new_epoch(cur);

  return cur;
}

void
TerminationDetector::attach_global_term_action(action_t action) {
  global_term_actions.emplace_back(action);
}

void
TerminationDetector::attach_epoch_term_action(
  epoch_t const& epoch, action_t action
) {
  auto epoch_iter = epoch_actions.find(epoch);
  if (epoch_iter == epoch_actions.end()) {
    epoch_actions.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(epoch),
      std::forward_as_tuple(action_container_t{action})
    );
  } else {
    epoch_iter->second.emplace_back(
      action
    );
  }
}

void
TerminationDetector::setup_new_epoch(epoch_t const& new_epoch) {
  auto epoch_iter = epoch_state.find(new_epoch);
  if (epoch_iter == epoch_state.end()) {
    assert(
      epoch_iter == epoch_state.end() and
      "Epoch should not have been created yet"
    );
    epoch_state.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(new_epoch),
      std::forward_as_tuple(term_state_t{})
    );
    epoch_iter = epoch_state.find(new_epoch);
  }

  epoch_iter->second.recv_event_count += 1;
}

void
TerminationDetector::propagate_new_epoch(epoch_t const& new_epoch) {
  setup_tree();

  setup_new_epoch(new_epoch);

  auto epoch_iter = epoch_state.find(new_epoch);
  auto const& event_count = epoch_iter->second.recv_event_count;

  bool const& is_ready = event_count == num_children + 1;

  if (is_ready and not is_root) {
    // propagate up the tree
    auto msg = new TermMsg(new_epoch);
    the_msg->set_term_message(msg);

    the_msg->send_msg<TermMsg, propagate_new_epoch_handler>(parent, msg, [=]{ delete msg; });
  } else if (is_ready and is_root) {
    // broadcast ready to all
    auto msg = new TermMsg(new_epoch);
    the_msg->set_term_message(msg);

    the_msg->broadcast_msg<TermMsg, ready_epoch_handler>(msg, [=]{ delete msg; });

    ready_new_epoch(new_epoch);
  }
}

void
TerminationDetector::ready_new_epoch(epoch_t const& new_epoch) {
  if (first_resolved_epoch == no_epoch) {
    assert(last_resolved_epoch == no_epoch);
    first_resolved_epoch = 0;
    last_resolved_epoch = new_epoch;
  } else {
    last_resolved_epoch = std::max(new_epoch, last_resolved_epoch);
  }
}



}} //end namespace runtime::term
