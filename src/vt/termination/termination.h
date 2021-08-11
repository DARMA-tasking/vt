/*
//@HEADER
// *****************************************************************************
//
//                                termination.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_TERMINATION_TERMINATION_H
#define INCLUDED_VT_TERMINATION_TERMINATION_H

#include "vt/config.h"
#include "vt/termination/term_common.h"
#include "vt/termination/term_msgs.h"
#include "vt/termination/term_state.h"
#include "vt/termination/term_action.h"
#include "vt/termination/term_interface.h"
#include "vt/termination/epoch_dependency.h"
#include "vt/termination/dijkstra-scholten/ds_headers.h"
#include "vt/termination/graph/epoch_graph.h"
#include "vt/epoch/epoch.h"
#include "vt/activefn/activefn.h"
#include "vt/collective/tree/tree.h"
#include "vt/termination/graph/epoch_graph_reduce.h"
#include "vt/termination/epoch_tags.h"
#include "vt/runtime/component/component_pack.h"

#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <vector>
#include <memory>

namespace vt { namespace term {

using DijkstraScholtenTerm = term::ds::StateDS;

/**
 * \struct TerminationDetector
 *
 * \brief Detect global termination and of subsets of work
 *
 * Implements distributed algorithms to termination detection across the entire
 * VT runtime and for subset of work, encapsulated in an epoch. Ships with two
 * algorithms: 4-counter wave-based termination for large collective epochs;
 * and, Dijkstra-Scholten parental responsibility termination for rooted
 * epochs. Epochs may have other epochs nested within them, forming a graph.
 *
 * The termination detector detects termination of the transitive closure of a
 * piece of work---either starting collectively with all nodes or starting on a
 * particular node (rooted).
 *
 * In order to track work on the distributed system, work is "produced" and
 * "consumed". Produce and consume are separate counters that are tracked on
 * each node for each epoch. When the global produce and consume counts (sum
 * across all nodes) are equal, termination is reached.
 */
struct TerminationDetector :
  runtime::component::Component<TerminationDetector>,
  TermAction, collective::tree::Tree, DijkstraScholtenTerm, TermInterface
{
  template <typename T>
  using EpochContainerType = std::unordered_map<EpochType, T>;
  using TermStateType      = TermState;
  using TermStateDSType    = term::ds::StateDS::TerminatorType;
  using SuccessorBagType   = EpochDependency::SuccessorBagType;
  using EpochGraph         = termination::graph::EpochGraph;
  using EpochGraphMsg      = termination::graph::EpochGraphMsg<EpochGraph>;

  /**
   * \internal \brief Construct a termination detector
   */
  TerminationDetector();

  virtual ~TerminationDetector() {}

  std::string name() override { return "TerminationDetector"; }

  /****************************************************************************
   *
   * Termination interface: produce(..)/consume(..) for 4-counter wave-based
   * termination, send(..) for Dijkstra-Scholten parental responsibility TD
   *
   ***************************************************************************/

  /**
   * \brief Produce on an epoch---increase the produce counter
   *
   * \param[in] epoch the epoch to produce; if empty, produce on global epoch
   * \param[in] num_units number of units to produce
   * \param[in] node the node where this unit will be consumed (optional)
   */
  void produce(
    EpochType epoch = any_epoch_sentinel, TermCounterType num_units = 1,
    NodeType node = uninitialized_destination
  );

  /**
   * \brief Consume on an epoch---increase the consume counter
   *
   * \param[in] epoch the epoch to consume; if empty, consume on global epoch
   * \param[in] num_units number of units to consume
   * \param[in] node the node where this unit was produced (optional)
   */
  void consume(
    EpochType epoch = any_epoch_sentinel, TermCounterType num_units = 1,
    NodeType node = uninitialized_destination
  );

  /**
   * \internal \brief Special produce for hang detection
   */
  inline void hangDetectSend() { hang_.l_prod++; }

  /**
   * \internal \brief Special consume for hang detection
   */
  inline void hangDetectRecv() { hang_.l_cons++; }
  /***************************************************************************/

  friend struct ds::StateDS;
  friend struct TermState;
  friend struct EpochDependency;

  /**
   * \brief Check if an epoch is rooted
   *
   * \param[in] epoch the epoch to check
   *
   * \return whether it is rooted
   */
  bool isRooted(EpochType epoch);

  /**
   * \brief Check if the algorithm behind an epoch is Dijkstra-Scholten parental
   * responsibility
   *
   * \param[in] epoch the epoch to check
   *
   * \return whether is it DS
   */
  bool isDS(EpochType epoch);

  /**
   * \internal \brief Get or create the DS terminator for an epoch
   *
   * \param[in] epoch the epoch
   * \param[in] is_root whether this is the root (relevant when creating)
   *
   * \return the DS terminator manager
   */
  TermStateDSType* getDSTerm(EpochType epoch, bool is_root = false);

  /**
   * \brief Reset global termination to start producing/consuming again
   */
  void resetGlobalTerm();

  /**
   * \internal \brief Free an epoch after termination
   *
   * \param[in] epoch the epoch
   */
  void freeEpoch(EpochType const& epoch);

public:
  /*
   * Interface for creating new epochs for termination detection
   */

  /**
   * \brief Create a new rooted epoch
   *
   * \param[in] use_ds whether to use the Dijkstra-Scholten algorithm
   * \param[in] parent parent epoch that waits for this new epoch
   *
   * \return the new epoch
   */
  EpochType makeEpochRooted(
    UseDS use_ds = UseDS{false},
    ParentEpochCapture parent = ParentEpochCapture{}
  );

  /**
   * \brief Create a new collective epoch
   *
   * \param[in] parent parent epoch that waits for this new epoch
   *
   * \return the new epoch
   */
  EpochType makeEpochCollective(
    ParentEpochCapture parent = ParentEpochCapture{}
  );

  /**
   * \brief Create a new rooted epoch with a label
   *
   * \param[in] label epoch label for debugging purposes
   * \param[in] use_ds whether to use the Dijkstra-Scholten algorithm
   * \param[in] parent parent epoch that waits for this new epoch
   *
   * \return the new epoch
   */
  EpochType makeEpochRooted(
    std::string const& label,
    UseDS use_ds = UseDS{false},
    ParentEpochCapture parent = ParentEpochCapture{}
  );

  /**
   * \brief Create a collective epoch with a label
   *
   * \param[in] label epoch label for debugging purposes
   * \param[in] parent parent epoch that waits for this new epoch
   *
   * \return the new epoch
   */
  EpochType makeEpochCollective(
    std::string const& label,
    ParentEpochCapture parent = ParentEpochCapture{}
  );

  /**
   * \brief Create a new rooted or collective epoch with a label
   *
   * \param[in] label epoch label for debugging purposes
   * \param[in] is_coll whether to create a collective or rooted epoch
   * \param[in] use_ds whether to use the Dijkstra-Scholten algorithm
   * \param[in] parent parent epoch that waits for this new epoch
   *
   * \return the new epoch
   */
  EpochType makeEpoch(
    std::string const& label,
    bool is_coll,
    UseDS use_ds = UseDS{false},
    ParentEpochCapture parent = ParentEpochCapture{}
  );

  /**
   * \brief Setup a collective epoch with the epoch already generated
   *
   * \param[in] epoch the collective epoch already generated
   * \param[in] label epoch label for debugging purposes
   * \param[in] parent parent epoch that waits for this new epoch
   */
  void initializeCollectiveEpoch(
    EpochType const epoch,
    std::string const& label,
    ParentEpochCapture parent = ParentEpochCapture{}
  );

  /**
   * \brief Setup a new rooted epoch with the epoch already generated
   *
   * \param[in] epoch the collective epoch already generated
   * \param[in] label epoch label for debugging purposes
   * \param[in] use_ds whether to use the Dijkstra-Scholten algorithm
   * \param[in] parent parent epoch that waits for this new epoch
   */
  void initializeRootedEpoch(
    EpochType const epoch,
    std::string const& label,
    UseDS use_ds = UseDS{false},
    ParentEpochCapture parent = ParentEpochCapture{}
  );

  /**
   * \brief Tell the termination detector that all initial work has been
   * enqueued for a given epoch on this node
   *
   * \param[in] epoch the finished epoch
   */
  void finishedEpoch(EpochType const& epoch);

  /**
   * \internal \brief Activate an epoch; start detecting on it
   *
   * \param[in] epoch the epoch to activate
   */
  void activateEpoch(EpochType const& epoch);

  /**
   * \internal \brief Finish an epoch without activating it (starting the work
   * of detecting its termination)
   *
   * \param[in] epoch the epoch that is finished
   */
  void finishNoActivateEpoch(EpochType const& epoch);

public:
  /*
   * Directly call into a specific type of rooted epoch, can not be overridden
   */

  /**
   * \brief Create a new rooted epoch that uses the 4-counter wave algorithm
   *
   * \param[in] parent parent epoch that waits for this new epoch
   * \param[in] label epoch label for debugging purposes
   *
   * \return the new epoch
   */
  EpochType makeEpochRootedWave(
    ParentEpochCapture parent, std::string const& label = ""
  );

  /**
   * \brief Create a new rooted epoch that uses the DS algorithm
   *
   * \param[in] parent parent epoch that waits for this new epoch
   * \param[in] label epoch label for debugging purposes
   *
   * \return the new epoch
   */
  EpochType makeEpochRootedDS(
    ParentEpochCapture parent, std::string const& label = ""
  );

  /**
   * \brief Setup a new rooted epoch that uses the 4-counter wave algorithm with
   * an epoch already generated
   *
   * \param[in] epoch the wave epoch already generated
   * \param[in] parent parent epoch that waits for this new epoch
   * \param[in] label epoch label for debugging purposes
   */
  void initializeRootedWaveEpoch(
    EpochType const epoch, ParentEpochCapture parent,
    std::string const& label = ""
  );

  /**
   * \brief Setup a new rooted epoch that uses the DS algorithm with the epoch
   * already generated
   *
   * \param[in] epoch the DS epoch already generated
   * \param[in] parent parent epoch that waits for this new epoch
   * \param[in] label epoch label for debugging purposes
   */
  void initializeRootedDSEpoch(
    EpochType const epoch, ParentEpochCapture parent,
    std::string const& label = ""
  );

private:
  enum CallFromEnum { Root, NonRoot };

  /**
   * \internal \brief Find or create on demand state for a collective wave-based
   * epoch
   *
   * \param[in] epoch the epoch
   * \param[in] is_ready whether it is ready
   *
   * \return termination state for the epoch
   */
  TermStateType& findOrCreateState(EpochType const& epoch, bool is_ready);

  /**
   * \internal \brief Cleanup an epoch after termination
   *
   * \param[in] epoch the epoch
   * \param[in] from the caller
   */
  void cleanupEpoch(EpochType const& epoch, CallFromEnum from);

  /**
   * \internal \brief Produce/consume on an epoch
   *
   * \param[in] state the epoch state
   * \param[in] num_units number of units
   * \param[in] produce whether its a produce or consume
   * \param[in] node the node producing to or consuming from
   */
  void produceConsumeState(
    TermStateType& state, TermCounterType const num_units, bool produce,
    NodeType node
  );

  /**
   * \internal \brief Produce\consume on an epoch
   *
   * \param[in] state the epoch state
   * \param[in] num_units number of units
   * \param[in] produce whether its a produce or consume
   * \param[in] node the node producing to or consuming from
   */
  void produceConsume(
    EpochType epoch = any_epoch_sentinel, TermCounterType num_units = 1,
    bool produce = true, NodeType node = uninitialized_destination
  );

  /**
   * \internal \brief Propagate an epoch with state
   *
   * \param[in] state epoch state
   * \param[in] prod num produced
   * \param[in] cons num consumed
   */
  void propagateEpochExternalState(
    TermStateType& state, TermCounterType const& prod, TermCounterType const& cons
  );

  /**
   * \internal \brief Propagate an epoch
   *
   * \param[in] epoch the epoch
   * \param[in] prod num produced
   * \param[in] cons num consumed
   */
  void propagateEpochExternal(
    EpochType const& epoch, TermCounterType const& prod,
    TermCounterType const& cons
  );

  /**
   * \internal \brief Check for and perform actions when a epoch's counts are
   * constant.
   *
   * \param[in] state the epoch state
   */
  void countsConstant(TermStateType& state);

public:
  /**
   * \internal \brief Build the epoch graph. Typically called to output to the
   * user due to a failure.
   */
  void startEpochGraphBuild();

private:
  /**
   * \internal \brief Update resolved epochs
   *
   * \param[in] epoch the epoch
   */
  void updateResolvedEpochs(EpochType const& epoch);

  /**
   * \internal \brief Inquire if an epoch has terminated
   *
   * \param[in] epoch the epoch
   * \param[in] from_node the node inquiring
   */
  void inquireTerminated(EpochType const& epoch, NodeType const& from_node);

  /**
   * \internal \brief Reply to a node whether an epoch has terminated
   *
   * \param[in] epoch the epoch
   * \param[in] is_terminated whether it has terminated
   */
  void replyTerminated(EpochType const& epoch, bool const& is_terminated);

public:
  /**
   * \internal \brief Set whether the scheduler has locally terminated
   *
   * \param[in] terminated whether it has terminated
   * \param[in] no_propagate whether to should propagate state remotely
   */
  void setLocalTerminated(bool const terminated, bool const no_propagate = true);

  /**
   * \internal \brief Progress function to move state forward
   */
  void maybePropagate();

  /**
   * \brief Get number of units produced on global epoch
   *
   * \return number of produced units
   */
  TermCounterType getNumUnits() const;

  /**
   * \brief Get number of collective epochs that have terminated
   *
   * \return number of epochs
   */
  std::size_t getNumTerminatedCollectiveEpochs() const;

public:
  /**
   * \brief Test if an epoch has terminated or not
   *
   * \param[in] epoch the epoch to test
   *
   * \return status enum indicating the known state
   */
  TermStatusEnum testEpochTerminated(EpochType epoch) override;

  /**
   * \brief Check if an epoch has terminated
   *
   * \note Might return (conservatively) false for some time if the epoch is
   * non-local, but will eventually return true
   *
   * \param[in] epoch the epoch to test
   *
   * \return whether it is known to be terminated
   */
  bool isEpochTerminated(EpochType epoch);

public:
  /**
   * \brief Make the local epoch graph
   *
   * \return shared pointer to epoch graph
   */
  std::shared_ptr<EpochGraph> makeGraph();

private:
  /**
   * \internal \brief Handler for hang checking
   *
   * \param[in] msg the message
   */
  static void hangCheckHandler(HangCheckMsg* msg);

  /**
   * \internal \brief Handler for building the local epoch graph
   *
   * \param[in] msg the message
   */
  static void buildLocalGraphHandler(BuildGraphMsg* msg);

  /**
   * \internal \brief Handler for to call when epoch graph is done building
   *
   * \param[in] msg the message
   */
  static void epochGraphBuiltHandler(EpochGraphMsg* msg);

private:
  /**
   * \internal \brief Propagate a particular epoch
   *
   * \param[in] state the state for the epoch
   *
   * \return whether it made progress
   */
  bool propagateEpoch(TermStateType& state);

  /**
   * \internal \brief Notfiy that an epoch has terminated
   *
   * \param[in] epoch the epoch
   * \param[in] from the caller
   */
  void epochTerminated(EpochType const& epoch, CallFromEnum from);

  /**
   * \internal \brief Do another wave for an epoch
   *
   * \param[in] epoch the epoch
   * \param[in] wave the wave count so far
   */
  void epochContinue(EpochType const& epoch, TermWaveType const& wave);

  /**
   * \internal \brief Setup state for a new epoch
   *
   * \param[in] epoch the epoch
   * \param[in] label the label for debugging
   */
  void setupNewEpoch(EpochType const& epoch, std::string const& label);

  /**
   * \internal \brief Ready an epoch
   *
   * \param[in] epoch the epoch
   */
  void readyNewEpoch(EpochType const& epoch);

  /**
   * \internal \brief Make an epoch
   *
   * \param[in] epoch the epoch
   * \param[in] is_root whether it is rooted
   * \param[in] label the label for debugging
   */
  void makeRootedHan(
    EpochType const& epoch, bool is_root, std::string const& label = ""
  );

public:
  /**
   * \brief Add a local work dependency on an epoch to stop propagation
   *
   * \param[in] epoch the epoch
   */
  void addLocalDependency(EpochType epoch);

  /**
   * \brief Release a local work dependency on an epoch to resume propagation
   *
   * \param[in] epoch the epoch
   */
  void releaseLocalDependency(EpochType epoch);

  /**
   * \internal \brief Make a dependency between two epochs
   *
   * \param[in] predecessor the predecessor epoch
   * \param[in] successoor the successoor epoch
   */
  void addDependency(EpochType predecessor, EpochType successoor);

public:
  // Methods for testing state of TD from unit tests
  EpochContainerType<TermStateType> const& getEpochState() { return epoch_state_; }
  std::unordered_set<EpochType> const& getEpochReadySet() { return epoch_ready_; }
  std::unordered_set<EpochType> const& getEpochWaitSet() { return epoch_wait_status_; }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | any_epoch_state_
      | hang_
      | epoch_state_
      | epoch_ready_
      | epoch_wait_status_
      | has_printed_epoch_graph;
  }

private:
  /**
   * \internal \brief Produce on the global epoch to inhibit global terminations
   * before receipt of a nested epoch's completion
   *
   * \note Although the creation of a new epoch also produces on the global
   * epoch, it consumes on both the generated epoch and the global one when the
   * epoch is activated (\c finishedEpoch ). This means that the termination of
   * the global epoch along with associated actions can race with notification
   * of termination on the newly created epoch. If the runtime is destroyed as a
   * result of the global epoch terminating the final notification of the newly
   * created epoch can arrive later causing strange behavior. This has been
   * observed in the code with a full reset of epoch allocation when the runtime
   * globally terminates.
   *
   * \param[in] ep nested epoch (for debugging)
   */
  void produceOnGlobal(EpochType ep);

  /**
   * \internal \brief Consume on the global epoch to inhibit global terminations
   * before receipt of a nested epoch's completion
   *
   * \param[in] ep the nested epoch (for debugging)
   */
  void consumeOnGlobal(EpochType ep);

  /**
   * \internal \brief Get an epoch's dependency information
   *
   * \param[in] epoch the epoch
   *
   * \return the dependency
   */
  EpochDependency* getEpochDep(EpochType epoch);

  /**
   * \internal \brief Decrement the join counter on an epoch's dependency
   *
   * \param[in] ep the epoch
   */
  void removeEpochStateDependency(EpochType ep);

  /**
   * \internal \brief Increment the join counter on an epoch's dependency
   *
   * \param[in] ep the epoch
   */
  void addEpochStateDependency(EpochType ep);

private:
  /**
   * \internal \brief Make a rooted epoch handler
   *
   * \param[in] msg the message
   */
  static void makeRootedHandler(TermMsg* msg);

  /**
   * \internal \brief Inquire if an epoch terminated handler
   *
   * \param[in] msg the message
   */
  static void inquireEpochTerminated(TermTerminatedMsg* msg);

  /**
   * \internal \brief Reply if an epoch terminated handler
   *
   * \param[in] msg the message
   */
  static void replyEpochTerminated(TermTerminatedReplyMsg* msg);

  /**
   * \internal \brief Propagate an epoch handler
   *
   * \param[in] msg the message
   */
  static void propagateEpochHandler(TermCounterMsg* msg);

  /**
   * \internal \brief Notify an epoch terminated handler
   *
   * \param[in] msg the message
   */
  static void epochTerminatedHandler(TermMsg* msg);

  /**
   * \internal \brief Continue doing waves for an epoch handler
   *
   * \param[in] msg the message
   */
  static void epochContinueHandler(TermMsg* msg);

private:
  // global termination state
  TermStateType any_epoch_state_;
  // hang detector termination state
  TermStateType hang_;
  // epoch termination state
  EpochContainerType<TermStateType> epoch_state_        = {};
  // ready epoch list (misnomer: finishedEpoch was invoked)
  std::unordered_set<EpochType> epoch_ready_            = {};
  // list of remote epochs pending status report of finished
  std::unordered_set<EpochType> epoch_wait_status_      = {};
  // has printed epoch graph during abort
  bool has_printed_epoch_graph                          = false;
};

}} // end namespace vt::term

#include "vt/termination/termination.impl.h"

#endif /*INCLUDED_VT_TERMINATION_TERMINATION_H*/
