/*
//@HEADER
// *****************************************************************************
//
//                                  greedylb.h
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

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_H
#define INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/greedylb/greedylb.fwd.h"
#include "vt/vrt/collection/balance/greedylb/greedylb_types.h"
#include "vt/vrt/collection/balance/greedylb/greedylb_constants.h"
#include "vt/vrt/collection/balance/greedylb/greedylb_msgs.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/timing/timing.h"

#include <unordered_map>
#include <map>
#include <vector>
#include <memory>
#include <list>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct GreedyLB : BaseLB {
  using ElementLoadType  = std::unordered_map<ObjIDType,TimeType>;
  using TransferType     = std::map<NodeType, std::vector<ObjIDType>>;
  using LoadType         = double;
  using LoadProfileType  = std::unordered_map<NodeType,LoadType>;

  GreedyLB() = default;
  virtual ~GreedyLB() {}

  void init(objgroup::proxy::Proxy<GreedyLB> in_proxy);
  void runLB() override;
  void inputParams(balance::SpecEntry* spec) override;
  void cleanup() override;

private:
  double getAvgLoad() const;
  double getMaxLoad() const;
  double getSumLoad() const;

  void loadStats();
  void finishedLB();
  void reduceCollect();
  void calcLoadOver();
  void loadOverBin(ObjBinType bin, ObjBinListType& bin_list);
  void runBalancer(ObjSampleType&& objs, LoadProfileType&& profile);
  void transferObjs(std::vector<GreedyProc>&& load);
  ObjIDType objSetNode(NodeType const& node, ObjIDType const& id);
  void recvObjsDirect(GreedyLBTypes::ObjIDType* objs);
  void finishedTransferExchange();
  void collectHandler(GreedyCollectMsg* msg);

  // This must stay static due to limitations in the scatter implementation
  // (does not work with objgroups)
  static void recvObjsHan(GreedyLBTypes::ObjIDType* objs);
  static objgroup::proxy::Proxy<GreedyLB> scatter_proxy;

private:
  double this_threshold = 0.0f;
  LoadType this_load_begin = 0.0f;
  ObjSampleType load_over;
  objgroup::proxy::Proxy<GreedyLB> proxy = {};

  // Parameters read from LB spec file
  double max_threshold = 0.0f;
  double min_threshold = 0.0f;
  bool auto_threshold = true;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_GREEDYLB_GREEDYLB_H*/
