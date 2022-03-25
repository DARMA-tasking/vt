/*
//@HEADER
// *****************************************************************************
//
//                              workload_replay.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_WORKLOAD_REPLAY_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_WORKLOAD_REPLAY_H

#include "vt/config.h"
#include "vt/elm/elm_id.h"
#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/model/load_model.h"
#include "vt/vrt/collection/balance/model/proposed_reassignment.h"

#include <string>
#include <unordered_map>
#include <set>

namespace vt { namespace vrt { namespace collection {
namespace balance {

/**
 * \brief Simulate replaying the object workloads as recorded in the json file,
 * but allow new load balancing decisions to be made.
 *
 * \param[in] initial_phase the first phase to replay
 * \param[in] phases_to_run how many phases to replay
 *
 * The json files specified by the command-line arguments --vt_lb_stats_file_in
 * and --vt_lb_stats_dir_in will be imported and the LB data contained within
 * will be fed through the specified load balancer(s) on each requested phase,
 * allowing new load balancing migrations to happen. There is no requirement to
 * colocate the LB data on the same rank as the object exists during any given
 * phase.
 */
void replayWorkloads(
  PhaseType initial_phase, PhaseType phases_to_run
);

/**
 * \struct WorkloadDataMigrator
 *
 * \brief A helper objgroup for workload replay. Derives from
 * \c vt::Vrt::collection::lb::BaseLB in order to gain access to
 * normalizeReassignments but is not a load balancer in the traditional sense.
 * A new instance should be created for each call to normalizeReassignments.
 */
struct WorkloadDataMigrator : lb::BaseLB {

  using ObjIDType = elm::ElementIDStruct;

  WorkloadDataMigrator() = default;

  /**
   * \brief Construct an objgroup and configure it
   *
   * \param[in] model_base the load model that reflects the known workloads
   *
   * \return the objgroup proxy to use for exchanging workload information
   */
  static objgroup::proxy::Proxy<WorkloadDataMigrator>
  construct(std::shared_ptr<LoadModel> model_base);

  void runLB(TimeType) override;

  void inputParams(SpecEntry* spec) override;

  static std::unordered_map<std::string, std::string> getInputKeysWithHelp();

  using BaseLB::normalizeReassignments;

  /**
   * \brief Update the current locations of objects so that ProposedReassignment
   * load models can be composed
   *
   * \param[in] lb_reassignment the Reassignment returned by a load balancer
   *
   * \return a new Reassignment that reflects the updated locations of objects
   */
  static std::shared_ptr<Reassignment>
  updateCurrentNodes(
    std::shared_ptr<const Reassignment> lb_reassignment
  );

  /**
   * \brief Build a StatsData object from the LB data in a json file
   *
   * \param[in] filename read in LB data from the specified json file
   *
   * \return the StatsData object built from the LB data
   */
  static std::shared_ptr<StatsData>
  readInWorkloads(std::string filename);

  /**
   * \brief Relocate object workloads to the rank where the objects are supposed
   * to exist during this phase
   *
   * \param[in] model_base the load model for the phase we are simulating
   * \param[in] migratable_objects_here migratable objects here on this phase
   *
   * \return load model that makes the necessary object workloads available
   */
  static std::shared_ptr<ProposedReassignment>
  relocateWorkloadsForReplay(
    std::shared_ptr<LoadModel> model_base,
    std::set<ObjIDType> migratable_objects_here
  );

  /**
   * \brief Instantiate objgroup and relocate applicable object workloads home
   *
   * \param[in] model_base the load model for the phase we are simulating
   * \param[in] migratable_objects_here migratable objects here on this phase
   *
   * \return load model that makes the necessary object workloads available
   */
  static std::shared_ptr<ProposedReassignment>
  relocateMisplacedWorkloadsHome(
    std::shared_ptr<LoadModel> model_base,
    std::set<ObjIDType> migratable_objects_here
  );

  /**
   * \brief Instantiate objgroup and relocate applicable workloads here
   *
   * \param[in] model_base the load model for the phase we are simulating
   * \param[in] migratable_objects_here migratable objects here on this phase
   *
   * \return load model that makes the necessary object workloads available
   */
  static std::shared_ptr<ProposedReassignment>
  relocateMisplacedWorkloadsHere(
    std::shared_ptr<LoadModel> model_base,
    std::set<ObjIDType> migratable_objects_here
  );

private:
  /**
   * \brief Relocate object workloads home if the object is not on this rank
   *
   * \param[in] model_base the load model for the phase we are simulating
   * \param[in] migratable_objects_here migratable objects here on this phase
   *
   * \return load model that makes the necessary object workloads available
   */
  std::shared_ptr<ProposedReassignment>
  createModelToMoveWorkloadsHome(
    std::shared_ptr<LoadModel> model_base,
    std::set<ObjIDType> migratable_objects_here
  );

  /**
   * \brief Relocate workloads here for objects on this rank
   *
   * \param[in] model_base the load model for the phase we are simulating
   * \param[in] migratable_objects_here migratable objects here on this phase
   *
   * \return load model that makes the necessary object workloads available
   */
  std::shared_ptr<ProposedReassignment>
  createModelToMoveWorkloadsHere(
    std::shared_ptr<LoadModel> model_base,
    std::set<ObjIDType> migratable_objects_here
  );
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_WORKLOAD_REPLAY_H*/
