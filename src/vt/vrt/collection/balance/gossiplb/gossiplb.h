/*
//@HEADER
// *****************************************************************************
//
//                                  gossiplb.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_GOSSIPLB_GOSSIPLB_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_GOSSIPLB_GOSSIPLB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/gossiplb/gossip_msg.h"
#include "vt/vrt/collection/balance/gossiplb/criterion.h"

#include <random>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace lb {

/// Enum for gossiping approach
enum struct InformTypeEnum : uint8_t {
  /**
   * \brief Synchronous sharing of underloaded processor loads
   *
   * The round number is defined at the processor level. This approach
   * propagates known loads after all messages for a round are received,
   * maximizing the amount of information propagated per round, but has a
   * synchronization cost.
   */
  SyncInform  = 0,
  /**
   * \brief Asynchronous sharing of underloaded processor loads
   *
   * The round number is defined at the message level. This approach
   * propagates known loads when the first message for a round is received,
   * avoiding the synchronization cost but delaying the propagation of some
   * information until the following round.
   */
  AsyncInform = 1
};

/// Enum for the order in which local objects are considered for transfer
enum struct ObjectOrderEnum : uint8_t {
  Arbitrary = 0, //< Arbitrary order: iterate as defined by the unordered_map
  /**
   * \brief By element ID
   *
   * Sort ascending by the ID member of ElementIDStruct.
   */
  ElmID     = 1,
  /**
   * \brief Order for the fewest migrations
   *
   * Order starting with the object with the smallest load that can be
   * transferred to drop the processor load below the average, then by
   * descending load for objects with smaller loads, and finally by ascending
   * load for objects with larger loads.
   */
  FewestMigrations = 2,
  /**
   * \brief Order for migrating the objects with the smallest loads
   *
   * Find the object with the smallest load where the sum of its own load and
   * all smaller loads meets or exceeds the amount by which this processor's
   * load exceeds the target load. Order starting with that object, then by
   * descending load for objects with smaller loads, and finally by ascending
   * load for objects with larger loads.
   */
  SmallObjects = 3
};

/// Enum for how the CMF is computed
enum struct CMFTypeEnum : uint8_t {
  /**
   * \brief Original approach
   *
   * Remove processors from the CMF as soon as they exceed the target (e.g.,
   * processor-avg) load. Use a CMF factor of 1.0/x, where x is the target load.
   */
  Original   = 0,
  /**
   * \brief Compute the CMF factor using the largest processor load in the CMF
   *
   * Do not remove processors from the CMF that exceed the target load until the
   * next iteration. Use a CMF factor of 1.0/x, where x is the greater of the
   * target load and the load of the most loaded processor in the CMF.
   */
  NormByMax  = 1,
  /**
   * \brief Compute the CMF factor using the load of this processor
   *
   * Do not remove processors from the CMF that exceed the target load until the
   * next iteration. Use a CMF factor of 1.0/x, where x is the load of the
   * processor that is computing the CMF.
   */
  NormBySelf = 2
};

struct GossipLB : BaseLB {
  using GossipMsgAsync = balance::GossipMsgAsync;
  using GossipMsgSync  = balance::GossipMsg;
  using NodeSetType    = std::vector<NodeType>;
  using ObjsType       = std::unordered_map<ObjIDType, LoadType>;
  using ReduceMsgType  = vt::collective::ReduceNoneMsg;
  using GossipRejectionMsgType = balance::GossipRejectionStatsMsg;

  GossipLB() = default;
  GossipLB(GossipLB const&) = delete;

  virtual ~GossipLB() {}

public:
  void init(objgroup::proxy::Proxy<GossipLB> in_proxy);
  void runLB() override;
  void inputParams(balance::SpecEntry* spec) override;

  static std::vector<ObjIDType> orderObjects(
    ObjectOrderEnum obj_ordering,
    std::unordered_map<ObjIDType, TimeType> cur_objs,
    LoadType this_new_load, TimeType target_max_load
  );

protected:
  void doLBStages(TimeType start_imb);
  void informAsync();
  void informSync();
  void decide();
  void migrate();

  void propagateRound(uint8_t k_cur_async, bool sync, EpochType epoch = no_epoch);
  void propagateIncomingAsync(GossipMsgAsync* msg);
  void propagateIncomingSync(GossipMsgSync* msg);
  bool isUnderloaded(LoadType load) const;
  bool isUnderloadedRelaxed(LoadType over, LoadType under) const;
  bool isOverloaded(LoadType load) const;

  std::vector<double> createCMF(NodeSetType const& under);
  NodeType sampleFromCMF(NodeSetType const& under, std::vector<double> const& cmf);
  std::vector<NodeType> makeUnderloaded() const;
  ElementLoadType::iterator selectObject(
    LoadType size, ElementLoadType& load, std::set<ObjIDType> const& available
  );

  void lazyMigrateObjsTo(EpochType epoch, NodeType node, ObjsType const& objs);
  void inLazyMigrations(balance::LazyMigrationMsg* msg);
  void gossipStatsHandler(StatsMsgType* msg);
  void gossipRejectionStatsHandler(GossipRejectionMsgType* msg);
  void thunkMigrations();

  void setupDone(ReduceMsgType* msg);

private:
  uint8_t f_                                        = 4;
  uint8_t k_max_                                    = 4;
  uint8_t k_cur_                                    = 0;
  uint16_t iter_                                    = 0;
  uint16_t trial_                                   = 0;
  uint16_t num_iters_                               = 4;
  /**
   * \brief Number of trials
   *
   * How many times to repeat the requested number of iterations, hoping to find
   * a better imbalance. This helps if it's easy to get stuck in a local minimum.
   */
  uint16_t num_trials_                              = 3;
  /**
   * \brief Whether to make migration choices deterministic
   *
   * This will only lead to reproducibility when paired with deterministic
   * object loads, for example when using a driver that feeds the load balancer
   * object loads read from vt stats files.
   */
  bool deterministic_                               = false;
  /**
   * \brief Whether to roll back to the best iteration
   *
   * If the final iteration of a trial has a worse imbalance than any earier
   * iteration, it will roll back to the best iteration.
   */
  bool rollback_                                    = true;
  /**
   * \brief Whether to adjust the target load when we have a long pole
   *
   * When an object load exceeds the processor-average load (i.e., we have a
   * "long pole"), adjust the target load to be the maximum object load
   * ("longest pole") instead of the processor-average load.
   */
  bool target_pole_                                 = false;
  std::random_device seed_;
  std::unordered_map<NodeType, LoadType> load_info_ = {};
  std::unordered_map<NodeType, LoadType> new_load_info_ = {};
  objgroup::proxy::Proxy<GossipLB> proxy_           = {};
  bool is_overloaded_                               = false;
  bool is_underloaded_                              = false;
  std::unordered_set<NodeType> selected_            = {};
  std::unordered_set<NodeType> underloaded_         = {};
  std::unordered_set<NodeType> new_underloaded_     = {};
  std::unordered_map<ObjIDType, TimeType> cur_objs_ = {};
  LoadType this_new_load_                           = 0.0;
  TimeType new_imbalance_                           = 0.0;
  TimeType target_max_load_                         = 0.0;
  CriterionEnum criterion_                          = CriterionEnum::ModifiedGrapevine;
  InformTypeEnum inform_type_                       = InformTypeEnum::SyncInform;
  ObjectOrderEnum obj_ordering_                     = ObjectOrderEnum::FewestMigrations;
  CMFTypeEnum cmf_type_                             = CMFTypeEnum::NormByMax;
  bool setup_done_                                  = false;
  bool propagate_next_round_                        = false;
  std::vector<bool> propagated_k_;
  std::mt19937 gen_propagate_;
  std::mt19937 gen_sample_;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_GOSSIPLB_GOSSIPLB_H*/
