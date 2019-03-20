/*
//@HEADER
// ************************************************************************
//
//                          greedylb.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/greedylb/greedylb.fwd.h"
#include "vt/vrt/collection/balance/greedylb/greedylb_types.h"
#include "vt/vrt/collection/balance/greedylb/greedylb_constants.h"
#include "vt/vrt/collection/balance/greedylb/greedylb_msgs.h"
#include "vt/vrt/collection/balance/proc_stats.h"
#include "vt/timing/timing.h"

#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include <list>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct GreedyLB : GreedyLBTypes {
  using ElementLoadType = std::unordered_map<ObjIDType,TimeType>;
  using ProcStatsMsgType = balance::ProcStatsMsg;
  using TransferType = std::map<NodeType, std::vector<ObjIDType>>;
  using LoadType = double;

  GreedyLB() = default;

public:
  void procDataIn(ElementLoadType const& data_in);

private:
  LoadType loadMilli(LoadType const& load);
  void reduceLoad();
  void loadStats(LoadType const& avg_load, LoadType const& max_load);
  static void loadStatsHandler(ProcStatsMsgType* msg);
  ObjBinType histogramSample(LoadType const& load);
  void finishedLB();
  void reduceCollect();
  void calcLoadOver();
  void loadOverBin(ObjBinType bin, ObjBinListType& bin_list);
  void runBalancer(ObjSampleType&& objs, LoadProfileType&& profile);
  void transferObjs(std::vector<GreedyProc>&& load);
  NodeType objGetNode(ObjIDType const& id);
  ObjIDType objSetNode(NodeType const& node, ObjIDType const& id);
  void recvObjsDirect(GreedyLBTypes::ObjIDType* objs);
  void finishedTransferExchange();
  static void recvObjsHan(GreedyLBTypes::ObjIDType* objs);

  struct GreedyAvgLoad {
    void operator()(balance::ProcStatsMsg* msg);
  };

  struct CentralCollect {
    void operator()(GreedyCollectMsg* msg);
  };

public:
  static void greedyLBHandler(balance::GreedyLBMsg* msg);
  static std::unique_ptr<GreedyLB> greedy_lb_inst;

private:
  double greedy_max_threshold = 0.0f;
  double greedy_threshold = 0.0f;
  bool greedy_auto_threshold = true;
  double this_threshold = 0.0f;
  TimeType start_time_ = 0.0f;
  LoadType avg_load = 0.0f, max_load = 0.0f;
  LoadType this_load = 0.0f, this_load_begin = 0.0f;
  ElementLoadType const* stats = nullptr;
  ObjSampleType obj_sample, load_over;
  std::size_t load_over_size = 0;
  int64_t transfer_count = 0;
  TransferType transfers;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_H*/
