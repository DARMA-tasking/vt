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
#include <unordered_set>
#include <vector>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct RejectionStats {
  RejectionStats() = default;
  RejectionStats(int n_rejected, int n_transfers)
    : n_rejected_(n_rejected), n_transfers_(n_transfers) { }

  friend RejectionStats operator+(RejectionStats a1, RejectionStats const& a2) {
    a1.n_rejected_ += a2.n_rejected_;
    a1.n_transfers_ += a2.n_transfers_;

    return a1;
  }

  int n_rejected_ = 0;
  int n_transfers_ = 0;
};

struct GossipRejectionStatsMsg : collective::ReduceTMsg<RejectionStats> {
  GossipRejectionStatsMsg() = default;
  GossipRejectionStatsMsg(int n_rejected, int n_transfers)
    : ReduceTMsg<RejectionStats>(RejectionStats(n_rejected, n_transfers))
  { }
  GossipRejectionStatsMsg(RejectionStats&& rs)
    : ReduceTMsg<RejectionStats>(std::move(rs))
  { }
};

struct GossipLB : BaseLB {
  using GossipMsgAsync = balance::GossipMsgAsync;
  using NodeSetType    = std::vector<NodeType>;
  using ObjsType       = std::unordered_map<ObjIDType, LoadType>;
  using ReduceMsgType  = vt::collective::ReduceNoneMsg;

  GossipLB() = default;

  GossipLB(GossipLB const&) = delete;

  virtual ~GossipLB() {}

public:
  void init(objgroup::proxy::Proxy<GossipLB> in_proxy);
  void runLB() override;
  void inputParams(balance::SpecEntry* spec) override;

protected:
  void doLBStages(TimeType start_imb);
  void informAsync();
  void decide();
  void migrate();

  void propagateRoundAsync(uint8_t k_cur_async, EpochType epoch = no_epoch);
  void propagateIncomingAsync(GossipMsgAsync* msg);
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
  void gossipRejectionStatsHandler(GossipRejectionStatsMsg* msg);
  void thunkMigrations();

  void setupDone(ReduceMsgType* msg);

private:
  uint8_t f_                                        = 4;
  uint8_t k_max_                                    = 4;
  uint8_t k_cur_                                    = 0;
  uint16_t iter_                                    = 0;
  uint16_t trial_                                   = 0;
  uint16_t num_iters_                               = 4;
  uint16_t num_trials_                              = 3;
  std::random_device seed_;
  std::unordered_map<NodeType, LoadType> load_info_ = {};
  objgroup::proxy::Proxy<GossipLB> proxy_           = {};
  bool is_overloaded_                               = false;
  bool is_underloaded_                              = false;
  std::unordered_set<NodeType> selected_            = {};
  std::unordered_set<NodeType> underloaded_         = {};
  std::unordered_map<ObjIDType, TimeType> cur_objs_ = {};
  LoadType this_new_load_                           = 0.0;
  TimeType new_imbalance_                           = 0.0;
  CriterionEnum criterion_                          = CriterionEnum::ModifiedGrapevine;
  bool setup_done_                                  = false;
  std::vector<bool> propagated_k_;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_GOSSIPLB_GOSSIPLB_H*/
