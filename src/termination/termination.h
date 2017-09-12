
#if ! defined __RUNTIME_TRANSPORT_TERMINATION__
#define __RUNTIME_TRANSPORT_TERMINATION__

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "common.h"
#include "epoch.h"
#include "function.h"
#include "term_msgs.h"
#include "term_state.h"
#include "tree.h"

namespace vt { namespace term {

using namespace vt::epoch;

struct TerminationDetector : Tree {
  using TermStateType = TermState;
  using ActionContainerType = std::vector<ActionType>;

  TerminationDetector() { any_epoch_state_.recv_event_count = 1; }

  template <typename T>
  using EpochContainerType = std::unordered_map<EpochType, T>;

  inline void produce(EpochType const& epoch = no_epoch) {
    produce_consume(epoch, true);
  }

  inline void consume(EpochType const& epoch = no_epoch) {
    produce_consume(epoch, false);
  }

  EpochType new_epoch();

  void produce_consume(EpochType const& epoch = no_epoch, bool produce = true);
  void maybe_propagate();

  void propagate_epoch_external(
    EpochType const& epoch, TermCounterType const& prod,
    TermCounterType const& cons
  );

  bool propagate_epoch(EpochType const& epoch, TermStateType& state);

  void epoch_finished(EpochType const& epoch);
  void epoch_continue(EpochType const& epoch);
  void trigger_all_epoch_actions(EpochType const& epoch);
  void trigger_all_actions(EpochType const& epoch);
  void attach_global_term_action(ActionType action);
  void attach_epoch_term_action(EpochType const& epoch, ActionType action);
  void setup_new_epoch(EpochType const& new_epoch);
  void propagate_new_epoch(EpochType const& new_epoch);
  void ready_new_epoch(EpochType const& new_epoch);

  static void register_default_termination_action();
  static void propagate_new_epoch_handler(TermMsg* msg);
  static void ready_epoch_handler(TermMsg* msg);
  static void propagate_epoch_handler(TermCounterMsg* msg);
  static void epoch_finished_handler(TermMsg* msg);
  static void epoch_continue_handler(TermMsg* msg);

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

}} // end namespace vt::term

namespace vt {

extern std::unique_ptr<term::TerminationDetector> theTerm;

} // end namespace vt

#endif /*__RUNTIME_TRANSPORT_EVENT_TERMINATION__*/
