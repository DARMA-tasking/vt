
#if ! defined __RUNTIME_TRANSPORT_TERMINATION__
#define __RUNTIME_TRANSPORT_TERMINATION__

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "config.h"
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
    produceConsume(epoch, true);
  }

  inline void consume(EpochType const& epoch = no_epoch) {
    produceConsume(epoch, false);
  }

  EpochType newEpoch();

  void produceConsume(EpochType const& epoch = no_epoch, bool produce = true);
  void maybePropagate();

  void propagateEpochExternal(
    EpochType const& epoch, TermCounterType const& prod,
    TermCounterType const& cons
  );

  bool propagateEpoch(EpochType const& epoch, TermStateType& state);

  void epochFinished(EpochType const& epoch);
  void epochContinue(EpochType const& epoch);
  void triggerAllEpochActions(EpochType const& epoch);
  void triggerAllActions(EpochType const& epoch);
  void attachGlobalTermAction(ActionType action);
  void forceGlobalTermAction(ActionType action);
  void attachEpochTermAction(EpochType const& epoch, ActionType action);
  void setupNewEpoch(EpochType const& new_epoch);
  void propagateNewEpoch(EpochType const& new_epoch);
  void readyNewEpoch(EpochType const& new_epoch);

  static void registerDefaultTerminationAction();
  static void propagateNewEpochHandler(TermMsg* msg);
  static void readyEpochHandler(TermMsg* msg);
  static void propagateEpochHandler(TermCounterMsg* msg);
  static void epochFinishedHandler(TermMsg* msg);
  static void epochContinueHandler(TermMsg* msg);

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
