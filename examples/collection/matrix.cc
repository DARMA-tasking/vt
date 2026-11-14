/*
//@HEADER
// *****************************************************************************
//
//                                  matrix.cc
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
#include <vt/vrt/collection/balance/model/per_collection.h>

#include <cstdlib>

int32_t N = 10;
double slot_percent = 0.2;
double exterior_percent = 0.7;
double slot_time = 1.0;
double slot_surface_time = 0.7;
double exterior_time = 0.5;
double interior_time = 0.3;
int32_t subblocks_per_rank = 2;

vt::Index2D global_range = {};

using vt::vrt::collection::balance::PerCollection;
using vt::vrt::collection::balance::LoadModel;

enum struct MatrixRegionEnum {
  SLOT_SLOT = 0,
  SLOT_SURFACE = 1,
  SURFACE_SLOT = 2,
  EXTERIOR_SURFACE = 3,
  INTERIOR_SURFACE = 4,
  ZEROS = 5
};

double getWork(MatrixRegionEnum region) {
  switch (region) {
  case MatrixRegionEnum::SLOT_SLOT:
    return slot_time;
  case MatrixRegionEnum::SLOT_SURFACE:
  case MatrixRegionEnum::SURFACE_SLOT:
    return slot_surface_time;
  case MatrixRegionEnum::EXTERIOR_SURFACE:
    return exterior_time;
  case MatrixRegionEnum::INTERIOR_SURFACE:
    return interior_time;
  case MatrixRegionEnum::ZEROS:
    return 0;
  }
  return 0;
}

MatrixRegionEnum getRegion(vt::Index2D const idx) {
  int const x = idx.x();
  int const y = idx.y();
  MatrixRegionEnum region = MatrixRegionEnum::ZEROS;
  if (x < N * slot_percent and y < N * slot_percent) {
    ///fmt::print("slot-slot {}\n", my_index);
    region = MatrixRegionEnum::SLOT_SLOT;
  } else if (x < N * slot_percent) {
    //fmt::print("slot-surface {}\n", my_index);
    region = MatrixRegionEnum::SLOT_SURFACE;
  } else if (y < N * slot_percent) {
    //fmt::print("surface-slot {}\n", my_index);
    region = MatrixRegionEnum::SURFACE_SLOT;
  } else if (x < N * slot_percent + N * exterior_percent and
             y < N * slot_percent + N * exterior_percent) {
    //fmt::print("exterior surface {}\n", my_index);
    region = MatrixRegionEnum::EXTERIOR_SURFACE;
  } else if (x >= N * slot_percent + N * exterior_percent and
             y >= N * slot_percent + N * exterior_percent) {
    //fmt::print("interior surface {}\n", my_index);
    region = MatrixRegionEnum::INTERIOR_SURFACE;
  } else {
    //fmt::print("zeros {}\n", my_index);
    region = MatrixRegionEnum::ZEROS;
  }
  return region;
}

vt::NodeType myMap(vt::Index2D* idx, vt::Index2D* range, vt::NodeType num_nodes) {
  vtAbortIf(not (sqrt(num_nodes) * sqrt(num_nodes) == num_nodes), "Must be square number of procs");
  int const np = sqrt(num_nodes);
  vtAbortIf(not (range->x() / np) * np == range->x(), "Must be divisible");
  vtAbortIf(not (range->y() / np) * np == range->y(), "Must be divisible");
  int const proc = (idx->x() / (range->x() / np)) * np + idx->y() / (range->y() / np);
  return proc;
}

int32_t getSubblock(vt::Index2D* idx, vt::Index2D* range, vt::NodeType num_nodes) {
  int const np = sqrt(num_nodes);
  int const subblock =
    (idx->x() / (range->x() / np)) * np * subblocks_per_rank +
    idx->y() / (range->y() / np / subblocks_per_rank);
  return subblock;
}

struct MatrixBlock : vt::Collection<MatrixBlock, vt::Index2D> {

  void setTimeForLB() {
    std::vector<vt::LoadType> vec;
    getLBData().setTime(getWork(region), vec);
  }

  void setUserDefined() {
    // auto const this_node = vt::theContext()->getNode();
    auto const num_nodes = vt::theContext()->getNumNodes();
    auto ptr = static_cast<vt::vrt::collection::storage::Storable*>(this);
    auto idx = getIndex();
    int subblock = getSubblock(&idx, &global_range, num_nodes);
    int proc = myMap(&idx, &global_range, num_nodes);

    ptr->valInsert("shared_id", static_cast<double>(subblock) , true, true, false);
    ptr->valInsert("shared_bytes", 1.0, true, true, false);
    ptr->valInsert("task_serialized_bytes", 0.0, true, true, false);
    ptr->valInsert("task_footprint_bytes", 0.0, true, true, false);
    ptr->valInsert("task_working_bytes", 0.0, true, true, false);
    ptr->valInsert("rank_working_bytes", 0.0, true, true, false);
    ptr->valInsert("home_rank", static_cast<double>(proc), true, true, false);
  }
  
  void setRegion() {
    auto const my_index = getIndex();
    region = getRegion(my_index);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::Collection<MatrixBlock, vt::Index2D>::serialize(s);
    s | region;
  }

private:
  MatrixRegionEnum region;
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  if (argc != 9) {
    vtAbort(
      "Expecting 8 arguments: "
      "<N> <slot-factor> <exterior-factor> "
      "<slot-slot time> <slot-surface/surface-slot time> "
      "<exterior time> <interior time> <subblocks-per-rank>"
    );
  } else {
    N = atoi(argv[1]);
    slot_percent = atof(argv[2]);
    exterior_percent = atof(argv[3]);
    slot_time = atof(argv[4]);
    slot_surface_time = atof(argv[5]);
    exterior_time = atof(argv[6]);
    interior_time = atof(argv[7]);
    subblocks_per_rank = atoi(argv[8]);
  }

  auto const num_nodes = vt::theContext()->getNumNodes();
  auto const this_node = vt::theContext()->getNode();

  if (this_node == 0) {
    fmt::print(
      "MatrixBlock benchmark:\n"
      "\tN={}\n"
      "\tslot_percent={}\n"
      "\texterior_percent={}\n"
      "\tslot_time={}\n"
      "\tslot_surface_time={}\n"
      "\texterior_time={}\n"
      "\tinterior_time={}\n"
      "\tsubblocks_per_rank={}\n",
      N,
      slot_percent,
      exterior_percent,
      slot_time,
      slot_surface_time,
      exterior_time,
      interior_time,
      subblocks_per_rank
    );
  }

  auto range = vt::Index2D(N, N);
  global_range = range;

  std::vector<vt::Index2D> list;

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      vt::Index2D idx(i, j);
      auto const proc = myMap(&idx, &range, num_nodes);
      // auto const subblock = getSubblock(&idx, &range, num_nodes);
      if (this_node == 0) {
        fmt::print("{:>2} ", proc);
      }
      if (getRegion(idx) != MatrixRegionEnum::ZEROS) {
        list.emplace_back(idx);
      }
    }
    if (this_node == 0) {
      fmt::print("\n");
    }
  }

  for (int i = 0; i < N; i++) {
    for (int j = 0; j < N; j++) {
      vt::Index2D idx(i, j);
      auto const subblock = getSubblock(&idx, &range, num_nodes);
      if (this_node == 0) {
        fmt::print("{:>2} ", subblock);
      }
    }
    if (this_node == 0) {
      fmt::print("\n");
    }
  }

  auto matrix = vt::makeCollection<MatrixBlock>("matrix block")
    .collective(true)
    .bounds(range)
    .listInsert(list)
    .mapperFunc<myMap>()
    .wait();
  
  if (this_node == 0) {
    vt_print(gen, "Starting setRegion and set time\n");
  }

  matrix.broadcastCollective<&MatrixBlock::setRegion>();
  matrix.broadcastCollective<&MatrixBlock::setTimeForLB>();
  matrix.broadcastCollective<&MatrixBlock::setUserDefined>();

  vt::thePhase()->nextPhaseCollective();

  matrix.broadcastCollective<&MatrixBlock::setTimeForLB>();

  vt::thePhase()->nextPhaseCollective();

  vt::finalize();
  return 0;
}
