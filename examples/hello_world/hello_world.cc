/*
//@HEADER
// *****************************************************************************
//
//                                hello_world.cc
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
#include <vt/datarep/dr.h>

struct HelloMsg : vt::Message {
  HelloMsg(vt::NodeType in_from, vt::DataRepIDType handle_id)
    : from(in_from),
      handle_id_(handle_id)
  { }

  vt::NodeType from = 0;
  vt::DataRepIDType handle_id_ = vt::no_datarep;
};

static void hello_world(HelloMsg* msg) {
  vt::NodeType this_node = vt::theContext()->getNode();
  fmt::print("{}: Hello from node {} (start)\n", this_node, msg->from);
  auto han_id = msg->handle_id_;
  vt::datarep::Reader<std::vector<double>> my_reader{han_id};
  my_reader.fetch(0);
  vt::theSched()->runSchedulerWhile([&]{ return not my_reader.isReady(); });
  auto const& vec = my_reader.get(0);
  for (auto&& elm : vec) {
    vt_print(gen, "elm={}\n", elm);
  }
  fmt::print("{}: Hello from node {}\n", this_node, msg->from);
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

  if (this_node == 0) {
    std::vector<double> my_vec;
    for (int i = 0; i < 10; i++) {
      my_vec.push_back(i*10);
    }
    auto my_dr = vt::theDR()->makeHandle(0, std::move(my_vec));
    auto msg = vt::makeMessage<HelloMsg>(this_node, my_dr.getHandleID());
    vt::theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);
  }

  vt::finalize();

  return 0;
}
