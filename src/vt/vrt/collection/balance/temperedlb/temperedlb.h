/*
//@HEADER
// *****************************************************************************
//
//                                 temperedlb.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPEREDLB_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPEREDLB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/stats_msg.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/vrt/collection/balance/temperedlb/tempered_msgs.h"
#include "vt/vrt/collection/balance/temperedlb/criterion.h"
#include "vt/vrt/collection/balance/temperedlb/tempered_enums.h"

#include <random>
#include <unordered_map>
#include <map>
#include <unordered_set>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct TemperedLB : BaseLB {
  using LoadMsgAsync   = balance::LoadMsgAsync;
  using LoadMsgSync    = balance::LoadMsg;
  using NodeSetType    = std::vector<NodeType>;
  using ObjsType       = std::unordered_map<ObjIDType, LoadType>;
  using ReduceMsgType  = vt::collective::ReduceNoneMsg;
  using RejectionMsgType = balance::RejectionStatsMsg;
  using StatsMsgType     = balance::NodeStatsMsg;
  using QuantityType     = std::map<lb::StatisticQuantity, double>;
  using StatisticMapType = std::unordered_map<lb::Statistic, QuantityType>;

  TemperedLB() = default;
  TemperedLB(TemperedLB const&) = delete;

  virtual ~TemperedLB() {}

public:
  void init(objgroup::proxy::Proxy<TemperedLB> in_proxy);
  void runLB() override;
  void inputParams(balance::SpecEntry* spec) override;

  static std::unordered_map<std::string, std::string> getInputKeysWithHelp();

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
  void propagateIncomingAsync(LoadMsgAsync* msg);
  void propagateIncomingSync(LoadMsgSync* msg);
  bool isUnderloaded(LoadType load) const;
  bool isUnderloadedRelaxed(LoadType over, LoadType under) const;
  bool isOverloaded(LoadType load) const;

  std::vector<double> createCMF(NodeSetType const& under);
  NodeType sampleFromCMF(NodeSetType const& under, std::vector<double> const& cmf);
  std::vector<NodeType> makeUnderloaded() const;
  std::vector<NodeType> makeSufficientlyUnderloaded(
    LoadType load_to_accommodate
  ) const;
  ElementLoadType::iterator selectObject(
    LoadType size, ElementLoadType& load, std::set<ObjIDType> const& available
  );

  void lazyMigrateObjsTo(EpochType epoch, NodeType node, ObjsType const& objs);
  void inLazyMigrations(balance::LazyMigrationMsg* msg);
  void loadStatsHandler(StatsMsgType* msg);
  void rejectionStatsHandler(RejectionMsgType* msg);
  void thunkMigrations();

  void setupDone(ReduceMsgType* msg);

private:
  uint16_t f_                                       = 0;
  uint8_t k_max_                                    = 0;
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
  uint16_t num_trials_                              = 1;
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
  objgroup::proxy::Proxy<TemperedLB> proxy_         = {};
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
  InformTypeEnum inform_type_                       = InformTypeEnum::AsyncInform;
  ObjectOrderEnum obj_ordering_                     = ObjectOrderEnum::FewestMigrations;
  CMFTypeEnum cmf_type_                             = CMFTypeEnum::NormByMax;
  KnowledgeEnum knowledge_                          = KnowledgeEnum::Log;
  bool setup_done_                                  = false;
  bool propagate_next_round_                        = false;
  std::vector<bool> propagated_k_;
  std::mt19937 gen_propagate_;
  std::mt19937 gen_sample_;
  StatisticMapType stats;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_TEMPEREDLB_TEMPEREDLB_H*/
