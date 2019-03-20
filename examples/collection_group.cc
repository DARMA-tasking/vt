/*
//@HEADER
// ************************************************************************
//
//                          collection_group.cc
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
using namespace vt::group;

struct TestMsg;

struct MyReduceMsg : collective::ReduceMsg {
  MyReduceMsg(int const& in_num) : num(in_num) {}
  int num = 0;
};

struct ColA : Collection<ColA,Index1D> {
  ColA()
    : Collection<ColA,Index1D>()
  {
    // auto const& this_node = theContext()->getNode();
    // ::fmt::print("{}: constructing: idx={}\n", this_node, getIndex().x());
  }

  void work(TestMsg* msg);
  void work2(TestMsg* msg);
};

struct TestMsg : CollectionMessage<ColA> { };

void ColA::work2(TestMsg* msg) {
  auto const& this_node = theContext()->getNode();
  ::fmt::print("work2: node={}, idx={}\n", this_node, getIndex().x());
}

static void reduceNone(MyReduceMsg* msg) {
  if (msg->isRoot()) {
    fmt::print("{}: at root: final num={}\n", theContext()->getNode(), msg->num);
  } else {
    MyReduceMsg* fst_msg = msg;
    MyReduceMsg* cur_msg = msg->getNext<MyReduceMsg>();
    while (cur_msg != nullptr) {
      // fmt::print(
      //   "{}: while fst_msg={}: cur_msg={}, is_root={}, count={}, next={}, num={}\n",
      //   theContext()->getNode(),
      //   print_ptr(fst_msg), print_ptr(cur_msg), print_bool(cur_msg->isRoot()),
      //   cur_msg->getCount(), print_ptr(cur_msg->getNext<MyReduceMsg>()),
      //   cur_msg->num
      // );
      fst_msg->num += cur_msg->num;
      cur_msg = cur_msg->getNext<MyReduceMsg>();
    }
  }
}

void ColA::work(TestMsg* msg) {
  auto const& this_node = theContext()->getNode();
  ::fmt::print("work: node={}, idx={}\n", this_node, getIndex().x());

  if (getIndex().x() == 2) {
    auto const& proxy = getCollectionProxy();
    auto const& msg = makeSharedMessage<TestMsg>();
    proxy.broadcast<TestMsg,&ColA::work2>(msg);
  }

  auto reduce_msg = makeSharedMessage<MyReduceMsg>(getIndex().x());
  auto const& proxy = getCollectionProxy();
  theCollection()->reduceMsg<ColA,MyReduceMsg,reduceNone>(proxy, reduce_msg);
}

struct HelloMsg : vt::Message {
  int from;

  explicit HelloMsg(int const& in_from)
    : Message(), from(in_from)
  { }
};

struct SysMsg : collective::ReduceTMsg<int> {
  explicit SysMsg(int in_num)
    : ReduceTMsg<int>(in_num)
  { }
};

struct Print {
  void operator()(SysMsg* msg) {
    fmt::print("final value={}\n", msg->getConstVal());
  }
};

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
    auto const& range = Index1D(std::max(num_nodes / 2, 1));
    //auto const& range = Index1D(static_cast<int>(num_nodes));
    auto const& proxy = theCollection()->construct<ColA>(range);
    auto const& msg = makeSharedMessage<TestMsg>();
    proxy.broadcast<TestMsg,&ColA::work>(msg);
    // proxy.broadcast<TestMsg,&ColA::work>(msg);
    // proxy.broadcast<TestMsg,&ColA::work>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
