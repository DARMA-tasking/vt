/*
//@HEADER
// *****************************************************************************
//
//                                 zoltanlb.cc
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_CC
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_CC

#include "vt/config.h"
#include "vt/vrt/collection/balance/zoltanlb/zoltanlb.h"
#include "vt/vrt/collection/balance/model/load_model.h"
#include "vt/collective/collective_alg.h"
#include "vt/elm/elm_comm.h"

#if vt_check_enabled(zoltan)

#include <zoltan.h>

namespace vt { namespace vrt { namespace collection { namespace lb {

ZoltanLB::ZoltanLB()
  : collective_scope_(theCollective()->makeCollectiveScope())
{ }

void ZoltanLB::init(objgroup::proxy::Proxy<ZoltanLB> in_proxy) {
  proxy = in_proxy;
}

/*static*/ std::unordered_map<std::string, std::string>
ZoltanLB::getInputKeysWithHelp() {
  std::string const read_manual = "Please read Zoltan manual.";
  std::unordered_map<std::string, std::string> const keys_help = {
    {"LB_APPROACH",                   read_manual },
    {"DEBUG_LEVEL",                   read_manual },
    {"FINAL_OUTPUT",                  read_manual },
    {"DEBUG_PROCESSOR",               read_manual },
    {"CHECK_HYPERGRAPH",              read_manual },
    {"PHG_EDGE_WEIGHT_OPERATION",     read_manual },
    {"IMBALANCE_TOL",                 read_manual },
    {"PHG_REPART_MULTIPLIER",         read_manual },
    {"PHG_EDGE_WEIGHT_OPERATION",     read_manual },
    {"PHG_CUT_OBJECTIVE",             read_manual },
    {"PHG_OUTPUT_LEVEL",              read_manual },
    {"PHG_COARSENING_METHOD",         read_manual },
    {"PHG_COARSEPARTITION_METHOD",    read_manual },
    {"PHG_REFINEMENT_QUALITY",        read_manual },
    {"PHG_RANDOMIZE_INPUT",           read_manual },
    {"PHG_EDGE_SIZE_THRESHOLD",       read_manual },
    {"PHG_PROCESSOR_REDUCTION_LIMIT", read_manual },
    {"PHG_REFINEMENT_METHOD",         read_manual },
    {"PHG_MULTILEVEL",                read_manual },
    {"SEED",                          read_manual }
  };
  return keys_help;
}

void ZoltanLB::inputParams(balance::ConfigEntry* config) {
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

  std::vector<std::string> allowed{"edges"};
  for (auto& c : zoltan_config_) {
    allowed.push_back(c.first);
  }
  config->checkAllowedKeys(allowed);

  do_edges_ = config->getOrDefault<bool>("edges", do_edges_);

  for (auto& c : zoltan_config_) {
    c.second = config->getOrDefault<std::string>(c.first, c.second);
  }
}

void ZoltanLB::runLB(TimeType total_load) {
  auto const& this_node = theContext()->getNode();
  this_load = loadMilli(total_load);

  if (this_node == 0) {
    vt_debug_print(terse, lb, "ZoltanLB: runLB: edges={}\n", do_edges_);
    fflush(stdout);
  }

  if (do_edges_) {
    runInEpochCollective("ZoltanLB::runLB -> makeGraphSymmetric", [this]{ makeGraphSymmetric(); });
    runInEpochCollective("ZoltanLB::runLB -> combineEdges", [this]{ combineEdges(); });
    runInEpochCollective("ZoltanLB::runLB -> countEdges", [this]{ countEdges(); });
    runInEpochCollective("ZoltanLB::runLB -> allocateShareEdgeGIDs", [this]{ allocateShareEdgeGIDs(); });
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

  vt_debug_print(
    terse, lb,
    "ZoltanLB: num_export={}, num_import={}\n",
    num_export, num_import
  );

  std::set<ObjIDType> load_objs;
  for (auto obj : *load_model_) {
    if (obj.isMigratable()) {
      load_objs.insert(obj);
    }
  }

  for (int i = 0; i < num_export; i++) {
    int to_node = export_procs[i];

    vt_debug_print(
      normal, lb,
      "migrateObjTo: to_node={} gid={:x}\n",
      to_node, export_global_ids[i]
    );

    auto const obj_id = export_global_ids[i];
    auto iter = load_objs.find(
      ObjIDType{obj_id, uninitialized_destination}
    );
    vtAssert(iter != load_objs.end(), "Object must exist here");

    migrateObjectTo(*iter, static_cast<NodeType>(to_node));
  }

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
      elm.first.cat_ == elm::CommCategory::SendRecv and
      not elm.first.selfEdge()
    ) {
      auto from = elm.first.fromObj();
      auto to = elm.first.toObj();

      auto from_node = from.curr_node;
      auto to_node = to.curr_node;

      vtAssert(
        from_node == this_node or to_node == this_node,
        "One node must involve this node"
      );

      vt_debug_print(
        verbose, lb,
        "makeGraphSymmetric: from={}, to={}\n",
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
    proxy[elm.first].send<CommMsg, &ZoltanLB::recvSharedEdges>(elm.second);
  }

}

void ZoltanLB::recvSharedEdges(CommMsg* msg) {
  auto& comm = msg->comm_;
  for (auto&& elm : comm) {
    vt_debug_print(
      verbose, lb,
      "recv shared edge: from={}, to={}\n",
      elm.first.fromObj(), elm.first.toObj()
    );

    load_comm_symm[elm.first] += elm.second;
  }
}

void ZoltanLB::combineEdges() {
  // Combine bi-directional, symmetric edges into a non-directional weight in
  // bytes the sum of receive and send bytes
  ElementCommType load_comm_combined;

  for (auto&& e1 : load_comm_symm) {
    auto from = std::max(e1.first.fromObj(), e1.first.toObj());
    auto to = std::min(e1.first.fromObj(), e1.first.toObj());

    auto key = elm::CommKey{
      elm::CommKey::CollectionTag{}, from, to, false
    };
    load_comm_combined[key] += e1.second;
  }

  load_comm_symm = std::move(load_comm_combined);
}

void ZoltanLB::countEdges() {
  vt_debug_print(
    normal, lb,
    "countEdges\n"
  );

  // Count the number of local and remote edges to allocation edge GIDs
  int local_edge = 0;
  int remote_owned_edge = 0;

  auto const this_node = theContext()->getNode();
  for (auto&& elm : load_comm_symm) {
    if (
      elm.first.cat_ == elm::CommCategory::SendRecv and
      not elm.first.selfEdge()
    ) {
      auto from = elm.first.fromObj();
      auto to = elm.first.toObj();

      auto from_node = from.curr_node;
      auto to_node = to.curr_node;

      if (from_node == to_node and from_node == this_node) {
        local_edge++;
      } else {
        // Break ties on non-local object edges based on obj ID
        auto large_obj_id = std::max(from, to);
        if (large_obj_id.curr_node == this_node) {
          remote_owned_edge++;
        }
      }
    }
  }

  int const total_ids = local_edge + remote_owned_edge;

  vt_debug_print(
    verbose, lb,
    "ZoltanLB: total_ids_={}\n",
    total_ids
  );

  proxy.allreduce<&ZoltanLB::reduceCount, collective::MaxOp>(total_ids);
}

void ZoltanLB::reduceCount(int max_edges_per_node) {
  max_edges_per_node_ = max_edges_per_node;

  vt_debug_print(
    normal, lb,
    "ZoltanLB: max_edges_per_node_={}\n",
    max_edges_per_node_
  );
}

void ZoltanLB::allocateShareEdgeGIDs() {
  std::unordered_map<NodeType, ElementCommType> shared_edges;

  auto const this_node = theContext()->getNode();
  for (auto&& elm : load_comm_symm) {
    auto from = elm.first.fromObj();
    auto to = elm.first.toObj();

    auto from_node = from.curr_node;
    auto to_node = to.curr_node;

    if (from_node == to_node and from_node == this_node) {
      auto offset = max_edges_per_node_ * this_node;
      auto id = 1 + offset + edge_id_++;
      auto key = elm.first;
      key.edge_id_.id = id;
      load_comm_edge_id[key] = elm.second;

      vt_debug_print(
        verbose, lb,
        "allocate: local edge_id={:x}, from={:x}, to={:x}\n",
        key.edge_id_.id,
        key.fromObj(),
        key.toObj()
      );

    } else {
      auto large_obj_id = std::max(from, to);
      if (large_obj_id.curr_node == this_node) {
        auto offset = max_edges_per_node_ * this_node;
        auto id = 1 + offset + edge_id_++;
        auto key = elm.first;
        key.edge_id_.id = id;
        load_comm_edge_id[key] = elm.second;

        vt_debug_print(
          verbose, lb,
          "allocate: remote edge_id={:x}, from={:x}, to={:x}\n",
          key.edge_id_.id,
          key.fromObj(),
          key.toObj()
        );
      } else {
        // The other node will provide the edge
      }
    }
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
  // Insert local load objs into a std::set to get a deterministic order to
  // traverse them for building the graph consistently
  std::set<ObjIDType> load_objs;
  for (auto obj : *load_model_) {
    if (obj.isMigratable()) {
      load_objs.insert(obj);
    }
  }

  auto graph = std::make_unique<Graph>();

  // Number of local vertices (overdecomposed blocks) on this node
  graph->num_vertices = load_objs.size();

  // Allocate space for each vertex to describe it
  graph->vertex_gid = std::make_unique<ZOLTAN_ID_TYPE[]>(graph->num_vertices);

  // Allocate space for the weight of each vertex (calculated as a weight
  // representing time the overdecomposed block spent executing)
  graph->vertex_weight = std::make_unique<int[]>(graph->num_vertices);

  static_assert(
    sizeof(ZOLTAN_ID_TYPE) == sizeof(uint64_t),
    "ObjIDType must be exactly the same size as ZOLTAN_ID_TYPE\n"
    "Please recompile with \"-D Zoltan_ENABLE_ULLONG_IDS:Bool=ON\""
  );

  // Initialize all the local vertices with global id
  {
    int idx = 0;
    for (auto&& obj : load_objs) {
      graph->vertex_gid[idx++] = obj.id;

      vt_debug_print(
        verbose, lb,
        "makeVertexGraph: vertex_idx={}: obj={}\n",
        idx - 1, obj
      );
    }
  }

  // Set the weights for all vertices, by converting to milliseconds as the
  // weight
  {
    int idx = 0;
    for (auto&& obj : load_objs) {
      auto load = load_model_->getModeledLoad(
        obj,
        {balance::PhaseOffset::NEXT_PHASE, balance::PhaseOffset::WHOLE_PHASE}
      );
      auto time = static_cast<int>(loadMilli(load));
      graph->vertex_weight[idx++] = time;
    }
  }

  if (do_edges_) {
    // Only get communication edges between vertices/migratable elements
    // Insert local comm objs into a std::set for deterministic ordering
    std::vector<elm::CommKey> comm_objs;
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
      int edge_idx = 0;
      int neighbor_idx = 0;

      // Set offset for first edge
      graph->neighbor_idx[edge_idx] = neighbor_idx;

      for (auto&& elm : comm_objs) {
        auto iter = load_comm_edge_id.find(elm);
        auto comm = iter->second;

        vtAssert(
          iter->first.edge_id_.id != elm::no_element_id,
          "Must have element ID"
        );

        graph->edge_gid[edge_idx] = iter->first.edge_id_.id;
        graph->edge_weight[edge_idx] = comm.bytes;

        vt_debug_print(
          verbose, lb,
          "makeEdgeGraph: edge_id={}: edge_idx={}, neighbor_idx={}\n",
          iter->first.edge_id_,
          edge_idx,
          neighbor_idx
        );

        // Set up the links between communicating GIDs
        vt_debug_print(
          verbose, lb,
          "makeEdgeGraph: \t edge_id={}: edge_idx={}, obj={:x}\n",
          iter->first.edge_id_, edge_idx, iter->first.fromObj().id
        );

        graph->neighbor_gid[neighbor_idx++] = iter->first.fromObj().id;

        vt_debug_print(
          verbose, lb,
          "makeEdgeGraph: \t edge_id={}: edge_idx={}, obj={:x}\n",
          iter->first.edge_id_, edge_idx, iter->first.toObj().id
        );

        graph->neighbor_gid[neighbor_idx++] = iter->first.toObj().id;

        // This edge begins at neighbor_idx
        graph->neighbor_idx[edge_idx + 1] = neighbor_idx;

        vt_debug_print(
          verbose, lb,
          "edge_id={:x} from={:x}, to={:x}\n",
          iter->first.edge_id_,
          iter->first.fromObj(), iter->first.toObj()
        );

        edge_idx++;
      }
      graph->num_all_neighbors = neighbor_idx;
    }

    vt_debug_print(
      normal, lb,
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

#endif /*vt_check_enabled(zoltan)*/

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_CC*/
