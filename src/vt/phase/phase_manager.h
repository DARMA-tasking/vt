/*
//@HEADER
// *****************************************************************************
//
//                               phase_manager.h
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

#if !defined INCLUDED_VT_PHASE_PHASE_MANAGER_H
#define INCLUDED_VT_PHASE_PHASE_MANAGER_H

#include "vt/configs/types/types_type.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/phase/phase_hook_enum.h"
#include "vt/phase/phase_hook_id.h"

#include <unordered_map>
#include <map>

#define VT_PHASE_MANAGER 20201023

namespace vt { namespace phase {

// fwd-decl for reduce messasge
struct NextMsg;

/**
 * \struct PhaseManager
 *
 * \brief General management of phases in an application to delineate collective
 * intervals of time across nodes.
 *
 * Many system components use phases as a natural boundary for performing
 * incremental operations as the runtime makes progress. For instance, traces
 * may be flushed at phase boundaries and the load balancing framework might
 * apply a strategy between phases.
 *
 * The main interface for users is invoking
 * \c vt::thePhase()->nextPhaseCollective() to start the next phase. The system
 * performs a reduction and blocks completion inside this call. Any work
 * that belongs in the preceding phase should be synchronized before this is
 * called. The runtime system or users can register hooks when a phase starts,
 * ends, or after any migrations are complete. Hooks may be collective or
 * rooted; collective hooks are invoked in the order in which they are
 * registered and are always run in a collective epoch.
 */
struct PhaseManager : runtime::component::Component<PhaseManager> {
  using HookIDType = typename std::underlying_type<PhaseHook>::type;
  using HookMapType = std::map<std::size_t, ActionType>;
  using HookIDMapType = std::unordered_map<HookIDType, HookMapType>;

  PhaseManager() = default;

  std::string name() override { return "PhaseManager"; }

  void startup() override;

  /**
   * \internal
   * \brief Construct a new \c PhaseManager as an objgroup
   *
   * \return unique pointer to the new manager
   */
  static std::unique_ptr<PhaseManager> construct();

  /**
   * \brief Get the current phase
   *
   * \return the current phase
   */
  PhaseType getCurrentPhase() const { return cur_phase_; }

  /**
   * \brief Collectively register a phase hook that triggers depending on the
   * type of hook
   *
   * \note These must be registered across all nodes as they will be run in a
   * collective epoch. This is for synchronized phase actions. The order in
   * which hooks are collectively registered dictate the order they are
   * executed.
   *
   * \param[in] type the type of trigger to register
   * \param[in] trigger the action to trigger
   *
   * \return registered ID that can be used to unregister the hook
   */
  PhaseHookID registerHookCollective(PhaseHook type, ActionType trigger);

  /**
   * \brief Rooted register a phase hook that triggers depending on the type of
   * hook
   *
   * \note This is an independent hook that runs on this node only
   *
   * \param[in] type the type of trigger to register
   * \param[in] trigger the action to trigger
   *
   * \return registered ID that can be used to unregister the hook
   */
  PhaseHookID registerHookRooted(PhaseHook type, ActionType trigger);

  /**
   * \brief Unregister an existing hook
   *
   * \warning For collective hooks, they must all be unregistered across all
   * nodes before the next \c nextPhaseCollective is invoked.
   *
   * \param[in] hook the id of the hook to unregister
   */
  void unregisterHook(PhaseHookID hook);

  /**
   * \brief Start the next phase collectively.
   *
   * \note Performs a reduction to coordinate across nodes and then triggers
   * post-phase triggerable actions. This function does not return until the any
   * post-phase actions, including migrations as a result, are terminated.
   */
  void nextPhaseCollective();

private:
  /**
   * \internal
   * \brief Reduce handler to kick off the next phase
   *
   * \param[in] msg the (empty) next phase message
   */
  void nextPhaseReduce(NextMsg* msg);

  /**
   * \internal
   * \brief Reduce handler to synchronize the end of the phase transition
   *
   * \param[in] msg the (empty) next phase message
   */
  void nextPhaseDone(NextMsg* msg);

  /**
   * \internal
   * \brief Run all the hooks registered here of a certain variety
   *
   * \param[in] type type of hook to run designated by the enum \c PhaseHook
   */
  void runHooks(PhaseHook type);

public:
  /**
   * \brief Run hooks manually
   *
   * \warning This is only intended to be used for testing and non-standard
   * use cases where they need to be run.
   */
  void runHooksManual(PhaseHook type);

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | cur_phase_
      | proxy_
      | collective_hooks_
      | rooted_hooks_
      | next_collective_hook_id_
      | next_rooted_hook_id_
      | in_next_phase_collective_
      | reduce_next_phase_done_
      | reduce_finished_;
  }

private:
  PhaseType cur_phase_ = 0;                 /**< Current phase */
  ObjGroupProxyType proxy_ = no_obj_group;  /**< Objgroup proxy  */
  HookIDMapType collective_hooks_;          /**< Collective regisstered hooks */
  HookIDMapType rooted_hooks_;              /**< Rooted regisstered hooks  */
  std::size_t next_collective_hook_id_ = 1; /**< Next ID for collective hooks */
  std::size_t next_rooted_hook_id_ = 1;     /**< Next ID for rooted hooks */
  bool in_next_phase_collective_ = false;   /**< Whether blocked in next phase */
  bool reduce_next_phase_done_ = false;     /**< Whether reduce is complete */
  bool reduce_finished_ = false;            /**< Whether next phase is done */
};

}} /* end namespace vt::phase */

namespace vt {

extern phase::PhaseManager* thePhase();

}  /* end namespace vt */

#endif /*INCLUDED_VT_PHASE_PHASE_MANAGER_H*/
