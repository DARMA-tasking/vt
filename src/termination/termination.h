
#if !defined INCLUDED_TERMINATION_TERMINATION_H
#define INCLUDED_TERMINATION_TERMINATION_H

#include "config.h"
#include "epoch/epoch.h"
#include "activefn/activefn.h"
#include "term_common.h"
#include "term_msgs.h"
#include "term_state.h"
#include "collective/tree/tree.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace vt { namespace term {

using namespace vt::epoch;

struct TerminationDetector : collective::tree::Tree {
  using TermStateType = TermState;
  using ActionContainerType = std::vector<ActionType>;

  TerminationDetector();

  template <typename T>
  using EpochContainerType = std::unordered_map<EpochType, T>;

  inline void produce(
    EpochType epoch = any_epoch_sentinel, TermCounterType const& num_units = 1
  ) {
    debug_print(term, node, "Termination: produce: epoch={}\n",epoch);
    auto const in_epoch = epoch == no_epoch ? any_epoch_sentinel : epoch;
    return produceConsume(in_epoch, num_units, true);
  }

  inline void consume(
    EpochType epoch = any_epoch_sentinel, TermCounterType const& num_units = 1
  ) {
    debug_print(term, node, "Termination: consume: epoch={}\n",epoch);
    auto const in_epoch = epoch == no_epoch ? any_epoch_sentinel : epoch;
    return produceConsume(in_epoch, num_units, false);
  }

public:
  EpochType newEpoch();

  void attachGlobalTermAction(ActionType action);
  void forceGlobalTermAction(ActionType action);
  void attachEpochTermAction(EpochType const& epoch, ActionType action);

  static void registerDefaultTerminationAction(ActionType default_action);

private:
  EpochType newEpochCollective();
  EpochType newEpochRooted();

  TermStateType& findOrCreateState(EpochType const& epoch, bool is_ready);
  void cleanupEpoch(EpochType const& epoch);
  void produceConsumeState(
    TermStateType& state, TermCounterType const& num_units, bool produce
  );
  void produceConsume(
    EpochType const& epoch = any_epoch_sentinel,
    TermCounterType const& num_units = 1, bool produce = true
  );
  void propagateEpochExternalState(
    TermStateType& state, TermCounterType const& prod, TermCounterType const& cons
  );
  void propagateEpochExternal(
    EpochType const& epoch, TermCounterType const& prod,
    TermCounterType const& cons
  );

public:
  void setLocalTerminated(bool const terminated, bool const no_local = true);
  void maybePropagate();
  TermCounterType getNumUnits() const;

private:
  bool propagateEpoch(TermStateType& state);
  void epochFinished(EpochType const& epoch, bool const cleanup);
  void epochContinue(EpochType const& epoch, TermWaveType const& wave);
  void triggerAllEpochActions(EpochType const& epoch);
  void triggerAllActions(EpochType const& epoch);
  void setupNewEpoch(EpochType const& new_epoch, bool const from_child);
  void propagateNewEpoch(EpochType const& new_epoch, bool const from_child);
  void readyNewEpoch(EpochType const& new_epoch);

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

extern term::TerminationDetector* theTerm();

} // end namespace vt

#endif /*INCLUDED_TERMINATION_TERMINATION_H*/
