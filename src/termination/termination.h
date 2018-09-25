
#if !defined INCLUDED_TERMINATION_TERMINATION_H
#define INCLUDED_TERMINATION_TERMINATION_H

#include "config.h"
#include "termination/term_common.h"
#include "termination/term_msgs.h"
#include "termination/term_state.h"
#include "termination/term_action.h"
#include "termination/term_interface.h"
#include "termination/dijkstra-scholten/ds_headers.h"
#include "epoch/epoch.h"
#include "activefn/activefn.h"
#include "collective/tree/tree.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace vt { namespace term {

using DijkstraScholtenTerm = term::ds::StateDS;

struct TerminationDetector :
  TermAction, collective::tree::Tree, DijkstraScholtenTerm, TermInterface
{
  template <typename T>
  using EpochContainerType = std::unordered_map<EpochType, T>;
  using TermStateType = TermState;
  using TermStateDSType = term::ds::StateDS::TerminatorType;

  TerminationDetector();

  /****************************************************************************
   *
   * Termination interface: produce(..)/consume(..) for 4-counter wave-based
   * termination, send(..) for Dijkstra-Scholten parental responsibility TD
   *
   ***************************************************************************/
  void produce(
    EpochType epoch = any_epoch_sentinel, TermCounterType const& num_units = 1
  );
  void consume(
    EpochType epoch = any_epoch_sentinel, TermCounterType const& num_units = 1
  );
  void send(NodeType const& node, EpochType const& epoch = any_epoch_sentinel);
  void recv(NodeType const& node, EpochType const& epoch = any_epoch_sentinel);
  /***************************************************************************/

  friend struct ds::StateDS;
  TermStateDSType* getDSTerm(EpochType const& epoch);

  void resetGlobalTerm();
  void freeEpoch(EpochType const& epoch);

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
  EpochType makeEpochRooted(bool const useDS = false);
  EpochType makeEpochCollective();
  EpochType makeEpoch(bool const is_collective, bool const useDS = false);
  void activateEpoch(EpochType const& epoch);
  void finishedEpoch(EpochType const& epoch);

  bool isEpochReady(EpochType const& epoch) const noexcept;
  bool isEpochFinished(EpochType const& epoch) const noexcept;

public:
  // void scopedEpoch(ActionType closure, ActionType action);

public:
  EpochType newEpochCollective();
  EpochType newEpochRooted(bool const useDS = false);

private:
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
  // finished epoch list (@todo: memory bound)
  std::unordered_set<EpochType> epoch_finished_;
  // ready epoch list (misnomer: finishedEpoch was invoked)
  std::unordered_set<EpochType> epoch_ready_;
};

}} // end namespace vt::term

#include "termination/termination.impl.h"

#endif /*INCLUDED_TERMINATION_TERMINATION_H*/
