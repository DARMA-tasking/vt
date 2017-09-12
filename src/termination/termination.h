
#if ! defined __RUNTIME_TRANSPORT_TERMINATION__
#define __RUNTIME_TRANSPORT_TERMINATION__

#include <cstdint>
#include <vector>
#include <unordered_map>

#include "common.h"
#include "function.h"
#include "tree.h"
#include "term_state.h"
#include "term_msgs.h"
#include "epoch.h"

namespace runtime { namespace term {

using namespace runtime::epoch;

struct TerminationDetector : Tree {
  using TermStateType = TermState;
  using ActionContainerType = std::vector<ActionType>;

  TerminationDetector() {
    any_epoch_state_.recv_event_count = 1;
  }

  template <typename T>
  using EpochContainerType = std::unordered_map<EpochType, T>;

  inline void
  produce(EpochType const& epoch = no_epoch) {
    produce_consume(epoch, true);
  }

  inline void
  consume(EpochType const& epoch = no_epoch) {
    produce_consume(epoch, false);
  }

  void
  produce_consume(EpochType const& epoch = no_epoch, bool produce = true);

  void
  maybe_propagate();

  void
  propagate_epoch_external(
    EpochType const& epoch, term_counter_t const& prod, term_counter_t const& cons
  );

  bool
  propagate_epoch(EpochType const& epoch, TermStateType& state);

  void
  epoch_finished(EpochType const& epoch);

  void
  epoch_continue(EpochType const& epoch);

  void
  trigger_all_epoch_actions(EpochType const& epoch);

  void
  trigger_all_actions(EpochType const& epoch);

  EpochType
  new_epoch();

  void
  attach_global_term_action(ActionType action);

  void
  attach_epoch_term_action(EpochType const& epoch, ActionType action);

  static void
  register_default_termination_action();

  void
  setup_new_epoch(EpochType const& new_epoch);

  void
  propagate_new_epoch(EpochType const& new_epoch);

  void
  ready_new_epoch(EpochType const& new_epoch);

  static void
  propagate_new_epoch_handler(TermMsg* msg);

  static void
  ready_epoch_handler(TermMsg* msg);

  static void
  propagate_epoch_handler(TermCounterMsg* msg);

  static void
  epoch_finished_handler(TermMsg* msg);

  static void
  epoch_continue_handler(TermMsg* msg);

private:
  EpochType cur_epoch_ = no_epoch;

  // the epoch window [fst, lst]
  EpochType first_resolved_epoch_ = no_epoch, last_resolved_epoch_ = no_epoch;

  // global termination state
  TermStateType any_epoch_state_;

  // epoch termination state
  EpochContainerType<TermStateType> epoch_state_;

  // action container for global termination
  ActionContainerType global_term_actions_;

  // action container for each epoch
  EpochContainerType<ActionContainerType> epoch_actions_;
};

}} //end namespace runtime::term


namespace runtime {

extern std::unique_ptr<term::TerminationDetector> the_term;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_EVENT_TERMINATION__*/
