/*
//@HEADER
// *****************************************************************************
//
//                              rdma_simple_get.cc
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

static void tell_handle(HandleMsg* msg) {
  vt::NodeType this_node = vt::theContext()->getNode();

  fmt::print("{}: handle={}\n", this_node, msg->han);

  if (this_node == 1 || this_node == 2) {
    fmt::print("{}: requesting data\n", this_node);
    vt::theRDMA()->getData(
      msg->han, this_node, sizeof(double)*3, vt::no_byte,
      [=](void* data, size_t num_bytes){
        double* const ptr = static_cast<double*>(data);
        size_t const num_elems = num_bytes / sizeof(double);
        fmt::print(
          "{}: data arrived: data={}, num_bytes={}\n",
          this_node, print_ptr(data), num_bytes
        );
        for (size_t i = 0; i < num_elems; i++) {
          fmt::print("\t: my_data[{}] = {}\n", i, ptr[i]);
        }
      }
    );
  }
}

static std::unique_ptr<double[]> my_data = nullptr;

static vt::RDMA_GetType test_get_fn(
  vt::BaseMessage*, vt::ByteType num_bytes, vt::ByteType offset, vt::TagType tag,
  bool
) {
  vt::NodeType this_node = vt::theContext()->getNode();
  fmt::print(
    "{}: running test_get_fn: num_bytes={}, tag={}\n",
    this_node, num_bytes, tag
  );
  return vt::RDMA_GetType{
    &my_data[0] + tag, num_bytes == vt::no_byte ? sizeof(double)*10 : num_bytes
  };
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();

  if (this_node == 0) {
    auto const len = 64;
    my_data = std::make_unique<double[]>(len);

    for (auto i = 0; i < len; i++) {
      my_data[i] = i+1;
    }

    vt::RDMA_HandleType my_handle = vt::theRDMA()->registerNewRdmaHandler();
    vt::theRDMA()->associateGetFunction<vt::BaseMessage,test_get_fn>(
      nullptr, my_handle, test_get_fn, true
    );

    fmt::print("initializing my_handle={}\n", my_handle);

    auto msg = vt::makeMessage<HandleMsg>(this_node);
    msg->han = my_handle;
    vt::theMsg()->broadcastMsg<HandleMsg, tell_handle>(msg);
  }

  vt::finalize();

  return 0;
}
