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
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/stats_driven_collection.h"
#include "vt/vrt/collection/balance/load_stats_replayer.h"
#include "vt/vrt/collection/balance/stats_driven_collection_mapper.impl.h"

#include <thread>
#include <chrono>

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename IndexType>
/*static*/
StatsDrivenCollectionMapper<IndexType>*
StatsDrivenCollection<IndexType>::mapping_ = nullptr;

template <typename IndexType>
/*static*/
std::unordered_map<
  NodeType,
  typename StatsDrivenCollection<IndexType>::buffer_contributions_type
> StatsDrivenCollection<IndexType>::ret_counts_ = {};

template <typename IndexType>
/*static*/
void StatsDrivenCollection<IndexType>::migrateInitialObjectsHere(
  ProxyType coll_proxy, const ElmPhaseLoadsMapType &loads_by_elm_by_phase,
  std::size_t initial_phase, StatsDrivenCollectionMapper<IndexType> &mapper
) {
  // loop over stats elms that were local for initial phase, asking for the
  // corresponding collection elements to be migrated here
  mapping_ = &mapper;
  auto const this_rank = vt::theContext()->getNode();
  for (auto &item : loads_by_elm_by_phase) {
    auto elm_id = item.first;
    auto &loads_by_phase = item.second;
    auto it = loads_by_phase.find(initial_phase);
    if (it != loads_by_phase.end()) {
      auto index = mapper.getIndexFromElm(elm_id);
      if (coll_proxy[index].tryGetLocalPtr() != nullptr) {
        vt_debug_print(
          normal, replay,
          "index {} (elm {}) is already here\n",
          index, elm_id
        );
      } else {
        vt_debug_print(
          normal, replay,
          "requesting index {} (elm {}) to migrate here\n",
          index, elm_id
        );
        coll_proxy[index].template send<
          MigrateHereMsg, &StatsDrivenCollection<IndexType>::migrateSelf
        >(this_rank);
      }
    }
  }
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
  vtAssert(initial_phase_ >= 0, "Initial phase did not get set before load was queried");
  auto simulated_phase = real_phase + initial_phase_;
  vt_debug_print(
    verbose, replay,
    "getLoad: index {} has load for real_phase={} (sim_phase={} given initial_phase_={})\n",
    this->getIndex(), real_phase, simulated_phase, initial_phase_
  );
  return stats_to_replay_[simulated_phase].duration;
}

template <typename IndexType>
std::size_t StatsDrivenCollection<IndexType>::getPayloadSize(int real_phase) {
  vtAssert(initial_phase_ >= 0, "Initial phase did not get set before payload size was queried");
  auto simulated_phase = real_phase + initial_phase_;
  auto bytes = stats_to_replay_[simulated_phase].serialized_bytes;
  if (bytes > 0) {
    // subtract off the serialized size of data needed for functionality
    auto adjust = ::checkpoint::getSize(stats_to_replay_);
    return (bytes > adjust ? bytes - adjust : 0);
  }
  return 0;
}

template <typename IndexType>
std::size_t StatsDrivenCollection<IndexType>::getReturnSize(int real_phase) {
  vtAssert(initial_phase_ >= 0, "Initial phase did not get set before return size was queried");
  auto simulated_phase = real_phase + initial_phase_;
  return stats_to_replay_[simulated_phase].callback_bytes;
}

template <typename IndexType>
void StatsDrivenCollection<IndexType>::emulate(EmulateMsg *msg) {
  using clock = std::chrono::high_resolution_clock;

  // start tracking real time usage
  clock::time_point start = clock::now();

  auto idx = this->getIndex();

  // set up the serialized size needed for the next phase
  if (msg->phase_ >= phases_to_simulate_ - 1) {
    auto payload_size = getPayloadSize(msg->phase_ + 1);
    payload_.resize(payload_size);
    auto achieved = ::checkpoint::getSize(*this);

    auto simulated_phase = msg->phase_ + 1 + initial_phase_;
    vt_debug_print(
      normal, replay,
      "emulate: index {} has serialized size {} when {} was requested\n",
      idx, achieved, stats_to_replay_[simulated_phase].serialized_bytes
    );
  }

  if (msg->phase_ >= phases_to_simulate_) {
    vt_debug_print(
      normal, replay,
      "emulate: emulating index {} in phase {}\n",
      idx, msg->phase_
    );

    // do any real work in here where we can hide the time cost
    double time_to_emulate = getLoad(msg->phase_);

    // figure out how much time we've wasted on real work
    double adjust = std::chrono::duration_cast<std::chrono::duration<double>>(
      clock::now() - start
    ).count();

    if (time_to_emulate > adjust) {
      // subtract off time already wasted and spin for what remains
      std::size_t remaining_time_us = (time_to_emulate - adjust) * 1e6;
      auto duration = std::chrono::microseconds(remaining_time_us);
      std::this_thread::sleep_for(duration);
    } else {
      vt_print(
        replay,
        "emulate: warning: really ran {} sec when {} sec was desired\n",
        adjust, time_to_emulate
      );
    }

    contributeResult(msg->phase_);
  } else {
    vt_debug_print(
      normal, replay,
      "emulate: simulating index {} in phase {}\n",
      idx, msg->phase_
    );
    (void) start;
  }
}

template <typename IndexType>
void StatsDrivenCollection<IndexType>::contributeResult(
  PhaseType phase
) {
  // send result back to home rank if we're not at home
  auto idx = this->getIndex();
  auto home = mapping_->getKnownHome(idx);
  vtAssert(home != uninitialized_destination, "Home must be known");
  if (home != theContext()->getNode()) {
    auto return_size = getReturnSize(phase);
    if (return_size > 0) {
      if (!accumulate_contributions_) {
        vt_debug_print(
          terse, replay,
          "emulate: index {} is sending {} bytes to home rank {}\n",
          idx, return_size, home
        );
        auto ret_msg = makeMessage<ResultMsg>(return_size);
        theMsg()->sendMsg<ResultMsg, recvResult>(home, ret_msg);
      } else {
        // ***THIS IS AN APPLICATION-SPECIFIC OPTIMIZATION***
        vt_debug_print(
          terse, replay,
          "emulate: index {} is accumulating {} bytes to home rank {}\n",
          idx, return_size, home
        );
        // @todo: verify that the matrix contribution is of the expected size
        auto &counts = ret_counts_[home];
        (counts.first)++;
        if (counts.first == counts.second) {
          vt_debug_print(
            terse, replay,
            "emulate: all {} contributions received; sending {} bytes to home rank {}\n",
            counts.second, return_size, home
          );
          auto ret_msg = makeMessage<ResultMsg>(return_size);
          theMsg()->sendMsg<ResultMsg, recvResult>(home, ret_msg);
        }
      }
    }
  }
}

/*static*/
template <typename IndexType>
void StatsDrivenCollection<IndexType>::recvResult(ResultMsg *msg) {
  vt_debug_print(
    terse, replay,
    "emulate: received {} result bytes\n",
    msg->result_.size()
  );
}

template <typename IndexType>
void StatsDrivenCollection<IndexType>::epiMigrateIn() {
  auto elm_id = this->getElmID().id;
  auto index = this->getIndex();
  mapping_->addElmToIndexMapping(elm_id, index);

  // ***THIS IS AN APPLICATION-SPECIFIC OPTIMIZATION***
  auto home = this->getElmID().home_node;
  auto &counts = ret_counts_[home];
  (counts.second)++;
}

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_STATS_DRIVEN_COLLECTION_IMPL_H*/
