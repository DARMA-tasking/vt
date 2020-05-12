/*
//@HEADER
// *****************************************************************************
//
//                          rdma_simple_put_direct.cc
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

#include <vt/transport.h>

#include <memory>

static int const put_len = 2;
static int const my_data_len = 8;
static std::unique_ptr<double[]> my_data = nullptr;

struct HandleMsg : vt::Message {
  vt::RDMA_HandleType han;
  explicit HandleMsg(vt::RDMA_HandleType const& in_han) : han(in_han) { }
};

static void readDataFn(HandleMsg* msg) {
  vt::NodeType this_node = vt::theContext()->getNode();

  fmt::print("{}: readDataFn: handle={}\n", this_node, msg->han);

  for (auto i = 0; i < put_len*2; i++) {
    fmt::print(
      "{}: han={} \t: my_data[{}] = {}\n", this_node, msg->han, i, my_data[i]
    );
  }
}

static void putDataFn(HandleMsg* msg) {
  vt::NodeType this_node = vt::theContext()->getNode();

  if (this_node == 1 or this_node == 2) {
    fmt::print(
      "{}: putting data, handle={}, my_data={}\n",
      this_node, msg->han, print_ptr(&my_data[0])
    );

    int const num_elm = 2;
    int const offset = num_elm*(this_node-1);
    auto han = msg->han;
    vt::theRDMA()->putTypedData(msg->han, &my_data[0], num_elm, offset, [=]{
      fmt::print(
        "{}: after put: sending msg back to 0: offset={}\n", this_node, offset
      );

      auto back = vt::makeMessage<HandleMsg>(han);
      vt::theMsg()->sendMsg<HandleMsg, readDataFn>(0, back.get());
    });
  }
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes < 4) {
    return vt::rerror("requires exactly 4 nodes");
  }

  my_data = std::make_unique<double[]>(my_data_len);

  // initialize my_data buffer, all but node 0 get -1.0
  for (auto i = 0; i < my_data_len; i++) {
    my_data[i] = this_node != 0 ? (this_node+1)*i+1 : -1.0;
  }

  if (this_node == 0) {
    vt::RDMA_HandleType my_handle =
      vt::theRDMA()->registerNewTypedRdmaHandler(&my_data[0], put_len);

    fmt::print(
      "{}: initializing my_handle={}\n",
      this_node, my_handle
    );

    auto msg1 = vt::makeMessage<HandleMsg>(my_handle);
    auto msg2 = vt::makeMessage<HandleMsg>(my_handle);
    vt::theMsg()->sendMsg<HandleMsg, putDataFn>(1, msg1.get());
    vt::theMsg()->sendMsg<HandleMsg, putDataFn>(2, msg2.get());
  }

  vt::finalize();

  return 0;
}
