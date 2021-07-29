/*
//@HEADER
// *****************************************************************************
//
//                           load_stats_replayer.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_LOAD_STATS_REPLAYER_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_LOAD_STATS_REPLAYER_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/baselb/baselb_msgs.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/runtime/component/component_pack.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/vrt/collection/balance/stats_driven_collection.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct LoadStatsReplayer
 *
 * \brief A VT component for replaying a run from the loads stored in stats
 * files but allowing new decisions about load balancing.
 *
 * A common flow is to output stats files while running vt at scale and then
 * reading those files in with this component.  It will initialize a collection
 * with elements at the locations found in the stats files for the specified
 * phase.  A load model that respects the elm loads in the stats files will
 * be used, and new load balancing decisions can be made by whatever load
 * balancer you selected.  This allows testing a load balancer or fine-tuning
 * the LB configuration you want to use for future at-scale runs.
 */
struct LoadStatsReplayer : runtime::component::Component<LoadStatsReplayer> {
public:
  using IndexVec = std::vector<uint64_t>;
  using ElmIDType = StatsDrivenCollection<Index2D>::ElmIDType;
  using PhaseLoadsMapType = StatsDrivenCollection<Index2D>::PhaseLoadsMapType;
  using ElmPhaseLoadsMapType = std::unordered_map<
    ElmIDType, PhaseLoadsMapType
  >;

private:
  struct ElmToIndexQueryMsg : vt::Message {
    using ElmIDType = LoadStatsReplayer::ElmIDType;

    ElmIDType elm_id_;
    vt::NodeType src_;

    explicit ElmToIndexQueryMsg(ElmIDType elm_id, vt::NodeType src)
      : elm_id_(elm_id), src_(src)
    { }
  };

public:
  LoadStatsReplayer() = default;

  void setProxy(objgroup::proxy::Proxy<LoadStatsReplayer> in_proxy);

  static std::unique_ptr<LoadStatsReplayer> construct();

  std::string name() override { return "LoadStatsReplayer"; }

  void startup() override;

  void createAndConfigureForReplay(
    std::size_t coll_elms_per_node, std::size_t initial_phase,
    std::size_t phases_to_run
  );

  void createCollectionAndModel(
    std::size_t coll_elms_per_node, std::size_t initial_phase
  );

  ElmPhaseLoadsMapType loadStatsToReplay(
    std::size_t initial_phase, std::size_t phases_to_run
  );

  void configureCollectionForReplay(
    const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
  );

  IndexVec getIndexFromElm(ElmIDType elm_id);

  template <typename IndexType>
  IndexType getTypedIndexFromElm(ElmIDType elm_id) {
    vtAbort("getTypedIndexFromElm: unexpected index type");
    return IndexType();
  }

  void addElmToIndexMapping(ElmIDType elm_id, Index1D index);
  void addElmToIndexMapping(ElmIDType elm_id, Index2D index);
  void addElmToIndexMapping(ElmIDType elm_id, Index3D index);
  void addElmToIndexMapping(ElmIDType elm_id, IndexVec index);

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | proxy_
      | elm_to_index_mapping_
      | loads_by_elm_by_phase_;
  }

private:
  ElmPhaseLoadsMapType readStats(
    std::size_t initial_phase, std::size_t phases_to_run
  );

  ElmPhaseLoadsMapType inputStatsFile(
    std::string const& filename, std::size_t initial_phase,
    std::size_t phases_to_run
  );

  void configureElementLocations(
    const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
  );

  void configureCollectionWithLoads(
    const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
  );

  void requestElmIndices(const ElmPhaseLoadsMapType &loads_by_elm_by_phase);

  static void requestElmToIndexMapping(ElmToIndexQueryMsg *msg);

  void migrateInitialObjectsHere(
    const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
  );

  void stuffStatsIntoCollection(
    const ElmPhaseLoadsMapType &loads_by_elm_by_phase, std::size_t initial_phase
  );

private:
  /// \brief Proxy for communicating
  objgroup::proxy::Proxy<LoadStatsReplayer> proxy_;

  /// \brief Proxy for created collection
  vt::CollectionProxy<StatsDrivenCollection<Index2D>, Index2D> coll_proxy_;

  /// \brief Mapping from element ids to vt indices
  std::unordered_map<ElmIDType, IndexVec> elm_to_index_mapping_;

  /// \brief Loads imported from stats files
  ElmPhaseLoadsMapType loads_by_elm_by_phase_;
};

}}}} /* end namespace vt::vrt::collection::balance */

namespace vt {

extern vrt::collection::balance::LoadStatsReplayer* theLoadStatsReplayer();

} /* end namespace vt */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_LOAD_STATS_REPLAYER_H*/
