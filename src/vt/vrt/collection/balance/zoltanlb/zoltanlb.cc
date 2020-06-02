/*
//@HEADER
// *****************************************************************************
//
//                                 zoltanlb.cc
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_CC
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_CC

#include "vt/config.h"
#include "vt/vrt/collection/balance/zoltanlb/zoltanlb.h"
#include "vt/collective/collective_alg.h"

#if backend_check_enabled(zoltan)

#include <zoltan.h>

namespace vt { namespace vrt { namespace collection { namespace lb {

namespace {

template <typename Callable>
static void executeInEpoch(Callable&& fn) {
  auto ep = vt::theTerm()->makeEpochCollective();
  vt::theMsg()->pushEpoch(ep);
  fn();
  vt::theMsg()->popEpoch(ep);
  vt::theTerm()->finishedEpoch(ep);
  bool done = false;
  vt::theTerm()->addAction(ep, [&done]{ done = true; });
  theSched()->runSchedulerWhile([&done]{ return not done; });
}

} /* end anon namespace */

ZoltanLB::ZoltanLB()
  : collective_scope_(theCollective()->makeCollectiveScope())
{ }

void ZoltanLB::init(objgroup::proxy::Proxy<ZoltanLB> in_proxy) {
  proxy = in_proxy;
}

void ZoltanLB::inputParams(balance::SpecEntry* spec) {
  zoltan_config_ = {
    {"LB_APPROACH",                   "REPARTITION" },
    {"DEBUG_LEVEL",                   "0"           },
    {"FINAL_OUTPUT",                  "0"           },
    {"DEBUG_PROCESSOR",               "0"           },
    {"CHECK_HYPERGRAPH",              "1"           },
    {"PHG_EDGE_WEIGHT_OPERATION",     "max"         },
    {"IMBALANCE_TOL",                 "1.00"        },
    {"PHG_REPART_MULTIPLIER",         "100"         },
    {"PHG_EDGE_WEIGHT_OPERATION",     "max"         },
    {"PHG_CUT_OBJECTIVE",             "connectivity"},
    {"PHG_OUTPUT_LEVEL",              "0"           },
    {"PHG_COARSENING_METHOD",         "agg"         },
    {"PHG_COARSEPARTITION_METHOD",    "auto"        },
    {"PHG_REFINEMENT_QUALITY",        "1"           },
    {"PHG_RANDOMIZE_INPUT",           "0"           },
    {"PHG_EDGE_SIZE_THRESHOLD",       "0.25"        },
    {"PHG_PROCESSOR_REDUCTION_LIMIT", "0.5"         },
    {"PHG_REFINEMENT_METHOD",         ""            },
    {"PHG_MULTILEVEL",                ""            },
    {"SEED",                          ""            }
  };

  std::vector<std::string> allowed{"edges", "use_shared_edges"};
  for (auto& c : zoltan_config_) {
    allowed.push_back(c.first);
  }
  spec->checkAllowedKeys(allowed);

  do_edges_ = spec->getOrDefault<bool>("edges", do_edges_);
  use_shared_edges_ = spec->getOrDefault<bool>(
    "use_shared_edges", use_shared_edges_
  );

  for (auto& c : zoltan_config_) {
    c.second = spec->getOrDefault<std::string>(c.first, c.second);
  }
}

void ZoltanLB::runLB() {
  auto const& this_node = theContext()->getNode();

  if (this_node == 0) {
    vt_print(lb, "ZoltanLB: runLB: edges={}\n", do_edges_);
    fflush(stdout);
  }

  if (do_edges_) {
    executeInEpoch([this]{ makeGraphSymmetric();    });
    executeInEpoch([this]{ combineEdges();          });
    executeInEpoch([this]{ countEdges();            });
    executeInEpoch([this]{ allocateShareEdgeGIDs(); });
  }

  initZoltan();
  setParams();
  auto graph = makeGraph();

  Zoltan_Set_Num_Obj_Fn         (zoltan_, getNumberOfVertices,      graph.get());
  Zoltan_Set_Obj_List_Fn        (zoltan_, getVertexList,            graph.get());
  Zoltan_Set_HG_Size_CS_Fn      (zoltan_, getHypergraphSize,        graph.get());
  Zoltan_Set_HG_CS_Fn           (zoltan_, getHypergraph,            graph.get());
  Zoltan_Set_HG_Size_Edge_Wts_Fn(zoltan_, getHypergraphEdgeSize,    graph.get());
  Zoltan_Set_HG_Edge_Wts_Fn     (zoltan_, getHypergraphEdgeWeights, graph.get());

  int changes = 0;
  int num_gid_entries = 0, num_lid_entries = 0, num_import = 0, num_export = 0;
  ZOLTAN_ID_PTR import_global_ids;
  ZOLTAN_ID_PTR import_local_ids;
  ZOLTAN_ID_PTR export_global_ids;
  ZOLTAN_ID_PTR export_local_ids;
  int* import_procs = nullptr, *export_procs = nullptr;
  int* import_to_part = nullptr, *export_to_part = nullptr;

  collective_scope_.mpiCollectiveWait([&]{
    auto const partition_return = Zoltan_LB_Partition(
      zoltan_, &changes, &num_gid_entries, &num_lid_entries,
      &num_import,
      &import_global_ids, &import_local_ids, &import_procs, &import_to_part,
      &num_export,
      &export_global_ids, &export_local_ids, &export_procs, &export_to_part
    );
    vtAssert(partition_return == ZOLTAN_OK, "Partition must be OK");
  });

  startMigrationCollective();

  debug_print(
    lb, node,
    "ZoltanLB: num_export={}, num_import={}\n",
    num_export, num_import
  );

  for (int i = 0; i < num_export; i++) {
    int to_node = export_procs[i];

    debug_print(
      lb, node,
      "migrateObjTo: to_node={} gid={:x}\n",
      to_node, export_global_ids[i]
    );

    migrateObjectTo(export_global_ids[i], static_cast<NodeType>(to_node));
  }

  finishMigrationCollective();

  Zoltan_LB_Free_Part(
    &import_global_ids, &import_local_ids, &import_procs, &import_to_part
  );
  Zoltan_LB_Free_Part(
    &export_global_ids, &export_local_ids, &export_procs, &export_to_part
  );

  destroyZoltan();
}

void ZoltanLB::makeGraphSymmetric() {
  auto const this_node = theContext()->getNode();

  // Go through the comm graph and extract out paired SendRecv edges that are
  // not self-send and have a non-local edge
  std::unordered_map<NodeType, ElementCommType> shared_edges;

  for (auto&& elm : *comm_data) {
    if (
      elm.first.cat_ == balance::CommCategory::SendRecv and
      not elm.first.selfEdge()
    ) {
      auto from = elm.first.fromObjTemp();
      auto to = elm.first.toObjTemp();

      auto from_node = objGetNode(from);
      auto to_node = objGetNode(to);

      vtAssert(
        from_node == this_node or to_node == this_node,
        "One node must involve this node"
      );

      debug_print(
        lb, node,
        "makeGraphSymmetric: from={:x}, to={:x}\n",
        from, to
      );

      if (from_node != this_node) {
        shared_edges[from_node][elm.first] = elm.second;
      } else if (to_node != this_node) {
        shared_edges[to_node][elm.first] = elm.second;
      }

      // Only add the edges wanted now (SendRecv, no self-edge)
      load_comm_symm[elm.first] += elm.second;
    }
  }

  for (auto&& elm : shared_edges) {
    auto msg = makeMessage<CommMsg>(elm.second);
    proxy[elm.first].send<CommMsg, &ZoltanLB::recvSharedEdges>(msg.get());
  }

}

void ZoltanLB::recvSharedEdges(CommMsg* msg) {
  auto& comm = msg->comm_;
  for (auto&& elm : comm) {
    debug_print(
      lb, node,
      "recv shared edge: from={:x}, to={:x}\n",
      elm.first.fromObjTemp(), elm.first.toObjTemp()
    );

    load_comm_symm[elm.first] += elm.second;
  }
}

void ZoltanLB::combineEdges() {
  // Combine bi-directional, symmetric edges into a non-directional weight in
  // bytes the sum of receive and send bytes
  ElementCommType load_comm_combined;

  for (auto&& e1 : load_comm_symm) {
    auto from = e1.first.fromObjTemp() > e1.first.toObjTemp() ?
      e1.first.fromObjTemp() :
      e1.first.toObjTemp();
    auto to = e1.first.fromObjTemp() > e1.first.toObjTemp() ?
      e1.first.toObjTemp() :
      e1.first.fromObjTemp();

    auto key = balance::LBCommKey{
      balance::LBCommKey::CollectionTag{},
      from, from, to, to, false
    };
    load_comm_combined[key] += e1.second;
  }

  load_comm_symm = std::move(load_comm_combined);
}

void ZoltanLB::countEdges() {
  debug_print(lb, node, "countEdges\n");

  // Count the number of local and remote edges to allocation edge GIDs
  int local_edge = 0;
  int remote_owned_edge = 0;

  auto const this_node = theContext()->getNode();
  for (auto&& elm : load_comm_symm) {
    if (
      elm.first.cat_ == balance::CommCategory::SendRecv and
      not elm.first.selfEdge()
    ) {
      auto from = elm.first.fromObjTemp();
      auto to = elm.first.toObjTemp();

      auto from_node = objGetNode(from);
      auto to_node = objGetNode(to);

      if (from_node == to_node and from_node == this_node) {
        local_edge++;
      } else {
        // Break ties on non-local object edges based on obj ID
        auto large_obj_id = from > to ? from : to;
        if (objGetNode(large_obj_id) == this_node) {
          remote_owned_edge++;
        }
      }
    }
  }

  int const total_ids = local_edge + remote_owned_edge;

  debug_print(lb, node, "ZoltanLB: total_ids_={}\n", total_ids);

  auto cb = theCB()->makeBcast<ZoltanLB,ReduceMsg,&ZoltanLB::reduceCount>(proxy);
  auto msg = makeMessage<ReduceMsg>(total_ids);
  proxy.reduce<collective::MaxOp<int>>(msg.get(),cb);
}

void ZoltanLB::reduceCount(ReduceMsg* msg) {
  max_edges_per_node_ = msg->getVal();

  debug_print(
    lb, node,
    "ZoltanLB: max_edges_per_node_={}\n",
    max_edges_per_node_
  );
}

void ZoltanLB::allocateShareEdgeGIDs() {
  std::unordered_map<NodeType, ElementCommType> shared_edges;

  auto const this_node = theContext()->getNode();
  for (auto&& elm : load_comm_symm) {
    auto from = elm.first.fromObjTemp();
    auto to = elm.first.toObjTemp();

    auto from_node = objGetNode(from);
    auto to_node = objGetNode(to);

    if (from_node == to_node and from_node == this_node) {
      auto offset = max_edges_per_node_ * this_node;
      auto id = 1 + offset + edge_id_++;
      auto key = elm.first;
      key.edge_id_ = id;
      load_comm_edge_id[key] = elm.second;

      debug_print(
        lb, node,
        "allocate: local edge_id={:x}, from={:x}, to={:x}\n",
        key.edge_id_,
        key.fromObjTemp(),
        key.toObjTemp()
      );

    } else {
      auto large_obj_id = from > to ? from : to;
      if (objGetNode(large_obj_id) == this_node) {
        auto offset = max_edges_per_node_ * this_node;
        auto id = 1 + offset + edge_id_++;
        auto key = elm.first;
        key.edge_id_ = id;
        load_comm_edge_id[key] = elm.second;

        debug_print(
          lb, node,
          "allocate: remote edge_id={:x}, from={:x}, to={:x}\n",
          key.edge_id_,
          key.fromObjTemp(),
          key.toObjTemp()
        );

        if (use_shared_edges_) {
          auto other_node = from_node == this_node ? to_node : from_node;
          shared_edges[other_node][key] = elm.second;
        }
      } else {
        // If use_shared_edges_, the other node will set the ID; wait to receive
        // it in next phase. If not shared, the other node will provide the edge
      }
    }
  }

  if (use_shared_edges_) {
    for (auto&& elm : shared_edges) {
      auto msg = makeMessage<CommMsg>(elm.second);
      proxy[elm.first].send<CommMsg, &ZoltanLB::recvEdgeGID>(msg.get());
    }
  }
}

void ZoltanLB::recvEdgeGID(CommMsg* msg) {
  auto& comm = msg->comm_;
  for (auto&& elm : comm) {
    vtAssert(
      load_comm_edge_id.find(elm.first) == load_comm_edge_id.end(),
      "Must not exists in edge ID map"
    );

    debug_print(
      lb, node,
      "recvEdgeGID: edge_id={:x}, from={:x}, to={:x}\n",
      elm.first.edge_id_,
      elm.first.fromObjTemp(),
      elm.first.toObjTemp()
    );
    load_comm_edge_id[elm.first] = elm.second;
  }
}

Zoltan_Struct* ZoltanLB::initZoltan() {
  collective_scope_.mpiCollectiveWait([this]{
    float ver = 0.0f;
    auto const ret = Zoltan_Initialize(0, nullptr, &ver);
    vtAssert(ret == ZOLTAN_OK, "Zoltan initialize must be OK");

    zoltan_ = Zoltan_Create(theContext()->getComm());
  });
  return zoltan_;
}

void ZoltanLB::destroyZoltan() {
  collective_scope_.mpiCollectiveWait([this]{
    Zoltan_Destroy(&zoltan_);
  });
}

void ZoltanLB::setParams() {
  auto gparts = fmt::format("{}", theContext()->getNumNodes());

  for (auto&& c : zoltan_config_) {
    if (c.second != "") {
      Zoltan_Set_Param(zoltan_, c.first.c_str(), c.second.c_str());
    }
  }

  Zoltan_Set_Param(zoltan_, "LB_METHOD",                 "HYPERGRAPH"  );
  Zoltan_Set_Param(zoltan_, "HYPERGRAPH_PACKAGE",        "PHG"         );
  Zoltan_Set_Param(zoltan_, "NUM_GID_ENTRIES",           "1"           );
  Zoltan_Set_Param(zoltan_, "NUM_LID_ENTRIES",           "1"           );
  Zoltan_Set_Param(zoltan_, "RETURN_LISTS",              "ALL"         );
  Zoltan_Set_Param(zoltan_, "OBJ_WEIGHT_DIM",            "1"           );
  Zoltan_Set_Param(zoltan_, "EDGE_WEIGHT_DIM",           "1"           );
  Zoltan_Set_Param(zoltan_, "NUM_GLOBAL_PARTS",          gparts.c_str());
}

std::unique_ptr<ZoltanLB::Graph> ZoltanLB::makeGraph() {
  auto graph = std::make_unique<Graph>();

  // Number of local vertices (overdecomposed blocks) on this node
  graph->num_vertices = static_cast<int>(load_data->size());

  // Allocate space for each vertex to describe it
  graph->vertex_gid = std::make_unique<ZOLTAN_ID_TYPE[]>(graph->num_vertices);

  // Allocate space for the weight of each vertex (calculated as a normalized
  // weight representing time the overdecomposed block spent executing)
  graph->vertex_weight = std::make_unique<int[]>(graph->num_vertices);

  static_assert(
    sizeof(ZOLTAN_ID_TYPE) == sizeof(ObjIDType),
    "ObjIDType be exactly the same size as ZOLTAN_ID_TYPE\n"
    "Please recompile with \"-D Zoltan_ENABLE_ULLONG_IDS:Bool=ON\""
  );

  // Insert local load objs into a std::set to get a deterministic order to
  // traverse them for building the graph consistenly
  std::set<ObjIDType> load_objs;
  for (auto&& elm : *load_data) {
    load_objs.insert(elm.first);
  }

  // Initialize all the local vertices with global id
  {
    int idx = 0;
    for (auto&& obj : load_objs) {
      graph->vertex_gid[idx++] = obj;

      debug_print(
        lb, node,
        "makeVertexGraph: vertex_id={}: obj={:x}\n",
        idx - 1, obj
      );
    }
  }

  // Set the weights for all vertices, by converting to milliseconds as the
  // weight
  {
    int idx = 0;
    for (auto&& obj : load_objs) {
      auto iter = load_data->find(obj);
      auto time = static_cast<int>(loadMilli(iter->second));
      graph->vertex_weight[idx++] = time;
    }
  }

  if (do_edges_) {
    // Only get communication edges between vertices/migratable elements
    // Insert local comm objs into a std::set for deterministic ordering
    std::vector<balance::LBCommKey> comm_objs;
    for (auto&& elm : load_comm_edge_id) {
      comm_objs.push_back(elm.first);
    }

    // Set the number of communication edges
    graph->num_edges = comm_objs.size();

    // Allocate space for each edge to describe it
    graph->edge_gid = std::make_unique<ZOLTAN_ID_TYPE[]>(graph->num_edges);

    // Allocate space for each edge weight
    graph->edge_weight = std::make_unique<int[]>(graph->num_edges);

    // Allocate space for indexing array into neighbor_gid array for each edge
    graph->neighbor_idx = std::make_unique<int[]>(graph->num_edges + 1);

    // Allocate space indexing array for edge edge_gid[i] beginning at
    // neighbor_gid[neighbor_idx[i]]
    graph->neighbor_gid = std::make_unique<ZOLTAN_ID_TYPE[]>(graph->num_edges * 2);

    // Set the edge weights as bytes in the communication graph
    {
      auto const this_node = theContext()->getNode();
      int edge_idx = 0;
      int neighbor_idx = 0;

      // Set offset for first edge
      graph->neighbor_idx[edge_idx] = neighbor_idx;

      for (auto&& elm : comm_objs) {
        auto iter = load_comm_edge_id.find(elm);
        auto bytes = iter->second;

        vtAssert(
          iter->first.edge_id_ != balance::no_element_id,
          "Must have element ID"
        );

        graph->edge_gid[edge_idx] = iter->first.edge_id_;
        graph->edge_weight[edge_idx] = bytes;

        debug_print(
          lb, node,
          "makeEdgeGraph: edge_id={}: edge_idx={}, neighbor_idx={}\n",
          iter->first.edge_id_,
          edge_idx,
          neighbor_idx
        );

        auto from_node = objGetNode(iter->first.fromObjTemp());
        auto to_node = objGetNode(iter->first.toObjTemp());

        // Set up the links between communicating GIDs
        if ((use_shared_edges_ and from_node == this_node) or not use_shared_edges_) {
          debug_print(
            lb, node,
            "makeEdgeGraph: \t edge_id={}: edge_idx={}, obj={:x}\n",
            iter->first.edge_id_, edge_idx, iter->first.fromObjTemp()
          );

          graph->neighbor_gid[neighbor_idx++] = iter->first.fromObjTemp();
        }
        if ((use_shared_edges_ and to_node == this_node) or not use_shared_edges_) {
          debug_print(
            lb, node,
            "makeEdgeGraph: \t edge_id={}: edge_idx={}, obj={:x}\n",
            iter->first.edge_id_, edge_idx, iter->first.toObjTemp()
          );

          graph->neighbor_gid[neighbor_idx++] = iter->first.toObjTemp();
        }

        // This edge begins at neighbor_idx
        graph->neighbor_idx[edge_idx + 1] = neighbor_idx;

        debug_print(
          lb, node,
          "edge_id={:x} from={:x}, to={:x}\n",
          iter->first.edge_id_,
          iter->first.fromObjTemp(), iter->first.toObjTemp()
        );

        edge_idx++;
      }
      graph->num_all_neighbors = neighbor_idx;
    }

    debug_print(
      lb, node,
      "ZoltanLB: number of vertices={} edges={}, neighbors={}\n",
      graph->num_vertices, graph->num_edges, graph->num_all_neighbors
    );

  } else {
    graph->num_edges = 0;
    graph->num_all_neighbors = 0;
  }

  return graph;
}


/*static*/ int ZoltanLB::getNumberOfVertices(void *data, int *ierr) {
  Graph* graph = reinterpret_cast<Graph*>(data);
  *ierr = ZOLTAN_OK;
  return graph->num_vertices;
}

/*static*/ void ZoltanLB::getVertexList(
  void *data, int gid_size, int lid_size, ZOLTAN_ID_PTR global_id,
  ZOLTAN_ID_PTR local_id, int weight_dim, float *obj_weights, int *ierr
) {
  Graph* graph = reinterpret_cast<Graph*>(data);
  for (int i = 0; i < graph->num_vertices; i++) {
    global_id[i]   = graph->vertex_gid[i];
    local_id[i]    = i;
    obj_weights[i] = graph->vertex_weight[i];
  }
  *ierr = ZOLTAN_OK;
}

/*static*/ void ZoltanLB::getHypergraphSize(
  void *data, int *num_lists, int *num_nonzeroes, int *format, int *ierr
) {
  Graph* graph = reinterpret_cast<Graph*>(data);
  *num_lists     = graph->num_edges;
  *num_nonzeroes = graph->num_all_neighbors;
  *format        = ZOLTAN_COMPRESSED_EDGE;
  *ierr = ZOLTAN_OK;
}

/*static*/ void ZoltanLB::getHypergraph(
  void *data, int gid_size, int num_edges, int num_nonzeroes, int format,
  ZOLTAN_ID_PTR edge_gid, int *vertex_ptr, ZOLTAN_ID_PTR vertex_gid, int *ierr
) {
  Graph* graph = reinterpret_cast<Graph*>(data);
  bool const edge_equal           = num_edges     == graph->num_edges;
  bool const neighbor_equal       = num_nonzeroes == graph->num_all_neighbors;
  bool const is_compressed_format = format        == ZOLTAN_COMPRESSED_EDGE;

  vtAssert(edge_equal,           "Edge count must be equal");
  vtAssert(neighbor_equal,       "Neighbors count must be equal");
  vtAssert(is_compressed_format, "Must be compressed edge format");

  if (!edge_equal || !neighbor_equal || !is_compressed_format) {
    *ierr = ZOLTAN_FATAL;
    return;
  } else {
    *ierr = ZOLTAN_OK;
  }

  for (int i = 0; i < num_edges; i++) {
    edge_gid[i]   = graph->edge_gid[i];
    vertex_ptr[i] = graph->neighbor_idx[i];
  }

  for (int i = 0; i < num_nonzeroes; i++) {
    vertex_gid[i] = graph->neighbor_gid[i];
  }
}

/*static*/ void ZoltanLB::getHypergraphEdgeSize(
  void *data, int *num_edges, int *ierr
) {
  Graph* graph = reinterpret_cast<Graph*>(data);
  *num_edges = graph->num_edges;
  *ierr = ZOLTAN_OK;
}

/*static*/ void ZoltanLB::getHypergraphEdgeWeights(
  void *data, int num_gid, int num_lid, int num_edges, int edge_weight_dim,
  ZOLTAN_ID_PTR edge_gid, ZOLTAN_ID_PTR edge_lid, float *edge_weights, int *ierr
) {
  Graph* graph = reinterpret_cast<Graph*>(data);
  for (int i = 0; i < num_edges; i++) {
    edge_gid[i]     = graph->edge_gid[i];
    edge_lid[i]     = graph->edge_gid[i];
    edge_weights[i] = graph->edge_weight[i];
  }
  *ierr = ZOLTAN_OK;
}

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*backend_check_enabled(zoltan)*/

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_CC*/
