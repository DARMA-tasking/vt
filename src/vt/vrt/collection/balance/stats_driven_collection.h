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
#include "vt/vrt/collection/balance/stats_driven_types.h"
#include "vt/vrt/collection/balance/stats_driven_collection_mapper.h"

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
  using NullMsg = vt::CollectionMessage<ThisType>;

  struct MigrateHereMsg : vt::CollectionMessage<ThisType> {
    MigrateHereMsg() = default;

    explicit MigrateHereMsg(vt::NodeType src)
      : src_(src)
    { }

    vt::NodeType src_ = vt::uninitialized_destination;
  };

  struct LoadStatsDataMsg : vt::CollectionMessage<ThisType> {
    using MessageParentType = vt::CollectionMessage<ThisType>;
    vt_msg_serialize_required();

    LoadStatsDataMsg() = default;

    explicit LoadStatsDataMsg(const PhaseLoadsMapType &stats)
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

    InitialPhaseMsg(
      std::size_t initial_phase, std::size_t phases_to_simulate)
      : phase_(initial_phase),
        phases_to_simulate_(phases_to_simulate)
    { }

    std::size_t phase_ = 0;
    std::size_t phases_to_simulate_ = 0;
  };

  struct EmulateMsg : vt::CollectionMessage<ThisType> {
    EmulateMsg() = default;

    explicit EmulateMsg(std::size_t real_phase)
      : phase_(real_phase)
    { }

    std::size_t phase_ = 0;
  };

  struct ResultMsg : vt::Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_required();

    ResultMsg() = default;

    explicit ResultMsg(std::size_t result_bytes)
      : result_(result_bytes) { }
    std::vector<char> result_;

    template <typename Serializer>
    void serialize(Serializer& s) {
      MessageParentType::serialize(s);
      s | result_;
    }
  };

  StatsDrivenCollection() = default;

  void setInitialPhase(InitialPhaseMsg* msg) {
    initial_phase_ = msg->phase_;
    phases_to_simulate_ = msg->phases_to_simulate_;
  }

  static void migrateInitialObjectsHere(
    ProxyType coll_proxy, const ElmPhaseLoadsMapType &loads_by_elm_by_phase,
    std::size_t initial_phase, StatsDrivenCollectionMapper<IndexType> &mapper
  );

  void migrateSelf(MigrateHereMsg* msg);

  void recvLoadStatsData(LoadStatsDataMsg *msg);

  vt::TimeType getLoad(int real_phase);

  std::size_t getPayloadSize(int real_phase);

  std::size_t getReturnSize(int real_phase);

  void emulate(EmulateMsg *msg);

  static void recvResult(ResultMsg *msg);

  template <typename Serializer>
  void serialize(Serializer& s) {
    vt::Collection<ThisType, IndexType>::serialize(s);
    s | stats_to_replay_
      | initial_phase_
      | phases_to_simulate_
      | payload_;
  }

  virtual void epiMigrateIn();

private:
  /// \brief Loads to feed into StatsReplay load model
  PhaseLoadsMapType stats_to_replay_;
  /// \brief Initial phase for replaying (offset so this is simulated phase 0)
  std::size_t initial_phase_ = -1;
  /// \brief How many phases to simulate before switching to emulation
  std::size_t phases_to_simulate_ = 0;
  /// \brief Additional payload that would need to be serialized
  std::vector<char> payload_;
  /// \brief Mapper from collection element IDs to indices
  static StatsDrivenCollectionMapper<IndexType> *mapping_;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_H*/
