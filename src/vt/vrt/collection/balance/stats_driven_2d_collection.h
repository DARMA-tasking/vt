/*
//@HEADER
// *****************************************************************************
//
//                        stats_driven_2d_collection.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_2D_COLLECTION_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_2D_COLLECTION_H

#include "vt/config.h"
#include "vt/vrt/collection/collection_headers.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

/**
 * \struct StatsDriven2DCollection
 *
 * \brief A collection that can be used with the StatsDrivenLoadModel to replay
 * task costs from imported stats files.
 */
struct StatsDriven2DCollection : vt::Collection<
  StatsDriven2DCollection, vt::Index2D
> {
  using ElmIDType = ElementIDType;
  using PhaseLoadsMapType = std::unordered_map<
    std::size_t /*phase from stats file*/, vt::TimeType
  >;

  using NullMsg = vt::CollectionMessage<StatsDriven2DCollection>;

  struct ElmToIndexMappingMsg : vt::Message {
    using ElmIDType = StatsDriven2DCollection::ElmIDType;

    vt::Index2D index_;
    ElmIDType elm_id_;

    explicit ElmToIndexMappingMsg(vt::Index2D index, ElmIDType elm_id)
      : index_(index), elm_id_(elm_id)
    { }
  };

  struct MigrateHereMsg : vt::CollectionMessage<StatsDriven2DCollection> {
    MigrateHereMsg() = default;

    MigrateHereMsg(vt::NodeType src)
      : src_(src)
    { }

    vt::NodeType src_ = vt::uninitialized_destination;
  };

  struct LoadStatsDataMsg : vt::CollectionMessage<StatsDriven2DCollection> {
    using MessageParentType = vt::CollectionMessage<StatsDriven2DCollection>;
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

  struct InitialPhaseMsg : vt::CollectionMessage<StatsDriven2DCollection> {
    InitialPhaseMsg() = default;

    InitialPhaseMsg(std::size_t initial_phase)
      : phase_(initial_phase)
    { }

    std::size_t phase_ = 0;
  };

  StatsDriven2DCollection() = default;

  inline static vt::NodeType collectionMap(
    vt::Index2D* idx, vt::Index2D*, vt::NodeType
  ) {
    return idx->x();
  }

  void setInitialPhase(InitialPhaseMsg* msg);

  void shareElmToIndexMapping(NullMsg* msg);

  void migrateSelf(MigrateHereMsg* msg);

  void recvLoadStatsData(LoadStatsDataMsg *msg);

  vt::TimeType getLoad(int real_phase);

  template <typename Serializer>
  void serialize(Serializer& s) {
    vt::Collection<StatsDriven2DCollection, vt::Index2D>::serialize(s);
    s | stats_to_replay_
      | initial_phase_;
  }

  virtual void epiMigrateIn();

private:
  /// \brief Loads to feed into StatsReplay load model
  PhaseLoadsMapType stats_to_replay_;
  /// \brief Initial phase for replaying (offset so this is simulated phase 0)
  std::size_t initial_phase_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_2D_COLLECTION_H*/
