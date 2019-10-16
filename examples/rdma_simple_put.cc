/*
//@HEADER
// *****************************************************************************
//
//                              rdma_simple_put.cc
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

struct TestMsg : vt::Message {
  RDMA_HandleType han;
  TestMsg(RDMA_HandleType const& in_han) : Message(), han(in_han) { }
};

static double* my_data = nullptr;

static void read_data_fn(TestMsg* msg) {
  fmt::print("{}: read_data_fn: handle={}\n", my_node, msg->han);

  if (my_node == 0) {
    int const len = 10;
    for (auto i = 0; i < len; i++) {
      fmt::print("\t: my_data[{}] = {}\n", i, my_data[i]);
    }
  }
}

static void put_data_fn(TestMsg* msg) {
  fmt::print("{}: put_data_fn: handle={}\n", my_node, msg->han);

  if (my_node < 4) {
    fmt::print("{}: putting data\n", my_node);

    int const local_data_len = 3;
    double* local_data = new double[local_data_len];
    for (auto i = 0; i < local_data_len; i++) {
      local_data[i] = (i+1)*1000*(my_node+1);
    }
    theRDMA()->putData(
      msg->han, local_data, sizeof(double)*local_data_len,
      (my_node-1)*local_data_len, no_tag, vt::rdma::rdma_default_byte_size,
      [=]{
        delete [] local_data;
        fmt::print("{}: after put: sending msg back to 0\n", my_node);
        auto msg2 = makeSharedMessage<TestMsg>(my_node);
        msg2->han = my_handle;
        theMsg()->sendMsg<TestMsg,read_data_fn>(0, msg2);
      }
    );
  }
}

static void put_handler_fn(
  BaseMessage* msg, RDMA_PtrType in_ptr, ByteType in_num_bytes, ByteType offset,
  TagType tag, bool
) {
  fmt::print(
    "{}: put_handler_fn: my_data={}, in_ptr={}, in_num_bytes={}, tag={}, "
    "offset={}\n",
    my_node, print_ptr(my_data), print_ptr(in_ptr), in_num_bytes, tag, offset
  );

  auto count = in_num_bytes/sizeof(double);
  for (decltype(count) i = 0; i < count; i++) {
    ::fmt::print(
      "{}: put_handler_fn: data[{}] = {}\n",
      my_node, i, static_cast<double*>(in_ptr)[i]
    );
  }

  std::memcpy(my_data + offset, in_ptr, in_num_bytes);
}


int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes != 4) {
    fmt::print("requires exactly 4 nodes\n");
    CollectiveOps::finalize();
    return 0;
  }

  if (my_node == 0) {
    auto const len = 64;
    my_data = new double[len];

    for (auto i = 0; i < len; i++) {
      my_data[i] = i+1;
    }

    //my_handle = theRDMA()->register_new_typed_rdma_handler(my_data, 10);
    my_handle = theRDMA()->registerNewRdmaHandler();
    theRDMA()->associatePutFunction<BaseMessage,put_handler_fn>(
      nullptr, my_handle, put_handler_fn, false
    );
    fmt::print("{}: initializing my_handle={}\n", my_node, my_handle);

    auto msg = makeSharedMessage<TestMsg>(my_node);
    msg->han = my_handle;
    theMsg()->broadcastMsg<TestMsg,put_data_fn>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
