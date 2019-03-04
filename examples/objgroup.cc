/*
//@HEADER
// ************************************************************************
//
//                          objgroup.cc
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

struct MyMsg : vt::Message {
  MyMsg(int in_a, int in_b) : a(in_a), b(in_b) { }
  int a = 0, b = 0;
};

struct MyObjGroup {
  void handler(MyMsg* msg) {
    auto node = vt::theContext()->getNode();
    fmt::print("{}: MyObjGroup::handler on a={}, b={}\n", node, msg->a, msg->b);
  }
};

int main(int argc, char** argv) {
  vt::initialize(argc, argv, nullptr);

  auto const& this_node = vt::theContext()->getNode();
  auto const& num_nodes = vt::theContext()->getNumNodes();

  if (num_nodes < 2) {
    return 0;
  }

  auto proxy = vt::theObjGroup()->makeCollective<MyObjGroup>();

  if (this_node == 0) {
    proxy[0].send<MyMsg,&MyObjGroup::handler>(5,10);
    proxy[1].send<MyMsg,&MyObjGroup::handler>(10,20);
    proxy.broadcast<MyMsg,&MyObjGroup::handler>(400,500);
  }

  while (!vt::rt->isTerminated()) {
    vt::runScheduler();
  }

  vt::finalize();

  return 0;
}
