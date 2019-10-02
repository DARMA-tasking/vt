/*
//@HEADER
// *****************************************************************************
//
//                              rdma_simple_get.cc
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

static void tell_handle(TestMsg* msg) {
  fmt::print("{}: handle={}\n", my_node, msg->han);

  if (my_node == 1 || my_node == 2) {
    fmt::print("{}: requesting data\n", my_node);
    theRDMA()->getData(
      msg->han, my_node, sizeof(double)*3, no_byte,
      [](void* data, size_t num_bytes){
        double* const ptr = static_cast<double*>(data);
        size_t const num_elems = num_bytes / sizeof(double);
        fmt::print(
          "{}: data arrived: data={}, num_bytes={}\n",
          my_node, print_ptr(data), num_bytes
        );
        for (size_t i = 0; i < num_elems; i++) {
          fmt::print("\t: my_data[{}] = {}\n", i, ptr[i]);
        }
      }
    );
  }
}

static double* my_data = nullptr;
static TestMsg* test_msg = nullptr;

static RDMA_GetType
test_get_fn(TestMsg* msg, ByteType num_bytes, ByteType offset, TagType tag, bool) {
  fmt::print(
    "{}: running test_get_fn: msg={}, num_bytes={}, tag={}\n",
    my_node, print_ptr(msg), num_bytes, tag
  );
  return RDMA_GetType{
    my_data+tag, num_bytes == no_byte ? sizeof(double)*10 : num_bytes
  };
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (my_node == 0) {
    auto const len = 64;
    my_data = new double[len];

    for (auto i = 0; i < len; i++) {
      my_data[i] = i+1;
    }

    my_handle = theRDMA()->registerNewRdmaHandler();
    theRDMA()->associateGetFunction<TestMsg,test_get_fn>(
      test_msg, my_handle, test_get_fn, true
    );

    fmt::print("initializing my_handle={}\n", my_handle);

    auto msg = makeSharedMessage<TestMsg>(my_node);
    msg->han = my_handle;
    theMsg()->broadcastMsg<TestMsg, tell_handle>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
