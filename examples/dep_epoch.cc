/*
//@HEADER
// ************************************************************************
//
//                           dep_epoch.cc
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

struct HelloMsg : vt::Message {
  HelloMsg(vt::NodeType in_from) : from(in_from) { }

  vt::EpochType ep = vt::no_epoch;
  vt::NodeType from = 0;
};

struct MyObjGroup {
  void handlerA(HelloMsg* msg) {
    auto node = vt::theContext()->getNode();
    fmt::print("{}: MyObjGroup::handlerA on from={}\n", node, msg->from);
  }
  void handlerB(HelloMsg* msg) {
    auto node = vt::theContext()->getNode();
    fmt::print("{}: MyObjGroup::handlerB on from={}\n", node, msg->from);
  }
};

struct HelloMsg2;

struct MyCol : vt::Collection<MyCol,vt::Index1D> {
  void handlerA(HelloMsg2* msg);
  void handlerB(HelloMsg2* msg);
};

struct HelloMsg2 : vt::CollectionMessage<MyCol> {
  HelloMsg2(vt::NodeType in_from) : from(in_from) { }

  vt::EpochType ep = vt::no_epoch;
  vt::NodeType from = 0;
};

vt::objgroup::proxy::Proxy<MyObjGroup> proxy;
vt::CollectionProxy<MyCol,vt::Index1D> cproxy;

void MyCol::handlerA(HelloMsg2* msg) {
  auto node = vt::theContext()->getNode();
  auto idx = this->getIndex();
  fmt::print("{}: {}: MyCol::handlerA on from={}\n", node, idx, msg->from);
}

void MyCol::handlerB(HelloMsg2* msg) {
  auto node = vt::theContext()->getNode();
  auto idx = this->getIndex();
  fmt::print("{}: {}: MyCol::handlerB on from={}\n", node, idx, msg->from);
}

//vt::EpochType epoch = vt::no_epoch;

static void testHan(HelloMsg* msg) {
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();
  vt::NodeType node = vt::theContext()->getNode();
  vt::NodeType next = node + 1 < num_nodes ? node + 1 : 0;
  fmt::print("{}: testHan from={}, epoch={:x}\n", node, msg->from, msg->ep);
  vt::theTerm()->releaseEpoch(msg->ep);

  proxy[next].release(msg->ep);

  // if (node == 0) {
  //   cproxy.release(msg->ep);
  // }
  for (int i = node * 10; i < (node+1)*10; i++) {
    cproxy[i].release(msg->ep);
  }

  if (node == 1) {
    auto msg2 = vt::makeSharedMessage<HelloMsg>(node);
    msg2->ep = msg->ep;
    vt::theMsg()->sendMsg<HelloMsg, testHan>(0, msg2);
  }

  // vt::NodeType this_node = vt::theContext()->getNode();
  // fmt::print(
  //   "{}: testHan from node {}, epoch={:x}\n", this_node, msg->from, epoch
  // );
  // vt::theTerm()->releaseEpoch(epoch);
}

static void hello_world(HelloMsg* msg) {
  vt::NodeType node = vt::theContext()->getNode();
  fmt::print("{}: Hello from node {}\n", node, msg->from);
}

int main(int argc, char** argv) {
  vt::initialize(argc, argv);

  vt::NodeType this_node = vt::theContext()->getNode();
  vt::NodeType num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes == 1) {
    return vt::rerror("requires at least 2 nodes");
  }

  proxy = vt::theObjGroup()->makeCollective<MyObjGroup>();
  cproxy = vt::theCollection()->constructCollective<MyCol>(vt::Index1D(num_nodes * 10));

  if (this_node == 0) {
    auto msg = vt::makeSharedMessage<HelloMsg>(static_cast<vt::NodeType>(100));
    proxy.broadcast<HelloMsg,&MyObjGroup::handlerA>(msg);

    auto msg2 = vt::makeSharedMessage<HelloMsg2>(static_cast<vt::NodeType>(100));
    cproxy.broadcast<HelloMsg2,&MyCol::handlerA>(msg2);
  }

  //epoch = vt::theTerm()->makeEpochCollectiveDep();

  if (this_node == 0) {
    auto epoch = vt::theTerm()->makeEpochRootedDep();
    vt::theMsg()->pushEpoch(epoch);

    auto msg = vt::makeSharedMessage<HelloMsg>(this_node);
    vt::theMsg()->broadcastMsg<HelloMsg, hello_world>(msg);

    auto msg3 = vt::makeSharedMessage<HelloMsg>(static_cast<vt::NodeType>(101));
    proxy.broadcast<HelloMsg,&MyObjGroup::handlerB>(msg3);

    auto msg4 = vt::makeSharedMessage<HelloMsg2>(static_cast<vt::NodeType>(101));
    cproxy.broadcast<HelloMsg2,&MyCol::handlerB>(msg4);

    vt::theMsg()->popEpoch(epoch);
    vt::theTerm()->finishedEpoch(epoch);

    auto msg2 = vt::makeSharedMessage<HelloMsg>(this_node);
    msg2->ep = epoch;
    vt::theMsg()->broadcastMsg<HelloMsg, testHan>(msg2);
  }

  //vt::theTerm()->finishedEpoch(epoch);

  while (!vt::rt->isTerminated()) {
    vt::runScheduler();
  }

  vt::finalize();

  return 0;
}
