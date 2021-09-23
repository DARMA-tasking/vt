/*
//@HEADER
// *****************************************************************************
//
//                                   hierlb.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb.fwd.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_types.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_child.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_msgs.h"
#include "vt/vrt/collection/balance/hierarchicallb/hierlb_strat.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/timing/timing.h"
#include "vt/objgroup/headers.h"

#include <unordered_map>
#include <vector>
#include <list>
#include <map>
#include <memory>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct HierarchicalLB : BaseLB {
  using ChildPtrType = std::unique_ptr<HierLBChild>;
  using ChildMapType = std::unordered_map<NodeType,ChildPtrType>;
  using ElementLoadType = std::unordered_map<ObjIDType,TimeType>;
  using TransferType = std::map<NodeType, std::vector<ObjIDType>>;
  using LoadType = double;

  HierarchicalLB() = default;
  virtual ~HierarchicalLB() {}

  void init(objgroup::proxy::Proxy<HierarchicalLB> in_proxy);
  void runLB() override;
  void inputParams(balance::SpecEntry* spec) override;

  static std::unordered_map<std::string, std::string> getInputKeysWithHelp();

  void setupTree();
  void calcLoadOver(HeapExtractEnum const extract);
  void loadOverBin(ObjBinType bin, ObjBinListType& bin_list);
  void procDataIn(ElementLoadType const& data_in);

private:
  double getAvgLoad() const;
  double getMaxLoad() const;
  double getSumLoad() const;

  void downTreeHandler(LBTreeDownMsg* msg);
  void lbTreeUpHandler(LBTreeUpMsg* msg);
  void setupDone(SetupDoneMsg* msg);

  void downTreeSend(
    NodeType const node, NodeType const from, ObjSampleType const& excess,
    bool const final_child, std::size_t const& approx_size
  );
  void lbTreeUpSend(
    NodeType const node, LoadType const child_load, NodeType const child,
    ObjSampleType const& load, NodeType const child_size
  );
  void downTree(
    NodeType const from, ObjSampleType excess, bool const final_child
  );
  void lbTreeUp(
    LoadType const child_load, NodeType const child, ObjSampleType load,
    NodeType const child_size
  );

  void sendDownTree();
  void distributeAmoungChildren();
  void clearObj(ObjSampleType& objs);
  HierLBChild* findMinChild();
  void startMigrations();

private:
  std::size_t getSize(ObjSampleType const&);

private:
  void loadStats();

private:
  double this_threshold = 0.5f;
  double I_tolerance = 0.05f;
  bool tree_setup = false;
  NodeType parent = uninitialized_destination;
  NodeType bottom_parent = uninitialized_destination;
  NodeType agg_node_size = 0, child_msgs = 0;
  ChildMapType children;
  LoadType this_load_begin = 0.0f;
  ObjSampleType load_over, given_objs, taken_objs;
  int64_t migrates_expected = 0, transfer_count = 0;
  TransferType transfers;
  objgroup::proxy::Proxy<HierarchicalLB> proxy = {};
  HeapExtractEnum extract_strategy = HeapExtractEnum::LoadOverLessThan;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_HIERARCHICALLB_HIERLB_H*/
