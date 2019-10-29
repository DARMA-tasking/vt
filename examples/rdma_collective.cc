/*
//@HEADER
// *****************************************************************************
//
//                              rdma_collective.cc
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

#include "vt/transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static RDMA_HandleType my_handle = no_rdma_handle;

#if backend_check_enabled(mpi_rdma)
static int const my_data_len = 8;
static double* my_data = nullptr;
#endif
static int const local_data_len = 24;
static double* local_data = nullptr;

struct TestMsg : vt::Message {
  RDMA_HandleType han;

  TestMsg(RDMA_HandleType const& in_han) : Message(), han(in_han) { }
};

#pragma GCC diagnostic ignored "-Wunused-function"
static void announce(TestMsg* msg) {
  auto const& rdma_handle = msg->han;

  fmt::print("{}: handle={}, requesting data\n", my_node, rdma_handle);

  if (my_node == 1) {
    theRDMA()->newGetChannel(my_handle, 2, 1, [=]{
      fmt::print("set up channel with 2\n");

      theRDMA()->getTypedDataInfoBuf(rdma_handle, local_data, local_data_len, 5, no_tag, [=]{
        fmt::print("{}: handle={}, finished getting data\n", my_node, rdma_handle);
        for (int i = 0; i < local_data_len; i++) {
          fmt::print("{}: \t local_data[{}] = {}\n", my_node, i, local_data[i]);
          vtAssertExpr(local_data[i] == 5.0+i);
        }
      });
    });
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes < 4) {
    fmt::print("requires exactly 4 nodes\n");
    CollectiveOps::finalize();
    return 0;
  }

#if backend_check_enabled(mpi_rdma)
  my_data = new double[my_data_len];
  local_data = new double[local_data_len];

  // initialize my_data buffer, all but node 0 get -1.0
  for (auto i = 0; i < my_data_len; i++) {
    my_data[i] = (my_node)*my_data_len + i;
  }

  for (auto i = 0; i < local_data_len; i++) {
    local_data[i] = 0.0;
  }

  my_handle = theRDMA()->registerCollectiveTyped(
    my_data, my_data_len, my_data_len*num_nodes
  );

  theCollective()->barrier();

  fmt::print("{}: handle={}, create handle\n", my_node, my_handle);

  if (my_node == 0) {
    theRDMA()->newGetChannel(my_handle, 0, 1, [=]{
      TestMsg* msg = makeSharedMessage<TestMsg>(my_node);
      msg->han = my_handle;
      theMsg()->broadcastMsg<TestMsg, announce>(msg);
    });
  }
#endif

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
