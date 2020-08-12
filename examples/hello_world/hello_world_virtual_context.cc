/*
//@HEADER
// *****************************************************************************
//
//                        hello_world_virtual_context.cc
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

struct HelloMsg : vt::Message {
  vt::VirtualProxyType proxy;

  explicit HelloMsg(vt::VirtualProxyType const& in_proxy)
    : proxy(in_proxy)
  { }
};

struct TestMsg : vt::vrt::VirtualMessage {
  int from = 0;

  explicit TestMsg(int const& in_from)
    : from(in_from)
  { }
};

struct MyVC : vt::vrt::VirtualContext {
  int my_data = 10;

  explicit MyVC(int const& my_data_in)
    : my_data(my_data_in)
  { }
};

static void my_han(TestMsg* msg, MyVC* vc) {
  auto this_node = vt::theContext()->getNode();
  fmt::print(
    "{}: vc={}: msg->from={}, vc->my_data={}\n",
    this_node, print_ptr(vc), msg->from, vc->my_data
  );
}

static void sendMsgToProxy(vt::VirtualProxyType const& proxy) {
  auto this_node = vt::theContext()->getNode();
  fmt::print("{}: sendMsgToProxy: proxy={}\n", this_node, proxy);

  auto m = vt::makeMessage<TestMsg>(this_node + 32);
  vt::theVirtualManager()->sendMsg<MyVC, TestMsg, my_han>(proxy, m.get());
}

static void hello_world(HelloMsg* msg) {
  auto this_node = vt::theContext()->getNode();
  fmt::print("{}: hello: proxy={}\n", this_node, msg->proxy);
  sendMsgToProxy(msg->proxy);
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

  if (this_node == 0) {
    auto proxy = vt::theVirtualManager()->makeVirtual<MyVC>(29);
    sendMsgToProxy(proxy);

    // send out the proxy to all the nodes
    auto msg = vt::makeMessage<HelloMsg>(proxy);
    vt::theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);
  }

  vt::finalize();

  return 0;
}
