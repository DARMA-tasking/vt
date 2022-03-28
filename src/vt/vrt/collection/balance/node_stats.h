/*
//@HEADER
// *****************************************************************************
//
//                                 node_stats.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_NODE_STATS_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_NODE_STATS_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/elm/elm_comm.h"
#include "vt/elm/elm_stats.fwd.h"
#include "vt/vrt/collection/balance/phase_msg.h"
#include "vt/vrt/collection/types/migratable.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/timing/timing.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/utils/json/base_appender.h"
#include "vt/vrt/collection/balance/stats_data.h"
#include "vt/vrt/collection/types/storage/storable.h"

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
  using StorableType = vt::vrt::collection::storage::Storable;

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
   * \internal \brief Add collection element info
   *
   * \param[in] id the element ID
   * \param[in] proxy the collection proxy
   * \param[in] index the index for the object
   * \param[in] migrate_fn the migration function
   */
  void registerCollectionInfo(
    ElementIDStruct id, VirtualProxyType proxy,
    std::vector<uint64_t> const& index, MigrateFnType migrate_fn
  );

  /**
   * \internal \brief Add objgroup element info
   *
   * \param[in] id the element ID
   * \param[in] proxy the objgroup proxy
   */
  void registerObjGroupInfo(ElementIDStruct id, ObjGroupProxyType proxy);

  /**
   * \internal \brief Add statistics for element (non-collection)
   *
   * \param[in] id the element ID
   * \param[in] in the stats
   * \param[in] focused_subphase the focused subphase (optional)
   */
  void addNodeStats(
    ElementIDStruct id, elm::ElementStats* in, StorableType *storable,
    SubphaseType focused_subphase = elm::ElementStats::no_subphase
  );

  /**
   * \internal \brief Clear/reset all statistics and IDs on this node
   */
  void clearStats();

  /**
   * \internal \brief Cleanup after LB runs
   */
  void startIterCleanup(PhaseType phase, unsigned int look_back);

  /**
   * \internal \brief Output stats file for given phase based on instrumented
   * data
   *
   * This outputs statistics in JSON format that includes task timings,
   * mappings, and communication.
   */
  void outputStatsForPhase(PhaseType phase);

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
   * \internal \brief Get stored object comm graph
   *
   * \return an observer pointer to the comm graph
   */
  std::unordered_map<PhaseType, CommMapType> const* getNodeComm() const;

  /**
   * \internal \brief Get stored object comm subphase graph
   *
   * \return an observer pointer to the comm subphase graph
   */
  std::unordered_map<PhaseType, std::unordered_map<SubphaseType, CommMapType>> const* getNodeSubphaseComm() const;

  /**
   * \internal \brief Test if this node has an object to migrate
   *
   * \param[in] obj_id the object ID struct
   *
   * \return whether this node has the object
   */
  bool hasObjectToMigrate(ElementIDStruct obj_id) const;

  /**
   * \internal \brief Migrate an local object to another node
   *
   * \param[in] obj_id the object ID struct
   * \param[in] to_node the node to migrate to
   *
   * \return whether this node has the object
   */
  bool migrateObjTo(ElementIDStruct obj_id, NodeType to_node);

  /**
   * \internal \brief Get the collection proxy for a given element ID
   *
   * \param[in] obj_id the ID struct for the element
   *
   * \return the virtual proxy if the element is part of the collection;
   * otherwise \c no_vrt_proxy
   */
  VirtualProxyType getCollectionProxyForElement(ElementIDStruct obj_id) const;

  /**
   * \internal \brief Get the objgroup proxy for a given element ID
   *
   * \param[in] obj_id the ID struct for the element
   *
   * \return the objgroup proxy if the element is part of an objgroup;
   * otherwise \c no_obj_group
   */
  ObjGroupProxyType getObjGroupProxyForElement(ElementIDStruct obj_id) const;

  void initialize() override;
  void finalize() override;
  void fatalError() override;

  /**
   * \brief Get the underlying stats data
   *
   * \warning For testing only!
   *
   * \return the stats data
   */
  StatsData* getStatsData() { return stats_.get(); }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | proxy_
      | node_migrate_
      | node_collection_lookup_
      | node_objgroup_lookup_
      | next_elm_
      | created_dir_
      | stats_;
  }

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
  /// Local migration type-free lambdas for each object
  std::unordered_map<ElementIDStruct,MigrateFnType> node_migrate_;
  /// Map from element ID to the collection's virtual proxy (untyped)
  std::unordered_map<ElementIDStruct,VirtualProxyType> node_collection_lookup_;
  /// Map from element ID to the objgroup's proxy (untyped)
  std::unordered_map<ElementIDStruct,ObjGroupProxyType> node_objgroup_lookup_;
  /// The current element ID
  ElementIDType next_elm_ = 1;
  /// Whether the stats directory has been created
  bool created_dir_ = false;
  /// The appender for outputting stat files in JSON format
  std::unique_ptr<util::json::BaseAppender> stat_writer_ = nullptr;
  /// The struct that holds all the statistic data
  std::unique_ptr<StatsData> stats_ = nullptr;
};

}}}} /* end namespace vt::vrt::collection::balance */

namespace vt {

extern vrt::collection::balance::NodeStats* theNodeStats();

} /* end namespace vt */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_NODE_STATS_H*/
