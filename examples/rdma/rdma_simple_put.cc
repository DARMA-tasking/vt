/*
//@HEADER
// *****************************************************************************
//
//                              rdma_simple_put.cc
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

#include <vt/transport.h>

#include <memory>

struct HandleMsg : vt::Message {
  vt::RDMA_HandleType han;
  explicit HandleMsg(vt::RDMA_HandleType const& in_han) : han(in_han) { }
};

static std::unique_ptr<double[]> my_data = nullptr;

static void read_data_fn(HandleMsg* msg) {
  vt::NodeType this_node = vt::theContext()->getNode();

  fmt::print("{}: read_data_fn: handle={}\n", this_node, msg->han);

  if (this_node == 0) {
    int const len = 10;
    for (auto i = 0; i < len; i++) {
      fmt::print("\t: my_data[{}] = {}\n", i, my_data[i]);
    }
  }
}

static void put_data_fn(HandleMsg* msg) {
  vt::NodeType this_node = vt::theContext()->getNode();
  vt::RDMA_HandleType handle = msg->han;

  fmt::print("{}: put_data_fn: handle={}\n", this_node, handle);

  if (this_node < 4) {
    fmt::print("{}: putting data\n", this_node);

    int const local_data_len = 3;
    double* local_data = new double[local_data_len];
    for (auto i = 0; i < local_data_len; i++) {
      local_data[i] = (i+1)*1000*(this_node+1);
    }

    vt::theRDMA()->putData(
      handle, local_data, sizeof(double)*local_data_len,
      (this_node-1)*local_data_len, vt::no_tag, vt::rdma::rdma_default_byte_size,
      [=]{
        delete [] local_data;
        fmt::print("{}: after put: sending msg back to 0\n", this_node);
        auto msg2 = vt::makeMessage<HandleMsg>(this_node);
        msg2->han = handle;
        vt::theMsg()->sendMsg<HandleMsg,read_data_fn>(0, msg2);
      }
    );
  }
}

static void put_handler_fn(
  vt::BaseMessage*, vt::RDMA_PtrType in_ptr, vt::ByteType in_num_bytes,
  vt::ByteType offset, vt::TagType tag, bool
) {
  vt::NodeType this_node = vt::theContext()->getNode();

  fmt::print(
    "{}: put_handler_fn: my_data={}, in_ptr={}, in_num_bytes={}, tag={}, "
    "offset={}\n",
    this_node, print_ptr(my_data.get()), print_ptr(in_ptr), in_num_bytes, tag,
    offset
  );

  auto count = in_num_bytes/sizeof(double);
  for (decltype(count) i = 0; i < count; i++) {
    ::fmt::print(
      "{}: put_handler_fn: data[{}] = {}\n",
      this_node, i, static_cast<double*>(in_ptr)[i]
    );
  }

  std::memcpy(my_data.get() + offset, in_ptr, in_num_bytes);
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes != 4) {
    return vt::rerror("requires exactly 4 nodes");
  }

  if (this_node == 0) {
    auto const len = 64;
    my_data = std::make_unique<double[]>(len);

    for (auto i = 0; i < len; i++) {
      my_data[i] = i+1;
    }

    vt::RDMA_HandleType my_handle = vt::theRDMA()->registerNewRdmaHandler();
    vt::theRDMA()->associatePutFunction<vt::BaseMessage,put_handler_fn>(
      nullptr, my_handle, put_handler_fn, false
    );
    fmt::print("{}: initializing my_handle={}\n", this_node, my_handle);

    auto msg = vt::makeMessage<HandleMsg>(this_node);
    msg->han = my_handle;
    vt::theMsg()->broadcastMsg<HandleMsg,put_data_fn>(msg, false);
  }

  vt::finalize();

  return 0;
}
