/*
//@HEADER
// *****************************************************************************
//
//                             rdma_channel_sync.cc
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

static RDMA_HandleType my_handle_1 = no_rdma_handle;

static int const put_len = 2;
#if backend_check_enabled(mpi_rdma)
static int const my_data_len = 8;
#endif
static double* my_data = nullptr;

static bool use_paired_sync = true;

struct TestMsg : vt::Message {
  RDMA_HandleType han;
  TestMsg(RDMA_HandleType const& in_han) : Message(), han(in_han) { }
};

static void put_channel_setup(TestMsg* msg);

static void read_data_fn(TestMsg* msg) {
  fmt::print("{}: read_data_fn: handle={}\n", my_node, msg->han);

  if (my_node == 0) {
    theRDMA()->syncLocalPutChannel(msg->han, 1, [=]{
      for (auto i = 0; i < put_len*2; i++) {
        fmt::print("{}: han={} \t: my_data[{}] = {}\n", my_node, msg->han, i, my_data[i]);
      }

      theRDMA()->newGetChannel(my_handle_1, 0, 2, [=]{
        TestMsg* msg1 = makeSharedMessage<TestMsg>(my_handle_1);
        theMsg()->sendMsg<TestMsg, put_channel_setup>(2, msg1);
      });
    });
  } else if (my_node == 2) {
    theRDMA()->syncLocalGetChannel(msg->han, [=]{
      for (auto i = 0; i < put_len*2; i++) {
        fmt::print("{}: han={} \t: my_data[{}] = {}\n", my_node, msg->han, i, my_data[i]);
      }
    });
  }
}

static void put_channel_setup(TestMsg* msg) {
  auto const& handle = msg->han;

  fmt::print("{}: put_channel_setup: handle={}\n", my_node, msg->han);

  if (my_node == 1) {
    theRDMA()->newPutChannel(handle, 0, 1, [=]{
      int const num_elm = 2;

      if (use_paired_sync) {
        theRDMA()->putTypedData(handle, my_data, num_elm, no_byte, [=]{
          TestMsg* back = makeSharedMessage<TestMsg>(handle);
          theMsg()->sendMsg<TestMsg, read_data_fn>(0, back);
        });
      } else {
        theRDMA()->putTypedData(handle, my_data, num_elm);
        theRDMA()->syncRemotePutChannel(handle, [=]{
          TestMsg* back = makeSharedMessage<TestMsg>(handle);
          theMsg()->sendMsg<TestMsg, read_data_fn>(0, back);
        });
      }
    });
  }
  else if (my_node == 2) {
    theRDMA()->newGetChannel(handle, 0, 2, [=]{
      fmt::print(
        "{}: creating get channel complete\n", my_node
      );
      int const num_elm = 2;

      if (use_paired_sync) {
        theRDMA()->getTypedDataInfoBuf(handle, my_data, num_elm, [=]{
          TestMsg* back = makeSharedMessage<TestMsg>(handle);
          theMsg()->sendMsg<TestMsg, read_data_fn>(2, back);
        });
      } else {
        theRDMA()->getTypedDataInfoBuf(handle, my_data, num_elm);
        // theRDMA()->get_typed_data_info_buf(handle, my_data+2, num_elm);
        theRDMA()->syncLocalGetChannel(handle, [=]{
          TestMsg* back = makeSharedMessage<TestMsg>(handle);
          theMsg()->sendMsg<TestMsg, read_data_fn>(2, back);
        });
      }
    });
  }
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

#if backend_check_enabled(mpi_rdma)
  my_data = new double[my_data_len];

  if (my_node < 3) {
    // initialize my_data buffer, all but node 0 get -1.0
    for (auto i = 0; i < 4; i++) {
      my_data[i] = my_node != 0 ? (my_node+1)*i+1 : -1.0;
      fmt::print("{}: \t: my_data[{}] = {}\n", my_node, i, my_data[i]);
    }
  }

  if (my_node == 0) {
    my_handle_1 = theRDMA()->registerNewTypedRdmaHandler(my_data, put_len);

    fmt::print(
      "{}: initializing my_handle_1={}\n", my_node, my_handle_1
    );

    theRDMA()->newPutChannel(my_handle_1, 0, 1, [=]{
      TestMsg* msg1 = makeSharedMessage<TestMsg>(my_handle_1);
      theMsg()->sendMsg<TestMsg, put_channel_setup>(1, msg1);
    });
  }
#endif

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
