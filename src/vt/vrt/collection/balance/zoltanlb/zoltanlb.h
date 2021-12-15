/*
//@HEADER
// *****************************************************************************
//
//                                  zoltanlb.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_H
#define INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_H

#include "vt/config.h"
#include "vt/vrt/collection/balance/lb_common.h"
#include "vt/vrt/collection/balance/baselb/baselb.h"
#include "vt/collective/collective_scope.h"

#if vt_check_enabled(zoltan)

#include <zoltan.h>

#include <memory>
#include <unordered_map>

namespace vt { namespace vrt { namespace collection { namespace lb {

struct ZoltanLB : BaseLB {
  using ReduceMsg = collective::ReduceTMsg<int>;

  ZoltanLB();

  void init(objgroup::proxy::Proxy<ZoltanLB> in_proxy);
  void runLB(TimeType total_load) override;
  void inputParams(balance::SpecEntry* spec) override;

  static std::unordered_map<std::string, std::string> getInputKeysWithHelp();

private:
  struct Graph {
    using GID = std::unique_ptr<ZOLTAN_ID_TYPE[]>;
    using LID = std::unique_ptr<int[]>;

    Graph() = default;

    int num_vertices = 0;        /* number of vertices locally owned */
    int num_edges = 0;           /* number of my hyperedges */
    int num_all_neighbors = 0;   /* number of vertices in local hyperedges */

    GID vertex_gid = nullptr;    /* global ID of these vertices */
    GID edge_gid = nullptr;      /* global ID of each of local hyperedges */
    GID neighbor_gid = nullptr;  /* Vertices of edge edge_gid[i] begin at
                                    neighbor_gid[neighbor_idx[i]] */

    LID vertex_weight = nullptr; /* weight of each vertex */
    LID edge_weight = nullptr;   /* weight of each edge */
    LID neighbor_idx = nullptr;  /* compressed hyperedge format:
                                    neighbor_gid idx of edge's vertices */
  };

  Zoltan_Struct* initZoltan();
  void destroyZoltan();
  void setParams();
  std::unique_ptr<Graph> makeGraph();
  void makeGraphSymmetric();
  void combineEdges();
  void countEdges();
  void allocateEdges();
  void reduceCount(ReduceMsg* msg);
  void allocateShareEdgeGIDs();

  struct CommMsg : vt::Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_required(); // comm_

    CommMsg() = default;
    explicit CommMsg(ElementCommType in_comm)
      : comm_(in_comm)
    { }

    ElementCommType comm_;

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      MessageParentType::serialize(s);
      s | comm_;
    }
  };

  void recvSharedEdges(CommMsg* msg);

private:
  static int getNumberOfVertices(void *data, int *ierr);
  static void getVertexList(
    void *data, int gid_size, int lid_size, ZOLTAN_ID_PTR global_id,
    ZOLTAN_ID_PTR local_id, int weight_dim, float *obj_weights, int *ierr
  );
  static void getHypergraphSize(
    void *data, int *num_lists, int *num_nonzeroes, int *format, int *ierr
  );
  static void getHypergraph(
    void *data, int gid_size, int num_edges, int num_nonzeroes, int format,
    ZOLTAN_ID_PTR edge_gid, int *vertex_ptr, ZOLTAN_ID_PTR vertex_gid, int *ierr
  );
  static void getHypergraphEdgeSize(void *data, int *num_edges, int *ierr);
  static void getHypergraphEdgeWeights(
    void *data, int num_gid, int num_lid, int num_edges, int edge_weight_dim,
    ZOLTAN_ID_PTR edge_gid, ZOLTAN_ID_PTR edge_lid, float *edge_weights,
    int *ierr
  );

private:
  objgroup::proxy::Proxy<ZoltanLB> proxy = {};
  Zoltan_Struct* zoltan_ = nullptr;
  bool do_edges_ = false;
  std::unordered_map<std::string, std::string> zoltan_config_;
  ElementCommType load_comm_symm;
  ElementCommType load_comm_edge_id;
  int max_edges_per_node_ = 0;
  balance::ElementIDType edge_id_ = 0;
  collective::CollectiveScope collective_scope_;
  LoadType this_load = 0.0f;
};

}}}} /* end namespace vt::vrt::collection::lb */

#endif /*vt_check_enabled(zoltan)*/

#endif /*INCLUDED_VT_VRT_COLLECTION_BALANCE_ZOLTANLB_ZOLTANLB_H*/
