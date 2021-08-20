/*
//@HEADER
// *****************************************************************************
//
//                         stats_driven_collection.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_H

#include "vt/config.h"
#include "vt/vrt/collection/collection_headers.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct StatsDrivenCollection
 *
 * \brief A collection that can be used with the StatsDrivenLoadModel to replay
 * task costs from imported stats files.
 */
template <typename IndexType>
struct StatsDrivenCollection : vt::Collection<
  StatsDrivenCollection<IndexType>, IndexType
> {
  using ThisType = StatsDrivenCollection<IndexType>;
  using ProxyType = vt::CollectionProxy<ThisType, IndexType>;
  using IndexVec = std::vector<uint64_t>;
  using ElmIDType = ElementIDType;
  using PhaseLoadsMapType = std::unordered_map<
    std::size_t /*phase from stats file*/, vt::TimeType
  >;
  using ElmPhaseLoadsMapType = std::unordered_map<
    ElmIDType, PhaseLoadsMapType
  >;

  using NullMsg = vt::CollectionMessage<ThisType>;

  struct MigrateHereMsg : vt::CollectionMessage<ThisType> {
    MigrateHereMsg() = default;

    MigrateHereMsg(vt::NodeType src)
      : src_(src)
    { }

    vt::NodeType src_ = vt::uninitialized_destination;
  };

  struct LoadStatsDataMsg : vt::CollectionMessage<ThisType> {
    using MessageParentType = vt::CollectionMessage<ThisType>;
    vt_msg_serialize_required();

    LoadStatsDataMsg() = default;

    LoadStatsDataMsg(const PhaseLoadsMapType &stats)
      : stats_(stats)
    { }

    template <typename Serializer>
    void serialize(Serializer& s) {
      MessageParentType::serialize(s);
      s | stats_;
    }

    PhaseLoadsMapType stats_;
  };

  struct InitialPhaseMsg : vt::CollectionMessage<ThisType> {
    InitialPhaseMsg() = default;

    InitialPhaseMsg(std::size_t initial_phase)
      : phase_(initial_phase)
    { }

    std::size_t phase_ = 0;
  };

  StatsDrivenCollection() = default;

  inline static NodeType collectionMap(
    IndexType* idx, IndexType* bounds, NodeType num_nodes
  ) {
    // correct operation here requires that the home rank specifically
    // know that it maps locally
    auto it = rank_mapping_.find(*idx);
    if (it != rank_mapping_.end()) {
      vt_debug_print(
        normal, replay,
        "collectionMap: index {} maps to rank {}\n",
        *idx, it->second
      );
      return it->second;
    }
    return uninitialized_destination;
  }

  void setInitialPhase(InitialPhaseMsg* msg) {
    initial_phase_ = msg->phase_;
  }

  static void migrateInitialObjectsHere(
    ProxyType coll_proxy, const ElmPhaseLoadsMapType &loads_by_elm_by_phase,
    std::size_t initial_phase
  );

  void migrateSelf(MigrateHereMsg* msg);

  void recvLoadStatsData(LoadStatsDataMsg *msg);

  vt::TimeType getLoad(int real_phase);

  template <typename Serializer>
  void serialize(Serializer& s) {
    vt::Collection<ThisType, IndexType>::serialize(s);
    s | stats_to_replay_
      | initial_phase_;
  }

  virtual void epiMigrateIn();

  static void addCollectionMapping(IndexType idx, NodeType home);

  static void addElmToIndexMapping(ElmIDType elm_id, IndexType index);

  static void addElmToIndexMapping(ElmIDType elm_id, IndexVec idx_vec);

  static IndexType getIndexFromElm(ElmIDType elm_id);

private:
  /// \brief Loads to feed into StatsReplay load model
  PhaseLoadsMapType stats_to_replay_;
  /// \brief Initial phase for replaying (offset so this is simulated phase 0)
  std::size_t initial_phase_ = -1;

  /// \brief Mapping from vt indices to home ranks for collection construction
  static std::map<IndexType, int /*mpi_rank*/> rank_mapping_;
  /// \brief Mapping from element ids to vt indices
  static std::unordered_map<ElmIDType, IndexType> elm_to_index_mapping_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_H*/
