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

#if backend_check_enabled(zoltan)

#include <zoltan.h>

namespace vt { namespace vrt { namespace collection { namespace lb {

void ZoltanLB::init(objgroup::proxy::Proxy<ZoltanLB> in_proxy) {
  proxy = in_proxy;
}

void ZoltanLB::inputParams(balance::SpecEntry* spec) { }

void ZoltanLB::runLB() {
  auto const& this_node = theContext()->getNode();
  // auto const& num_nodes = theContext()->getNumNodes();
  // auto const next_node = this_node + 1 > num_nodes-1 ? 0 : this_node + 1;

  if (this_node == 0) {
    vt_print(lb, "ZoltanLB: runLB\n");
    fflush(stdout);
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
  auto const partition_return = Zoltan_LB_Partition(
    zoltan_, &changes, &num_gid_entries, &num_lid_entries,
    &num_import,
    &import_global_ids, &import_local_ids, &import_procs, &import_to_part,
    &num_export,
    &export_global_ids, &export_local_ids, &export_procs, &export_to_part
  );
  assert(partition_return == ZOLTAN_OK);

  startMigrationCollective();

  // vt_print(
  //   lb,
  //   "ZoltanLB: num_export={}, num_import={}\n",
  //   num_export, num_import
  // );

  for (int i = 0; i < num_export; i++) {
    int to_node = export_procs[i];
    // vt_print(lb, "migrateObjTo: to_node={} gid={:x}\n", to_node, export_global_ids[i]);
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

Zoltan_Struct* ZoltanLB::initZoltan() {
  float ver = 0.0f;
  auto const ret = Zoltan_Initialize(0, nullptr, &ver);
  assert(ret == ZOLTAN_OK);

  zoltan_ = Zoltan_Create(theContext()->getComm());
  return zoltan_;
}

void ZoltanLB::destroyZoltan() {
  Zoltan_Destroy(&zoltan_);
}

void ZoltanLB::setParams() {
  auto gparts = fmt::format("{}", theContext()->getNumNodes());
  Zoltan_Set_Param(zoltan_, "DEBUG_LEVEL",               "0"           );
  Zoltan_Set_Param(zoltan_, "CHECK_HYPERGRAPH",          "1"           );
  Zoltan_Set_Param(zoltan_, "LB_METHOD",                 "HYPERGRAPH"  );
  Zoltan_Set_Param(zoltan_, "HYPERGRAPH_PACKAGE",        "PHG"         );
  Zoltan_Set_Param(zoltan_, "NUM_GID_ENTRIES",           "1"           );
  Zoltan_Set_Param(zoltan_, "NUM_LID_ENTRIES",           "1"           );
  Zoltan_Set_Param(zoltan_, "RETURN_LISTS",              "ALL"         );
  Zoltan_Set_Param(zoltan_, "OBJ_WEIGHT_DIM",            "1"           );
  Zoltan_Set_Param(zoltan_, "EDGE_WEIGHT_DIM",           "1"           );
  Zoltan_Set_Param(zoltan_, "NUM_GLOBAL_PARTS",          gparts.c_str());
  Zoltan_Set_Param(zoltan_, "LB_APPROACH",               "REPARTITION" );
  Zoltan_Set_Param(zoltan_, "PHG_EDGE_WEIGHT_OPERATION", "max"         );
  Zoltan_Set_Param(zoltan_, "IMBALANCE_TOL",             "1.00"        );
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
    "ObjIDType must fit in ZOLTAN_ID_TYPE\n"
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
    for (auto&& elm : *comm_data) {
      if (elm.first.cat_ == balance::CommCategory::SendRecv) {
        comm_objs.push_back(elm.first);
      }
    }

    // Set the number of communication edges
    graph->num_edges = static_cast<int>(comm_objs.size());

    // Allocate space for each edge to describe it
    graph->edge_gid = std::make_unique<ZOLTAN_ID_TYPE[]>(graph->num_edges);

    // Allocate space for each edge weight
    graph->edge_weight = std::make_unique<int[]>(graph->num_edges);

    // Allocate space for indexing array into neighbor_gid array for each edge
    graph->neighbor_idx = std::make_unique<int[]>(graph->num_edges);

    // Allocate space indexing array for edge edge_gid[i] beginning at
    // neighbor_gid[neighbor_idx[i]]
    graph->neighbor_gid = std::make_unique<ZOLTAN_ID_TYPE[]>(graph->num_edges * 2);

    // Set the edge weights as bytes in the communication graph
    {
      int edge_idx = 0;
      int neighbor_idx = 0;
      for (auto&& elm : comm_objs) {
        auto iter = comm_data->find(elm);
        auto bytes = iter->second;
        graph->edge_weight[edge_idx] = bytes;

        // This edge begins at neighbor_idx
        graph->neighbor_idx[edge_idx] = neighbor_idx;

        // Set up the links between communicating GIDs
        graph->neighbor_gid[neighbor_idx++] = iter->first.fromObjTemp();
        graph->neighbor_gid[neighbor_idx++] = iter->first.toObjTemp();

        edge_idx++;
      }
    }
  } else {
    graph->num_edges = 0;
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

  assert(edge_equal           && "Edge count must be equal");
  assert(neighbor_equal       && "Neighbors count must be equal");
  assert(is_compressed_format && "Must be compressed edge format");

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
