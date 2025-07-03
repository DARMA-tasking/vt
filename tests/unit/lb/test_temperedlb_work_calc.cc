/*
//@HEADER
// *****************************************************************************
//
//                         test_temperedlb_work_calc.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#include <vt/transport.h>
#include <vt/vrt/collection/balance/temperedlb/temperedlb.h>

#include "test_helpers.h"
#include "test_parallel_harness.h"

namespace vt::tests::unit::lb {

#if vt_check_enabled(lblite)

using TestTemperedLBWorkCalc = TestParallelHarness;

/**
 *
 * This is the graph we are intending to test.

111111111111111111111111111111111111   2222222222222222222222222222222222222222
1                                  1   2                                      2
1                                  1   2                                      2
1                                  1   2   ««o12»»                            2
1       c1               c2        1   2«««       »»        c3                2
1     .....            .....       1 «««            »»»  .......              2
1    .     .          .     .      ««  2              .»»       ...           2
1   .       .        .       .  «««1   2             .   »»        .          2
1  .         .      .        «««   1   2            .      »»       .         2
1  . o1»»»o2«««««««««««««o3«««««««««««««««««««««««««««««««««o4      .         2
1  .  »    »».      . «« »»   .    1   2            .       »«      .         2
1   .  »»   »»       «   » » .     1   2             .    » «      .          2
1    .   » . »»    «« .  »  »»     1   2              ..»   «   ...           2
1     ....»   »o10«    .»...  »    1   2               »  .«....              2
1          »            »      »   1   2             »»   «                   2
111111111111»11111111111»1111111»111   2222222222222»2222«222222222222222222222
             »          »        »                »»     «
             »          »         »»            »»      «
          3333»33333333»333333333333»3333333333»3333333«3333333
          3    »       »             »       »»        «      3
          3     »      »o11«          »     »         «       3
          3     »          «           »  »»  c5     «        3
          3      »          «           »»  ....... «         3
          3       »       c4«          »  ».       .«         3
          3        »    .....«.      »»  . »o7«««o8« .        3
          3         »...     « ...  »   .   »»    «   .       3
          3         »         «   »»    .    »    «   .       3
          3          »        «  » .    .    »»  ««   .       3
          3        .  »o5»»»»o6»»»»»»»»»»»»»»»o9««   .        3
          3        .     ««««      .      ..       ..         3
          3         .             .         .......           3
          3          ...       ...                            3
          3             .......                               3
          3                                                   3
          33333333333333333333333333333333333333333333333333333

**/

auto getObjLoads(
  std::unordered_map<elm::ElementIDStruct, LoadType> const& map
) -> double {
  double load = 0;
  for (auto const& [_, value] : map) {
    load += value;
  }
  return load;
};

auto getNodeEdges(
  std::unordered_map<elm::ElementIDStruct, LoadType> const& objs,
  std::unordered_map<
    elm::ElementIDStruct, std::vector<std::tuple<elm::ElementIDStruct, double>>
  > const& edge_map,
  bool off
) -> double {
  double total_bytes = 0;
  for (auto const& [obj_id, _] : objs) {
    for (auto const& [obj_id2, vec] : edge_map) {
      if (obj_id == obj_id2) {
        for (auto const& [obj_id3, bytes] : vec) {
          bool edge_off_node = objs.find(obj_id3) == objs.end();
          if (edge_off_node and off) {
            total_bytes += bytes;
          } else if (not edge_off_node and not off) {
            total_bytes += bytes;
          }
        }
      }
    }
  }
  return total_bytes;
};

auto computeOffHomeVolume(
  vt::NodeType node,
  std::unordered_map<elm::ElementIDStruct, LoadType> const& objs,
  std::unordered_map<elm::ElementIDStruct, SharedIDType> const& obj_shared_block,
  std::unordered_map<
    SharedIDType, std::tuple<NodeType, vt::vrt::collection::lb::BytesType>
  > const& shared_edge
) -> double {
  double total_bytes = 0;
  std::set<SharedIDType> shared_here;
  for (auto const& [obj_id, _] : objs) {
    if (auto it = obj_shared_block.find(obj_id); it != obj_shared_block.end()) {
      auto shared_id = it->second;
      auto [shared_node, shared_bytes] = shared_edge.find(shared_id)->second;
      if (shared_node != node) {
        total_bytes += shared_bytes;
      }
    }
  }
  return total_bytes;
}

using TemperedLB = vt::vrt::collection::lb::TemperedLB;
using ClusterInfo = vt::vrt::collection::lb::ClusterInfo;
using NodeInfo = vt::vrt::collection::lb::NodeInfo;
using ObjIDType = vt::vrt::collection::lb::BaseLB::ObjIDType;
using WorkBreakdown = vt::vrt::collection::lb::WorkBreakdown;

auto getObjLoad(
  ObjIDType obj_id,
  std::map<NodeType, std::unordered_map<ObjIDType, LoadType>> cur_objs
) -> double {
  for (auto const& [rank, map] : cur_objs) {
    for (auto const& [other_obj_id, load] : map) {
      if (other_obj_id == obj_id) {
        return load;
      }
    }
  }
  return 0;
}

auto testClusterSwap(
  TemperedLB* tlb,
  NodeType rank,
  std::unordered_map<NodeType, WorkBreakdown> work_init,
  std::unordered_map<SharedIDType, ClusterInfo> cluster_info,
  std::map<NodeType, std::unordered_map<ObjIDType, LoadType>> cur_objs,
  std::unordered_map<ObjIDType, SharedIDType> obj_shared_block,
  SharedIDType to_remove, SharedIDType to_add
) {
  // Set initial distribution
  tlb->setCurObjs(cur_objs[rank]);
  // Get shared blocks for initial distribution
  auto blocks_here_initial = tlb->getSharedBlocksHere();

  auto cur_objs_add_remove = cur_objs[rank];
  for (auto const& [obj_id, shared_id] : obj_shared_block) {
    if (shared_id == to_add) {
      cur_objs_add_remove[obj_id] = getObjLoad(obj_id, cur_objs);
    }
    if (shared_id == to_remove) {
      cur_objs_add_remove.erase(cur_objs_add_remove.find(obj_id));
    }
  }

  std::set<ObjIDType> non_cluster_objs;
  for (auto const& [elm_id, _] : cur_objs[rank]) {
    if (obj_shared_block.find(elm_id) == obj_shared_block.end()) {
      non_cluster_objs.insert(elm_id);
    }
  }

  tlb->setCurObjs(cur_objs_add_remove);
  auto wb2 = tlb->computeWorkBreakdown(rank, cur_objs_add_remove);

  NodeInfo ni{
    getObjLoads(cur_objs_add_remove),
    work_init[rank].work,
    work_init[rank].inter_send_vol, work_init[rank].inter_recv_vol,
    work_init[rank].intra_send_vol, work_init[rank].intra_recv_vol,
    work_init[rank].shared_vol,
    blocks_here_initial,
    non_cluster_objs
  };

  ClusterInfo cluster_to_remove =
    to_remove != -1 ? cluster_info[to_remove] : ClusterInfo{};
  ClusterInfo cluster_to_add =
    to_add != -1 ? cluster_info[to_add] : ClusterInfo{};

  auto new_work = tlb->computeWorkAfterClusterSwap(
    rank, ni, cluster_to_remove, cluster_to_add
  );

  EXPECT_NEAR(new_work, wb2.work, FLT_EPSILON);

  vt_print(gen, "new_work={}, wb2={}\n", new_work, wb2.work);
}


TEST_F(TestTemperedLBWorkCalc, test_work_calc_1) {
  using BytesType = vt::vrt::collection::lb::BytesType;

  auto tlb = std::make_unique<TemperedLB>();

  double const alpha = 1.0;
  double const beta = 0.4;
  double const gamma = 0.2;
  double const delta = 0.1;

  tlb->setAlpha(alpha);
  tlb->setBeta(beta);
  tlb->setGamma(gamma);
  tlb->setDelta(delta);

  // Test some arbitrary values of each term to make sure they are computed
  // correctly
  double work = tlb->computeWork(1000, 100, 50, 20);
  EXPECT_EQ(
    work,
    alpha * 1000 +
    beta  * 100 +
    gamma * 50 +
    delta * 20
  );

  // rank 1 objects
  auto o1 = elm::ElmIDBits::createCollectionImpl(true, 1, 1, 1);
  auto o2 = elm::ElmIDBits::createCollectionImpl(true, 2, 1, 1);
  auto o3 = elm::ElmIDBits::createCollectionImpl(true, 3, 1, 1);
  auto o10 = elm::ElmIDBits::createCollectionImpl(true, 10, 1, 1);

  // rank 2 objects
  auto o4 = elm::ElmIDBits::createCollectionImpl(true, 4, 2, 2);
  auto o12 = elm::ElmIDBits::createCollectionImpl(true, 12, 2, 2);

  // rank 3 objects
  auto o5 = elm::ElmIDBits::createCollectionImpl(true, 5, 3, 3);
  auto o6 = elm::ElmIDBits::createCollectionImpl(true, 6, 3, 3);
  auto o7 = elm::ElmIDBits::createCollectionImpl(true, 7, 3, 3);
  auto o8 = elm::ElmIDBits::createCollectionImpl(true, 8, 3, 3);
  auto o9 = elm::ElmIDBits::createCollectionImpl(true, 9, 3, 3);
  auto o11 = elm::ElmIDBits::createCollectionImpl(true, 11, 3, 3);

  // clusters
  std::unordered_map<ObjIDType, SharedIDType> obj_shared_block = {
    {o1, 1}, {o2, 1},
    {o3, 2},
    {o4, 3},
    {o5, 4}, {o6, 4},
    {o7, 5}, {o8, 5}, {o9, 5}
  };

  std::unordered_map<SharedIDType, BytesType> shared_block_size = {
    {1, 100},
    {2, 200},
    {3, 150},
    {4, 300},
    {5, 100}
  };

  std::map<NodeType, std::unordered_map<ObjIDType, LoadType>> cur_objs = {
    {1,
      {{o1, 20},
       {o2, 5},
       {o3, 10},
       {o10, 3}
      }
    },
    {2,
     {{o4, 30},
      {o12, 9}
     }
    },
    {3,
      {{o5, 10},
       {o6, 15},
       {o7, 3},
       {o8, 2},
       {o9, 8},
       {o11, 1}
      }
    }
  };

  std::unordered_map<SharedIDType, std::tuple<NodeType, BytesType>> shared_edge = {
    {1, {1, 100}},
    {2, {1, 200}},
    {3, {2, 150}},
    {4, {3, 300}},
    {5, {3, 100}}
  };

  std::unordered_map<
    elm::ElementIDStruct, std::vector<std::tuple<elm::ElementIDStruct, double>>
  > send_edges, recv_edges;

  send_edges = {
    {o1, {{o2, 10}, {o5, 10}}},
    {o2, {{o10, 10}}},
    {o3, {{o2, 10}, {o7, 10}, {o10, 10}, {o11, 10}}},
    {o4, {{o3, 10}, {o8, 10}}},
    {o5, {{o6, 10}}},
    {o6, {{o4, 10}, {o5, 10}, {o9, 10}, {o11, 10}}},
    {o7, {{o9, 10}}},
    {o8, {{o7, 10}, {o9, 10}}},
    {o12, {{o3, 10}, {o4, 10}}}
  };

  recv_edges = {
    {o2, {{o1, 10}, {o3, 10}}},
    {o3, {{o4, 10}, {o12, 10}}},
    {o4, {{o6, 10}, {o12, 10}}},
    {o5, {{o1, 10}, {o6, 10}}},
    {o6, {{o5, 10}}},
    {o7, {{o3, 10}, {o8, 10}}},
    {o8, {{o4, 10}}},
    {o9, {{o6, 10}, {o7, 10}, {o8, 10}}},
    {o10, {{o2, 10}, {o3, 10}}},
    {o11, {{o3, 10}, {o6, 10}}}
  };

  tlb->setObjSharedBlock(obj_shared_block);
  tlb->setSharedSize(shared_block_size);
  tlb->setSharedEdge(shared_edge);
  tlb->setSendEdges(send_edges);
  tlb->setRecvEdges(recv_edges);

  std::vector<NodeType> ranks = {1, 2, 3};
  std::unordered_map<NodeType, NodeInfo> node_info;
  std::unordered_map<SharedIDType, ClusterInfo> cluster_info;
  std::unordered_map<NodeType, WorkBreakdown> work_init;

  for (auto const& rank : ranks) {
    tlb->setCurObjs(cur_objs[rank]);

    auto wb = tlb->computeWorkBreakdown(rank, cur_objs[rank]);
    vt_print(gen, "rank={}, work={}\n", rank, wb.work);
    work_init[rank] = wb;

    EXPECT_NEAR(
      wb.work,
      alpha * getObjLoads(cur_objs[rank]) +
      beta * std::max(
        getNodeEdges(cur_objs[rank], send_edges, true),
        getNodeEdges(cur_objs[rank], recv_edges, true)
      ) +
      gamma * std::max(
        getNodeEdges(cur_objs[rank], send_edges, false),
        getNodeEdges(cur_objs[rank], recv_edges, false)
      ) +
      delta * computeOffHomeVolume(
        rank, cur_objs[rank], obj_shared_block, shared_edge
      ),
      FLT_EPSILON
    );

    node_info[rank] = NodeInfo{
      getObjLoads(cur_objs[rank]),
      wb.work,
      wb.inter_send_vol, wb.inter_recv_vol,
      wb.intra_send_vol, wb.intra_recv_vol,
      wb.shared_vol, tlb->getSharedBlocksHere(), {}
    };

    for (auto const& shared_id : tlb->getSharedBlocksHere()) {
      cluster_info[shared_id] = tlb->makeClusterSummary(shared_id);
    }
  }

  // Test removal of all clusters that exist on a given rank (one-by-one)
  for (auto const& rank : ranks) {
    for (auto const& [shared_id, rank_bytes] : shared_edge) {
      auto const shared_rank = std::get<0>(rank_bytes);
      if (rank == shared_rank) {
        vt_print(
          temperedlb, "try remove: rank={}, shared_id={}\n", rank, shared_id
        );
        testClusterSwap(
          tlb.get(), rank, work_init, cluster_info, cur_objs, obj_shared_block,
          shared_id, -1
        );
      }
    }
  }

  // Test addition of all cluster that don't exist on a given rank
  for (auto const& rank : ranks) {
    std::set<SharedIDType> to_try_add;
    for (auto const& rank2 : ranks) {
      if (rank != rank2) {
        for (auto const& [obj, shared_id] : obj_shared_block) {
          if (std::get<0>(shared_edge[shared_id]) != rank) {
            to_try_add.insert(shared_id);
          }
        }
      }
    }

    for (auto const& shared_id : to_try_add) {
      vt_print(temperedlb, "try add: rank={}, shared_id={}\n", rank, shared_id);
      testClusterSwap(
        tlb.get(), rank, work_init, cluster_info, cur_objs, obj_shared_block,
        -1, shared_id
      );
    }
  }

  // Test cluster swaps
  for (auto const& rank : ranks) {
    std::set<SharedIDType> to_try_add;
    for (auto const& rank2 : ranks) {
      if (rank != rank2) {
        for (auto const& [obj, shared_id] : obj_shared_block) {
          if (std::get<0>(shared_edge[shared_id]) != rank) {
            to_try_add.insert(shared_id);
          }
        }
      }
    }

    for (auto const& [shared_id_remove, rank_bytes] : shared_edge) {
      auto const shared_rank = std::get<0>(rank_bytes);
      if (rank == shared_rank) {
        for (auto const& shared_id_add : to_try_add) {
          vt_print(
            temperedlb,
            "try swap: rank={}, shared_id_remove={}, shared_id_add={}\n",
            rank, shared_id_remove, shared_id_add
          );
          testClusterSwap(
            tlb.get(), rank, work_init, cluster_info, cur_objs, obj_shared_block,
            shared_id_remove, shared_id_add
          );
        }
      }
    }
  }

}

#endif /* vt_check_enabled(lblite) */

} /* end namespace vt::tests::unit::lb */
