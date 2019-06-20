/*
//@HEADER
// *****************************************************************************
//
//                                termination.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_TERMINATION_TERMINATION_H
#define INCLUDED_TERMINATION_TERMINATION_H

#include "vt/config.h"
#include "vt/termination/term_common.h"
#include "vt/termination/term_msgs.h"
#include "vt/termination/term_state.h"
#include "vt/termination/term_action.h"
#include "vt/termination/term_interface.h"
#include "vt/termination/term_window.h"
#include "vt/termination/epoch_dependency.h"
#include "vt/termination/dijkstra-scholten/ds_headers.h"
#include "vt/termination/graph/epoch_graph.h"
#include "vt/epoch/epoch.h"
#include "vt/activefn/activefn.h"
#include "vt/collective/tree/tree.h"
#include "vt/configs/arguments/args.h"
#include "vt/termination/graph/epoch_graph_reduce.h"

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
  using ArgType            = vt::arguments::ArgConfig;
  using SuccessorBagType   = EpochDependency::SuccessorBagType;
  using EpochGraph         = termination::graph::EpochGraph;
  using EpochGraphMsg      = termination::graph::EpochGraphMsg<EpochGraph>;

  TerminationDetector();
  virtual ~TerminationDetector() {}

  /****************************************************************************
   *
   * Termination interface: produce(..)/consume(..) for 4-counter wave-based
   * termination, send(..) for Dijkstra-Scholten parental responsibility TD
   *
   ***************************************************************************/
  void produce(
    EpochType epoch = any_epoch_sentinel, TermCounterType num_units = 1,
    NodeType node = uninitialized_destination
  );
  void consume(
    EpochType epoch = any_epoch_sentinel, TermCounterType num_units = 1,
    NodeType node = uninitialized_destination
  );
  inline void hangDetectSend() { hang_.l_prod++; }
  inline void hangDetectRecv() { hang_.l_cons++; }
  /***************************************************************************/

  friend struct ds::StateDS;
  friend struct TermState;
  friend struct EpochDependency;

  /*
   * Test different types of epochs
   */
  bool isRooted(EpochType epoch) const;
  bool isDS(EpochType epoch) const;
  bool isDep(EpochType epoch) const;

  TermStateDSType* getDSTerm(EpochType epoch, bool is_root = false);

  void resetGlobalTerm();
  void freeEpoch(EpochType const& epoch);

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
   * Interface for making epochs for termination detection
   */
  EpochType makeEpochRooted(
    bool useDS = false, bool child = true, EpochType parent = no_epoch,
    bool is_dep = false
  );
  EpochType makeEpochCollective(
    bool child = true, EpochType parent = no_epoch, bool is_dep = false
  );
  EpochType makeEpoch(
    bool is_coll, bool useDS = false, bool child = true,
    EpochType parent = no_epoch, bool is_dep = false
  );

  /*
   * Interface to make "dependent" epochs which are not ready-by-default
   *
   *      Interaction modes between e_parent <- e_child epochs
   *
   *  (1) dep    <- dep     (+)
   *  (2) dep    <- no-dep  (+, only after release)
   *  (3) no-dep <- dep     (+)
   *  (4) no-dep <- no-dep  (+, current)
   *
   */
  EpochType makeEpochRootedDep(
    bool useDS = true, bool child = true, EpochType parent = no_epoch
  );
  EpochType makeEpochCollectiveDep(
    bool child = true, EpochType parent = no_epoch
  );

  /*
   * Release a dependent epoch, register action on release
   */
  void releaseEpoch(EpochType epoch);
  void onReleaseEpoch(EpochType epoch, ActionType action);

  /*
   * Test if an epoch is dependent and if it is, if that epoch has been released
   */
  bool epochReleased(EpochType epoch);

  /*
   * Tell the system an epoch will not have more events added locally. Must be
   * called once for a rooted epoch and on every node for a collective.
   */
  void finishedEpoch(EpochType const& epoch);

  /*
   * Advanced interface: finishedEpoch calls both of these functions, which can
   * be split apart in more complex use cases
   */
  void activateEpoch(EpochType const& epoch);
  void finishNoActivateEpoch(EpochType const& epoch);

private:
  ParentBagType const& getParents(EpochType epoch);
  void cleanupReleasedEpoch(EpochType epoch);
  void runReleaseEpochActions(EpochType epoch);
  bool epochParentReleased(EpochType epoch);

public:
  /*
   * Directly call into a specific type of rooted epoch, can not be overridden
   */
  EpochType makeEpochRootedWave(bool child, EpochType parent, bool dep = false);
  EpochType makeEpochRootedDS(bool child, EpochType parent, bool dep = false);

private:
  enum CallFromEnum { Root, NonRoot };

  TermStateType& findOrCreateState(EpochType const& epoch, bool is_ready);
  void cleanupEpoch(EpochType const& epoch, CallFromEnum from);
  void produceConsumeState(
    TermStateType& state, TermCounterType const num_units, bool produce,
    NodeType node
  );
  void produceConsume(
    EpochType epoch = any_epoch_sentinel, TermCounterType num_units = 1,
    bool produce = true, NodeType node = uninitialized_destination
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
  void countsConstant(TermStateType& state);

public:
  void startEpochGraphBuild();

private:
  void updateResolvedEpochs(EpochType const& epoch);
  void inquireTerminated(EpochType const& epoch, NodeType const& from_node);
  void replyTerminated(EpochType const& epoch, bool const& is_terminated);

public:
  void setLocalTerminated(bool const terminated, bool const no_propagate = true);
  void maybePropagate();
  TermCounterType getNumUnits() const;
  std::size_t getNumTerminatedCollectiveEpochs() const;

public:
  // TermTerminated interface
  TermStatusEnum testEpochTerminated(EpochType epoch) override;
  // Might return (conservatively) false if the epoch is non-local
  bool isEpochTerminated(EpochType epoch);

public:
  std::shared_ptr<EpochGraph> makeGraph();

private:
  static void hangCheckHandler(HangCheckMsg* msg);
  static void buildLocalGraphHandler(BuildGraphMsg* msg);
  static void epochGraphBuiltHandler(EpochGraphMsg* msg);

private:
  bool propagateEpoch(TermStateType& state);
  void epochTerminated(EpochType const& epoch, CallFromEnum from);
  void epochContinue(EpochType const& epoch, TermWaveType const& wave);
  void setupNewEpoch(EpochType const& epoch);
  void readyNewEpoch(EpochType const& epoch);
  void makeRootedHan(EpochType const& epoch, bool is_root);

public:
  void addDependency(EpochType predecessor, EpochType successoor);

public:
  // Methods for testing state of TD from unit tests
  EpochContainerType<TermStateType> const& getEpochState() { return epoch_state_; }
  std::unordered_set<EpochType> const& getEpochReadySet() { return epoch_ready_; }
  std::unordered_set<EpochType> const& getEpochWaitSet() { return epoch_wait_status_; }

private:
  EpochDependency* getEpochDep(EpochType epoch);
  void removeEpochStateDependency(EpochType ep);
  void addEpochStateDependency(EpochType ep);

private:
  static void makeRootedHandler(TermMsg* msg);
  static void inquireEpochTerminated(TermTerminatedMsg* msg);
  static void replyEpochTerminated(TermTerminatedReplyMsg* msg);
  static void propagateEpochHandler(TermCounterMsg* msg);
  static void epochTerminatedHandler(TermMsg* msg);
  static void epochContinueHandler(TermMsg* msg);

private:
  // global termination state
  TermStateType any_epoch_state_;
  // hang detector termination state
  TermStateType hang_;
  // epoch termination state
  EpochContainerType<TermStateType> epoch_state_                            = {};
  // epoch window container for specific archetyped epochs
  std::unordered_map<EpochType,WindowType> epoch_arch_                      = {};
  // epoch window for basic collective epochs
  std::unique_ptr<EpochWindow> epoch_coll_                                  = nullptr;
  // ready epoch list (misnomer: finishedEpoch was invoked)
  std::unordered_set<EpochType> epoch_ready_                                = {};
  // list of remote epochs pending status report of finished
  std::unordered_set<EpochType> epoch_wait_status_                          = {};
  // has printed epoch graph during abort
  bool has_printed_epoch_graph                                              = false;
  // released epoch list for dependent epochs
  std::unordered_set<EpochType> epoch_released_                             = {};
  // release epoch action list for dependent epochs
  std::unordered_map<EpochType,std::list<ActionType>> epoch_release_action_ = {};
};

}} // end namespace vt::term

#include "vt/termination/termination.impl.h"
#include "vt/termination/term_scope.impl.h"

#endif /*INCLUDED_TERMINATION_TERMINATION_H*/
