/*
//@HEADER
// *****************************************************************************
//
//                       stats_driven_collection.impl.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_IMPLH

#include "vt/config.h"
#include "vt/vrt/collection/balance/stats_driven_collection.h"
#include "vt/vrt/collection/balance/load_stats_replayer.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename IndexType>
/*static*/
std::map<IndexType, int /*mpi_rank*/>
StatsDrivenCollection<IndexType>::rank_mapping_;

template <typename IndexType>
/*static*/
void StatsDrivenCollection<IndexType>::addMapping(
  IndexType idx, NodeType home
) {
  rank_mapping_[idx] = home;
}

template <typename IndexType>
void StatsDrivenCollection<IndexType>::setInitialPhase(InitialPhaseMsg* msg) {
  initial_phase_ = msg->phase_;
}

template <typename IndexType>
void StatsDrivenCollection<IndexType>::migrateSelf(MigrateHereMsg* msg) {
  // migrate oneself to the requesting rank
  auto const this_rank = theContext()->getNode();
  auto dest = msg->src_;
  if (dest != this_rank) {
    vt_debug_print(
      normal, replay,
      "migrateSelf: index {} asked to migrate from {} to {}\n",
      this->getIndex(), this_rank, dest
    );
    this->migrate(dest);
  }
}

template <typename IndexType>
void StatsDrivenCollection<IndexType>::recvLoadStatsData(LoadStatsDataMsg *msg) {
  vt_debug_print(
    normal, replay,
    "recvLoadStatsData: index {} received stats data for {} phases\n",
    this->getIndex(), msg->stats_.size()
  );
  stats_to_replay_.insert(msg->stats_.begin(), msg->stats_.end());
}

template <typename IndexType>
vt::TimeType StatsDrivenCollection<IndexType>::getLoad(int real_phase) {
  auto simulated_phase = real_phase + initial_phase_;
  vt_debug_print(
    verbose, replay,
    "getLoad: index {} has load for real_phase={} (sim_phase={} given initial_phase_={})\n",
    this->getIndex(), real_phase, simulated_phase, initial_phase_
  );
  return stats_to_replay_[simulated_phase];
}

template <typename IndexType>
void StatsDrivenCollection<IndexType>::epiMigrateIn() {
  auto elm_id = this->getElmID().id;
  auto index = this->getIndex();
  vt::theLoadStatsReplayer()->addElmToIndexMapping(elm_id, index);
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_IMPL_H*/
