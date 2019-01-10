/*
//@HEADER
// ************************************************************************
//
//                          rdma_simple_put_direct.cc
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

#include "vt/transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static RDMA_HandleType my_handle_1 = no_rdma_handle;

static int const put_len = 2;
static int const my_data_len = 8;
static double* my_data = nullptr;

struct TestMsg : vt::Message {
  RDMA_HandleType han;
  TestMsg(RDMA_HandleType const& in_han) : Message(), han(in_han) { }
};

static void readDataFn(TestMsg* msg) {
  fmt::print("{}: readDataFn: handle={}\n", my_node, msg->han);

  for (auto i = 0; i < put_len*2; i++) {
    fmt::print(
      "{}: han={} \t: my_data[{}] = {}\n", my_node, msg->han, i, my_data[i]
    );
  }
}

static void putDataFn(TestMsg* msg) {
  if (my_node == 1 or my_node == 2) {
    fmt::print(
      "{}: putting data, handle={}, my_data={}\n",
      my_node, msg->han, print_ptr(my_data)
    );

    int const num_elm = 2;
    int const offset = num_elm*(my_node-1);
    theRDMA()->putTypedData(msg->han, my_data, num_elm, offset, no_action, [=]{
      fmt::print(
        "{}: after put: sending msg back to 0: offset={}\n", my_node, offset
      );

      TestMsg* back = makeSharedMessage<TestMsg>(msg->han);
      theMsg()->sendMsg<TestMsg, readDataFn>(0, back);
    });
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes < 4) {
    CollectiveOps::output("requires exactly 4 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  my_data = new double[my_data_len];

  // initialize my_data buffer, all but node 0 get -1.0
  for (auto i = 0; i < my_data_len; i++) {
    my_data[i] = my_node != 0 ? (my_node+1)*i+1 : -1.0;
  }

  if (my_node == 0) {
    my_handle_1 = theRDMA()->registerNewTypedRdmaHandler(my_data, put_len);

    fmt::print(
      "{}: initializing my_handle_1={}\n",
      my_node, my_handle_1
    );

    TestMsg* msg1 = makeSharedMessage<TestMsg>(my_handle_1);
    TestMsg* msg2 = makeSharedMessage<TestMsg>(my_handle_1);
    theMsg()->sendMsg<TestMsg, putDataFn>(1, msg1);
    theMsg()->sendMsg<TestMsg, putDataFn>(2, msg2);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
