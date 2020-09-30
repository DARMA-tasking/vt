/*
//@HEADER
// *****************************************************************************
//
//                                 node_stats.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_NODE_STATS_H
#define INCLUDED_VRT_COLLECTION_BALANCE_NODE_STATS_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/lb_comm.h"
#include "vt/vrt/collection/balance/phase_msg.h"
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/vrt/collection/types/migratable.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/timing/timing.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct NodeStats
 *
 * \brief A VT component that backs the instrumentation of virtualized entities
 * on each node, such as the objects that the collection manager orchestrates,
 * to provide data to the load balancing framework. The actual instrumentation
 * occurs in \c vt::vrt:collection::balance::ElementStats which is composed into
 * the elements of a collection.
 *
 * Collects statistics/timings on active function/methods for objects and
 * communication between them on each node. After collecting this data, passes
 * it to the load balancing framework, specifically the
 * \c * vt::vrt::collection::balance::LBManager
 */
struct NodeStats : runtime::component::Component<NodeStats> {
  using MigrateFnType       = std::function<void(NodeType)>;

  /**
   * \internal \brief System call to construct \c NodeStats
   */
  NodeStats() = default;

  std::string name() override { return "NodeStats"; }

private:
  /**
   * \internal \brief Setup the proxy for \c NodeStats
   *
   * \param[in] in_proxy the objgroup proxy
   */
  void setProxy(objgroup::proxy::Proxy<NodeStats> in_proxy);

public:
  /**
   * \internal \brief Construct the NodeStats component
   *
   * \return pointer to the component
   */
  static std::unique_ptr<NodeStats> construct();

  /**
   * \internal \brief Add node statistics for local object
   *
   * \param[in] col_elm the collection element pointer
   * \param[in] phase the current phase
   * \param[in] time the time the object took
   * \param[in] comm the comm graph for the object
   *
   * \return the temporary ID for the object assigned for this phase
   */
  ElementIDType addNodeStats(
    Migratable* col_elm,
    PhaseType const& phase, TimeType const& time,
    std::vector<TimeType> const& subphase_time, CommMapType const& comm
  );

  /**
   * \internal \brief Clear/reset all statistics and IDs on this node
   */
  void clearStats();

  /**
   * \internal \brief Cleanup after LB runs; convert temporary to permanent IDs
   */
  void startIterCleanup(PhaseType phase, unsigned int look_back);

  /**
   * \internal \brief Release collection after LB runs for this phase
   */
  void releaseLB();

  /**
   * \internal \brief Output stats file based on instrumented data
   *
   * The contents of the file consist of a series of records separated
   * by newlines. Each record consists of comma separated fields. The
   * first field of each record is a decimal integer phase which the
   * record describes.
   *
   * The first batch of records representing object workloads contain
   * 5 fields. The first field is the phase, as above. The second
   * field contains a unique object identifier, as a decimal
   * integer. The third field contains the object's total workload
   * during the phase, measured by elapsed time, and represented as a
   * floating-point decimal number. The fourth field contains the
   * number of subphases that made up the phase, as a decimal
   * integer. The fifth field contains a "[]"-bracketed list of
   * workloads, one decimal floating point value per subphase,
   * separate by commas.
   *
   * The second batch of records representing communication graph
   * edges contain 5 fields. The first field is the phase, as
   * above. The second and third fields are the recipient and source
   * of a communication, as decimal integers. The fourth field is the
   * weight of the edge, representing the number of bytes transmitted
   * between the end-points, as a decimal integer. The fifth field is
   * the category of the communication, relating the sender and
   * recipient and distinguishing point-to-point messages from
   * broadcasts, as a decimal integer.
   */
  void outputStatsFile();

  /**
   * \internal \brief Generate the next object element ID for LB
   */
  ElementIDType getNextElm();

  /**
   * \internal \brief Get stored object loads
   *
   * \return an observer pointer to the load map
   */
  std::unordered_map<PhaseType, LoadMapType> const* getNodeLoad() const;

  /**
   * \internal \brief Get stored object loads for individual subphases
   *
   * \return an observer pointer to the subphase load map
   */
  std::unordered_map<PhaseType, SubphaseLoadMapType> const* getNodeSubphaseLoad() const;

  /**
   * \internal \brief Get stored object comm graph
   *
   * \return an observer pointer to the comm graph
   */
  std::unordered_map<PhaseType, CommMapType> const* getNodeComm() const;

  /**
   * \internal \brief Test if this node has an object to migrate
   *
   * \param[in] obj_id the object temporary ID
   *
   * \return whether this node has the object
   */
  bool hasObjectToMigrate(ElementIDType obj_id) const;

  /**
   * \internal \brief Migrate an local object to another node
   *
   * \param[in] obj_id the object temporary ID
   * \param[in] to_node the node to migrate to
   *
   * \return whether this node has the object
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

  /**
   * \internal \brief Get the collection proxy for a given element ID
   *
   * \param[in] temp_id the temporary ID for the element for a given phase
   *
   * \return the virtual proxy if the element is part of the collection;
   * otherwise \c no_vrt_proxy
   */
  VirtualProxyType getCollectionProxyForElement(ElementIDType temp_id) const;

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
  objgroup::proxy::Proxy<NodeStats> proxy_;
  /// Node timings for each local object
  std::unordered_map<PhaseType, LoadMapType> node_data_;
  /// Node subphase timings for each local object
  std::unordered_map<PhaseType, SubphaseLoadMapType> node_subphase_data_;
  /// Local migration type-free lambdas for each object
  std::unordered_map<ElementIDType,MigrateFnType> node_migrate_;
  /// Map of temporary ID to permanent ID
  std::unordered_map<ElementIDType,ElementIDType> node_temp_to_perm_;
  /// Map of permanent ID to temporary ID
  std::unordered_map<ElementIDType,ElementIDType> node_perm_to_temp_;
  /// Map from element temporary ID to the collection's virtual proxy (untyped)
  std::unordered_map<ElementIDType,VirtualProxyType> node_collection_lookup_;
  /// Node communication graph for each local object
  std::unordered_map<PhaseType, CommMapType> node_comm_;
  /// The current element ID
  ElementIDType next_elm_;
  /// The stats file name for outputting instrumentation
  FILE* stats_file_ = nullptr;
  /// Whether the stats directory has been created
  bool created_dir_ = false;
};

}}}} /* end namespace vt::vrt::collection::balance */

namespace vt {

extern vrt::collection::balance::NodeStats* theNodeStats();

} /* end namespace vt */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_NODE_STATS_H*/
