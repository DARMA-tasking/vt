
#if !defined INCLUDED_TERMINATION_TERMINATION_H
#define INCLUDED_TERMINATION_TERMINATION_H

#include "vt/config.h"
#include "vt/termination/term_common.h"
#include "vt/termination/term_msgs.h"
#include "vt/termination/term_state.h"
#include "vt/termination/term_action.h"
#include "vt/termination/term_interface.h"
#include "vt/termination/term_window.h"
#include "vt/termination/dijkstra-scholten/ds_headers.h"
#include "vt/epoch/epoch.h"
#include "vt/activefn/activefn.h"
#include "vt/collective/tree/tree.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include <memory>

namespace vt { namespace term {

using DijkstraScholtenTerm = term::ds::StateDS;

struct TerminationDetector :
  TermAction, collective::tree::Tree, DijkstraScholtenTerm, TermInterface
{
  template <typename T>
  using EpochContainerType = std::unordered_map<EpochType, T>;
  using TermStateType      = TermState;
  using TermStateDSType    = term::ds::StateDS::TerminatorType;
  using WindowType         = std::unique_ptr<EpochWindow>;

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
  void genProd(EpochType const& epoch);
  void genCons(EpochType const& epoch);
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
  EpochType newEpoch(bool const child = false);

public:
  /*
   * Interface for scoped epochs using C++ lexical scopes to encapsulate epoch
   * regimes
   */

  struct Scoped {
    static EpochType rooted(bool small, ActionType closure);
    static EpochType rooted(bool small, ActionType closure, ActionType action);
    static EpochType collective(ActionType closure);
    static EpochType collective(ActionType closure, ActionType action);

    template <typename... Actions>
    static void rootedSeq(bool small, Actions... closures);
  } scope;

public:
  /*
   * New interface for making epochs for termination detection
   */
  EpochType makeEpochRooted(bool const useDS = false);
  EpochType makeEpochCollective();
  EpochType makeEpoch(bool const is_collective, bool const useDS = false);
  void activateEpoch(EpochType const& epoch);
  void finishedEpoch(EpochType const& epoch);

public:
  EpochType newEpochCollective(bool const child = false);
  EpochType newEpochRooted(bool const useDS = false, bool const child = false);

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

  EpochType getArchetype(EpochType const& epoch) const;
  EpochWindow* getWindow(EpochType const& epoch);

private:
  void updateResolvedEpochs(EpochType const& epoch, bool const rooted);
  void inquireFinished(EpochType const& epoch, NodeType const& from_node);
  void replyFinished(EpochType const& epoch, bool const& is_finished);

public:
  void setLocalTerminated(bool const terminated, bool const no_local = true);
  void maybePropagate();
  TermCounterType getNumUnits() const;

public:
  // TermFinished interface
  TermStatusEnum testEpochFinished(
    EpochType const& epoch, ActionType action
  ) override;

private:
  bool propagateEpoch(TermStateType& state);
  void epochFinished(EpochType const& epoch, bool const cleanup);
  void epochContinue(EpochType const& epoch, TermWaveType const& wave);
  void setupNewEpoch(EpochType const& new_epoch, bool const from_child);
  void propagateNewEpoch(EpochType const& new_epoch, bool const from_child);
  void readyNewEpoch(EpochType const& new_epoch);
  void linkChildEpoch(EpochType const& epoch);
  void rootMakeEpoch(EpochType const& epoch, bool const child = false);
  void makeRootedEpoch(EpochType const& epoch, bool const is_root);

  static void makeRootedEpoch(TermMsg* msg);
  static void inquireEpochFinished(TermFinishedMsg* msg);
  static void replyEpochFinished(TermFinishedReplyMsg* msg);
  static void propagateNewEpochHandler(TermMsg* msg);
  static void readyEpochHandler(TermMsg* msg);
  static void propagateEpochHandler(TermCounterMsg* msg);
  static void epochFinishedHandler(TermMsg* msg);
  static void epochContinueHandler(TermMsg* msg);

private:
  // global termination state
  TermStateType any_epoch_state_;
  // epoch termination state
  EpochContainerType<TermStateType> epoch_state_        = {};
  // epoch window container for specific archetyped epochs
  std::unordered_map<EpochType,WindowType> epoch_arch_  = {};
  // epoch window for basic collective epochs
  std::unique_ptr<EpochWindow> epoch_coll_              = nullptr;
  // ready epoch list (misnomer: finishedEpoch was invoked)
  std::unordered_set<EpochType> epoch_ready_            = {};
};

}} // end namespace vt::term

#include "vt/termination/termination.impl.h"
#include "vt/termination/term_scope.impl.h"

#endif /*INCLUDED_TERMINATION_TERMINATION_H*/
