
#if ! defined __RUNTIME_TRANSPORT_TERMINATION__
#define __RUNTIME_TRANSPORT_TERMINATION__

#include <cstdint>
#include <vector>
#include <unordered_map>

#include "common.h"
#include "function.h"
#include "message.h"
#include "tree.h"
#include "term_state.h"
#include "epoch.h"

namespace runtime { namespace term {

using namespace runtime::epoch;

struct TerminationDetector : Tree {
  using term_state_t = TermState;
  using action_container_t = std::vector<action_t>;

  TerminationDetector() {
    any_epoch_state.recv_event_count = 1;
  }

  template <typename T>
  using epoch_container_t = std::unordered_map<epoch_t, T>;

  inline void
  produce(epoch_t const& epoch = no_epoch) {
    produce_consume(epoch, true);
  }

  inline void
  consume(epoch_t const& epoch = no_epoch) {
    produce_consume(epoch, false);
  }

  void
  produce_consume(epoch_t const& epoch = no_epoch, bool produce = true);

  void
  maybe_propagate();

  void
  propagate_epoch_external(
    epoch_t const& epoch, term_counter_t const& prod, term_counter_t const& cons
  );

  bool
  propagate_epoch(epoch_t const& epoch, term_state_t& state);

  void
  epoch_finished(epoch_t const& epoch);

  void
  epoch_continue(epoch_t const& epoch, int const& wave);

  void
  trigger_all_epoch_actions(epoch_t const& epoch);

  void
  trigger_all_actions(epoch_t const& epoch);

  epoch_t
  new_epoch();

  void
  attach_global_term_action(action_t action);

  void
  attach_epoch_term_action(epoch_t const& epoch, action_t action);

  static void
  register_termination_handlers();

  static void
  register_default_termination_action();

private:
  void
  setup_new_epoch(epoch_t const& new_epoch);

  void
  propagate_new_epoch(epoch_t const& new_epoch);

  void
  ready_new_epoch(epoch_t const& new_epoch);

  handler_t new_epoch_han = uninitialized_handler;
  handler_t propagate_epoch_han = uninitialized_handler;
  handler_t epoch_finished_han = uninitialized_handler;
  handler_t ready_epoch_han = uninitialized_handler;
  handler_t epoch_continue_han = uninitialized_handler;

private:
  epoch_t cur_epoch = no_epoch;

  // the epoch window [fst, lst]
  epoch_t first_resolved_epoch = no_epoch, last_resolved_epoch = no_epoch;

  // global termination state
  term_state_t any_epoch_state;

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
