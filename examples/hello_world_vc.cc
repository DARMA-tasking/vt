/*
//@HEADER
// ************************************************************************
//
//                          hello_world_vc.cc
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
using namespace vt::vrt;

struct HelloMsg : vt::Message {
  VirtualProxyType proxy;

  HelloMsg(VirtualProxyType const& in_proxy)
    : Message(), proxy(in_proxy)
  { }
};

struct TestMsg : vt::vrt::VirtualMessage {
  int from = 0;

  TestMsg(int const& in_from)
    : VirtualMessage(), from(in_from)
  { }
};

struct MyVC : vt::vrt::VirtualContext {
  int my_data = 10;

  MyVC(int const& my_data_in) : my_data(my_data_in) { }
};

static void my_han(TestMsg* msg, MyVC* vc) {
  auto this_node = theContext()->getNode();
  fmt::print(
    "{}: vc={}: msg->from={}, vc->my_data={}\n",
    this_node, print_ptr(vc), msg->from, vc->my_data
  );
}

static void sendMsgToProxy(VirtualProxyType const& proxy) {
  auto this_node = theContext()->getNode();
  fmt::print("{}: sendMsgToProxy: proxy={}\n", this_node, proxy);

  auto m = makeSharedMessage<TestMsg>(this_node + 32);
  theVirtualManager()->sendMsg<MyVC, TestMsg, my_han>(proxy, m);
}

static void hello_world(HelloMsg* msg) {
  auto this_node = theContext()->getNode();
  fmt::print("{}: hello: proxy={}\n", this_node, msg->proxy);
  sendMsgToProxy(msg->proxy);
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  auto const& my_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  if (my_node == 0) {
    auto proxy = theVirtualManager()->makeVirtual<MyVC>(29);
    sendMsgToProxy(proxy);

    // send out the proxy to all the nodes
    HelloMsg* msg = makeSharedMessage<HelloMsg>(proxy);
    theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
