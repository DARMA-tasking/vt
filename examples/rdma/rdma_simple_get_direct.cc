/*
//@HEADER
// *****************************************************************************
//
//                          rdma_simple_get_direct.cc
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

static int const my_data_len = 8;
static std::unique_ptr<double[]> my_data = nullptr;

struct HandleMsg : vt::Message {
  vt::RDMA_HandleType han;
  explicit HandleMsg(vt::RDMA_HandleType const& in_han) : han(in_han) { }
};

static void tellHandle(HandleMsg* msg) {
  vt::NodeType this_node = vt::theContext()->getNode();

  if (this_node != 0) {
    fmt::print("{}: handle={}, requesting data\n", this_node, msg->han);
    int const num_elm = 2;
    vt::theRDMA()->getTypedDataInfoBuf(
      msg->han, &my_data[0], num_elm, vt::no_byte, vt::no_tag, [=]{
        for (auto i = 0; i < num_elm; i++) {
          fmt::print("node {}: \t: my_data[{}] = {}\n", this_node, i, my_data[i]);
        }
      }
    );
  }
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();

  my_data = std::make_unique<double[]>(my_data_len);

  // initialize my_data buffer, all but node 0 get -1.0
  for (auto i = 0; i < my_data_len; i++) {
    my_data[i] = this_node == 0 ? (this_node+1)*i+1 : -1.0;
  }

  if (this_node == 0) {
    vt::RDMA_HandleType my_handle =
      vt::theRDMA()->registerNewTypedRdmaHandler(&my_data[0], my_data_len);

    auto msg = vt::makeMessage<HandleMsg>(this_node);
    msg->han = my_handle;
    vt::theMsg()->broadcastMsg<HandleMsg, tellHandle>(msg);
  }

  vt::finalize();

  return 0;
}
