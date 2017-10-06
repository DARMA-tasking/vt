
#if ! defined __RUNTIME_TRANSPORT_TERMINATION__
#define __RUNTIME_TRANSPORT_TERMINATION__

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "config.h"
#include "epoch.h"
#include "registry_function.h"
#include "term_common.h"
#include "term_msgs.h"
#include "term_state.h"
#include "tree.h"

namespace vt { namespace term {

using namespace vt::epoch;

struct TerminationDetector : Tree {
  using TermStateType = TermState;
  using ActionContainerType = std::vector<ActionType>;

  TerminationDetector()
    : Tree(tree_cons_tag_t),
      any_epoch_state_(any_epoch_sentinel, getNumChildren())
  { }

  template <typename T>
  using EpochContainerType = std::unordered_map<EpochType, T>;

  inline void produce(
    EpochType epoch = any_epoch_sentinel, TermCounterType const& num_units = 1
  ) {
    debug_print(term, node, "y1: produce: epoch=%d\n",epoch);
    auto const in_epoch = epoch == no_epoch ? any_epoch_sentinel : epoch;
    return produceConsume(in_epoch, num_units, true);
  }

  inline void consume(
    EpochType epoch = any_epoch_sentinel, TermCounterType const& num_units = 1
  ) {
    debug_print(term, node, "y1: consume: epoch=%d\n",epoch);
    auto const in_epoch = epoch == no_epoch ? any_epoch_sentinel : epoch;
    return produceConsume(in_epoch, num_units, false);
  }

  TermStateType& findOrCreateState(EpochType const& epoch, bool is_ready);
  EpochType newEpoch();
  void cleanupEpoch(EpochType const& epoch);
  void produceConsumeState(
    TermStateType& state, TermCounterType const& num_units, bool produce
  );
  void produceConsume(
    EpochType const& epoch = any_epoch_sentinel,
    TermCounterType const& num_units = 1, bool produce = true
  );
  void maybePropagate();

  void propagateEpochExternalState(
    TermStateType& state, TermCounterType const& prod, TermCounterType const& cons
  );
  void propagateEpochExternal(
    EpochType const& epoch, TermCounterType const& prod,
    TermCounterType const& cons
  );

  bool propagateEpoch(TermStateType& state);

  void epochFinished(EpochType const& epoch, bool const cleanup);
  void epochContinue(EpochType const& epoch, TermWaveType const& wave);
  void triggerAllEpochActions(EpochType const& epoch);
  void triggerAllActions(EpochType const& epoch);
  void attachGlobalTermAction(ActionType action);
  void forceGlobalTermAction(ActionType action);
  void attachEpochTermAction(EpochType const& epoch, ActionType action);
  void setupNewEpoch(EpochType const& new_epoch, bool const from_child);
  void propagateNewEpoch(EpochType const& new_epoch, bool const from_child);
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
