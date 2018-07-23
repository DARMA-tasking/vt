
#if !defined INCLUDED_TERMINATION_TERMINATION_H
#define INCLUDED_TERMINATION_TERMINATION_H

#include "config.h"
#include "termination/term_common.h"
#include "termination/term_msgs.h"
#include "termination/term_state.h"
#include "termination/term_action.h"
#include "epoch/epoch.h"
#include "activefn/activefn.h"
#include "collective/tree/tree.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vt { namespace term {

struct TerminationDetector : TermAction, collective::tree::Tree {
  template <typename T>
  using EpochContainerType = std::unordered_map<EpochType, T>;
  using TermStateType = TermState;

  TerminationDetector();

  void produce(
    EpochType epoch = any_epoch_sentinel, TermCounterType const& num_units = 1
  );

  void consume(
    EpochType epoch = any_epoch_sentinel, TermCounterType const& num_units = 1
  );

public:
  /*
   * Deprecated interface for creating a new collective epoch for detecting
   * termination
   */
  EpochType newEpoch();

public:
  /*
   * New interface for making epochs for termination detection
   */
  EpochType makeEpochRooted();
  EpochType makeEpochCollective();
  EpochType makeEpoch(bool const is_collective);
  void activateEpoch(EpochType const& epoch);
  void finishedEpoch(EpochType const& epoch);

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

public:
  // TermFinished interface
  bool testEpochFinished(EpochType const& epoch) override;

private:
  bool propagateEpoch(TermStateType& state);
  void epochFinished(EpochType const& epoch, bool const cleanup);
  void epochContinue(EpochType const& epoch, TermWaveType const& wave);
  void setupNewEpoch(EpochType const& new_epoch, bool const from_child);
  void propagateNewEpoch(EpochType const& new_epoch, bool const from_child);
  void readyNewEpoch(EpochType const& new_epoch);

  void rootMakeEpoch(EpochType const& epoch);
  void makeRootedEpoch(EpochType const& epoch, bool const is_root);
  static void makeRootedEpoch(TermMsg* msg);

  static void propagateNewEpochHandler(TermMsg* msg);
  static void readyEpochHandler(TermMsg* msg);
  static void propagateEpochHandler(TermCounterMsg* msg);
  static void epochFinishedHandler(TermMsg* msg);
  static void epochContinueHandler(TermMsg* msg);

private:
  // the epoch window [fst, lst]
  EpochType first_resolved_epoch_ = no_epoch, last_resolved_epoch_ = no_epoch;
  // global termination state
  TermStateType any_epoch_state_;
  // epoch termination state
  EpochContainerType<TermStateType> epoch_state_;
  // finished epoch: temporary fix (has high memory bound)
  std::unordered_set<EpochType> epoch_finished_;
};

}} // end namespace vt::term

#include "termination/termination.impl.h"

#endif /*INCLUDED_TERMINATION_TERMINATION_H*/
