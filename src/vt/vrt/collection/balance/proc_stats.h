/*
//@HEADER
// *****************************************************************************
//
//                                 proc_stats.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/lb_comm.h"
#include "vt/vrt/collection/balance/phase_msg.h"
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/timing/timing.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace balance {

struct ProcStats : runtime::component::Component<ProcStats> {
  using MigrateFnType = std::function<void(NodeType)>;
  using LoadMapType   = std::unordered_map<ElementIDType,TimeType>;

  ProcStats() = default;

  std::string name() override { return "ProcStats"; }

private:
  /**
   * \internal \brief Setup the proxy for \c ProcStats
   *
   * \param[in] in_proxy the objgroup proxy
   */
  void setProxy(objgroup::proxy::Proxy<ProcStats> in_proxy);

public:
  /**
   * \internal \brief Construct the ProcStats component
   *
   * \return pointer to the component
   */
  static std::unique_ptr<ProcStats> construct();

  /**
   * \internal \brief Add processor statistics for local object
   *
   * \param[in] elm_proxy the element proxy to the object
   * \param[in] col_elm the collection element pointer
   * \param[in] phase the current phase
   * \param[in] time the time the object took
   * \param[in] comm the comm graph for the object
   *
   * \return the temporary ID for the object assigned for this phase
   */
  template <typename ColT>
  ElementIDType addProcStats(
    VirtualElmProxyType<ColT> const& elm_proxy, ColT* col_elm,
    PhaseType const& phase, TimeType const& time, CommMapType const& comm
  );

  /**
   * \internal \brief Clear/reset all statistics and IDs on this processor
   */
  void clearStats();

  /**
   * \internal \brief Cleanup after LB runs; convert temporary to permanent IDs
   */
  void startIterCleanup();

  /**
   * \internal \brief Release collection after LB runs for this phase
   */
  void releaseLB();

  /**
   * \internal \brief Output stats file based on instrumented data
   */
  void outputStatsFile();

  /**
   * \internal \brief Generate the next object element ID for LB
   */
  ElementIDType getNextElm();

  /**
   * \internal \brief Get object loads for a given phase
   *
   * \param[in] phase the phase
   *
   * \return the load map
   */
  LoadMapType const& getProcLoad(PhaseType phase) const;

  /**
   * \internal \brief Get object comm graph for a given phase
   *
   * \param[in] phase the phase
   *
   * \return the load map
   */
  CommMapType const& getProcComm(PhaseType phase) const;

  /**
   * \internal \brief Test if this processor has an object to migrate
   *
   * \param[in] obj_id the object temporary ID
   *
   * \return whether this processor has the object
   */
  bool hasObjectToMigrate(ElementIDType obj_id) const;

  /**
   * \internal \brief Migrate an local object to another node
   *
   * \param[in] obj_id the object temporary ID
   * \param[in] to_node the node to migrate to
   *
   * \return whether this processor has the object
   */
  bool migrateObjTo(ElementIDType obj_id, NodeType to_node);

  /**
   * \internal \brief Convert temporary element ID to permanent Returns
   * \c no_element_id if not found.
   * \param[in] temp_id temporary ID
   *
   * \return permanent ID
   */
  ElementIDType tempToPerm(ElementIDType temp_id) const;

  /**
   * \internal \brief Convert permanent element ID to temporary. Returns
   * \c no_element_id if not found.
   *
   * \param[in] perm_id permanent ID
   *
   * \return temporary ID
   */
  ElementIDType permToTemp(ElementIDType perm_id) const;

private:
  /**
   * \internal \brief Create the stats file
   */
  void createStatsFile();

  /**
   * \internal \brief Close the stats file
   */
  void closeStatsFile();

private:
  /// Local proxy to objgroup
  objgroup::proxy::Proxy<ProcStats> proxy_;
  /// Processor timings for each local object
  std::vector<LoadMapType> proc_data_;
  /// Local migration type-free lambdas for each object
  std::unordered_map<ElementIDType,MigrateFnType> proc_migrate_;
  /// Map of temporary ID to permanent ID
  std::unordered_map<ElementIDType,ElementIDType> proc_temp_to_perm_;
  /// Map of permanent ID to temporary ID
  std::unordered_map<ElementIDType,ElementIDType> proc_perm_to_temp_;
  /// Processor communication graph for each local object
  std::vector<CommMapType> proc_comm_;
  /// The current element ID
  ElementIDType next_elm_;
  /// The stats file name for outputting instrumentation
  FILE* stats_file_ = nullptr;
  /// Whether the stats directory has been created
  bool created_dir_ = false;
};

}}}} /* end namespace vt::vrt::collection::balance */

namespace vt {

extern vrt::collection::balance::ProcStats* theProcStats();

} /* end namespace vt */

#include "vt/vrt/collection/balance/proc_stats.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_PROC_STATS_H*/
