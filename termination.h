
#if ! defined __RUNTIME_TRANSPORT_TERMINATION__
#define __RUNTIME_TRANSPORT_TERMINATION__

#include <cstdint>
#include <vector>
#include <unordered_map>

#include "common.h"

namespace runtime { namespace term {

using term_counter_t = uint64_t;

struct TermState {
  term_counter_t prod = 0, cons = 0;

  // when this is equal to num_children+1, ready to propgate
  int recv_event_count = 0, recv_wave_count = 0;
};

static constexpr epoch_t const first_epoch = 1;

struct EpochMsg : runtime::Message {
  epoch_t new_epoch = no_epoch;

  EpochMsg(epoch_t const& in_new_epoch)
    : new_epoch(in_new_epoch)
  { }
};

struct EpochPropagateMsg : runtime::Message {
  epoch_t epoch = no_epoch;
  term_counter_t prod = 0, cons = 0;

  EpochPropagateMsg(
    epoch_t const& in_epoch, term_counter_t const& in_prod,
    term_counter_t const& in_cons
  ) : epoch(in_epoch), prod(in_prod), cons(in_cons)
  { }
};

struct TerminationDetector {
  using term_state_t = TermState;
  using action_container_t = std::vector<action_t>;

  template <typename T>
  using epoch_container_t = std::unordered_map<epoch_t, T>;

  void
  produce(epoch_t const& epoch = no_epoch) {
    produce_consume(epoch, true);
  }

  void
  consume(epoch_t const& epoch = no_epoch) {
    produce_consume(epoch, false);
  }

  void
  produce_consume(epoch_t const& epoch = no_epoch, bool produce = true) {
    if (produce) any_epoch_state.prod++;
    else any_epoch_state.cons++;

    if (epoch != no_epoch) {
      auto epoch_iter = epoch_state.find(epoch);

      if (epoch_iter == epoch_state.end()) {
        epoch_state.emplace(
          std::piecewise_construct,
          std::forward_as_tuple(epoch),
          std::forward_as_tuple(term_state_t{})
        );
      }
      epoch_iter = epoch_state.find(epoch);

      if (produce) epoch_iter->second.prod++;
      else epoch_iter->second.cons++;

      if (epoch_iter->second.cons == epoch_iter->second.prod) {
        propagate_epoch(epoch, epoch_iter->second.prod, epoch_iter->second.cons);
      }
    }

    if (any_epoch_state.prod == any_epoch_state.cons) {
      propagate_epoch(epoch, any_epoch_state.prod, any_epoch_state.cons);
    }
  }

  void
  maybe_propagate() {
    if (any_epoch_state.prod == any_epoch_state.cons) {
      propagate_epoch(
        no_epoch, any_epoch_state.prod, any_epoch_state.cons
      );
    }

    for (auto&& e : epoch_state) {
      if (e.second.prod == e.second.cons) {
        propagate_epoch(
          e.first, e.second.prod, e.second.cons
        );
      }
    }
  }

  void
  propagate_epoch(
    epoch_t const& epoch, term_counter_t const& prod, term_counter_t const& cons
  ) {
    setup_tree();

    auto epoch_iter = epoch_state.find(epoch);

    if (epoch_iter == epoch_state.end()) {
      epoch_state.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(epoch),
        std::forward_as_tuple(term_state_t{})
      );
    }
    epoch_iter = epoch_state.find(epoch);

    if (epoch_iter != epoch_state.end()) {
      auto const& event_count = epoch_iter->second.recv_event_count;

      // aggregate counts of events for epoch
      epoch_iter->second.prod += prod;
      epoch_iter->second.cons += cons;

      if (not is_root and event_count == num_children + 1 and
          epoch_iter->second.prod == epoch_iter->second.cons) {
        auto msg = new EpochPropagateMsg(epoch, prod, cons);
        the_msg->set_term_message(msg);
        the_msg->send_msg(parent, propagate_epoch_han, msg, [=]{
          delete msg;
        });
      } else if (is_root and event_count == num_children + 1) {
        // four-counter method implementation
        root_term_state2 = root_term_state1;
        root_term_state1.prod = prod;
        root_term_state1.cons = cons;

        if (root_term_state1.prod == root_term_state1.cons and
            root_term_state2.prod == root_term_state2.cons and
            root_term_state1.prod == root_term_state2.prod
           ) {
          auto msg = new EpochMsg(epoch);
          the_msg->broadcast_msg(epoch_finished_han, msg, [=]{
            delete msg;
          });

          trigger_all_actions(epoch);
        }
      }
    }
  }

  void
  epoch_finished(epoch_t const& epoch){
    trigger_all_actions(epoch);
    // close the epoch window
    if (first_resolved_epoch == epoch) {
      first_resolved_epoch++;
    }
  }

  void
  trigger_all_actions(epoch_t const& epoch) {
    if (epoch == no_epoch) {
      for (auto&& action : global_term_actions) {
        action();
      }
      global_term_actions.clear();
    } else {
      auto action_iter = epoch_actions.find(epoch);
      if (action_iter != epoch_actions.end()) {
        for (auto&& action : action_iter->second) {
          action();
        }
        epoch_actions.erase(action_iter);
      }
    }
  }

  epoch_t new_epoch() {
    if (cur_epoch == no_epoch) {
      cur_epoch = first_epoch;
    }

    epoch_t const cur = cur_epoch;
    cur_epoch++;

    propagate_new_epoch(cur);

    return cur;
  }

  void
  attach_global_term_action(action_t action) {
    global_term_actions.emplace_back(action);
  }

  void
  attach_epoch_term_action(epoch_t const& epoch, action_t action) {
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

  static void
  register_termination_handlers();

  static void
  register_default_termination_action();

private:
  void
  setup_tree() {
    if (not set_up_tree) {
      my_node = the_context->get_node();
      num_nodes = the_context->get_num_nodes();

      c1 = my_node*2+1;
      c2 = my_node*2+2;
      has_c1 = c1 < num_nodes;
      has_c2 = c2 < num_nodes;
      num_children = has_c1 + has_c2;

      is_root = my_node == 0;

      if (not is_root) {
        parent = my_node / 2;
      }

      set_up_tree = true;
    }
  }

  void
  setup_new_epoch(epoch_t const& new_epoch) {
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
  propagate_new_epoch(epoch_t const& new_epoch) {
    setup_tree();

    setup_new_epoch(new_epoch);

    auto epoch_iter = epoch_state.find(new_epoch);
    auto const& event_count = epoch_iter->second.recv_event_count;

    if (event_count == num_children + 1 and not is_root) {
      // propagate up the tree
      auto msg = new EpochMsg(new_epoch);
      the_msg->set_term_message(msg);
      the_msg->send_msg(parent, new_epoch_han, msg, [=]{
        delete msg;
      });
    } else if (is_root) {
      // broadcast ready to all
      auto msg = new EpochMsg(new_epoch);
      the_msg->set_term_message(msg);
      the_msg->broadcast_msg(ready_epoch_han, msg, [=]{
        delete msg;
      });
      ready_new_epoch(new_epoch);
    }
  }

  void
  ready_new_epoch(epoch_t const& new_epoch) {
    if (first_resolved_epoch == no_epoch) {
      assert(last_resolved_epoch == no_epoch);
      first_resolved_epoch = 0;
      last_resolved_epoch = new_epoch;
    } else {
      last_resolved_epoch = std::max(new_epoch, last_resolved_epoch);
    }
  }

  bool set_up_tree = false;
  node_t c1 = -1, c2 = -1, parent = -1;
  node_t num_children = 0, my_node = 0, num_nodes = 0;
  bool has_c1 = false, has_c2 = false, is_root = false;

  handler_t new_epoch_han = uninitialized_handler;
  handler_t propagate_epoch_han = uninitialized_handler;
  handler_t epoch_finished_han = uninitialized_handler;
  handler_t ready_epoch_han = uninitialized_handler;

private:
  epoch_t cur_epoch = no_epoch;

  // the epoch window [fst, lst]
  epoch_t first_resolved_epoch = no_epoch, last_resolved_epoch = no_epoch;

  // global termination state
  term_state_t any_epoch_state;

  term_state_t root_term_state1, root_term_state2;

  // epoch termination state
  epoch_container_t<term_state_t> epoch_state;

  // action container for global termination
  action_container_t global_term_actions;

  // action container for each epoch
  epoch_container_t<action_container_t> epoch_actions;
};

}} //end namespace runtime::term


namespace runtime {

extern std::unique_ptr<term::TerminationDetector> the_term;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_TERMINATION__*/
