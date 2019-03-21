/*
//@HEADER
// ************************************************************************
//
//                          term_ds.cc
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

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;
static EpochType cur_epoch = no_epoch;
static int32_t num = 2;

struct TestMsg : Message {};

static void test_handler(TestMsg* msg) {
  ::fmt::print("node={}, running handler: num={}\n", my_node, num);
  num--;

  if (num > 0) {
    auto msg2 = makeSharedMessage<TestMsg>();
    envelopeSetEpoch(msg2->env, cur_epoch);
    theMsg()->sendMsg<TestMsg,test_handler>((my_node + 1) % num_nodes,msg2);
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes == 1) {
    CollectiveOps::output("requires at least 2 nodes");
    CollectiveOps::finalize();
    return 0;
  }

  if (0) {
    cur_epoch = theTerm()->newEpoch();

    fmt::print("{}: new cur_epoch={}\n", my_node, cur_epoch);

    theMsg()->sendMsg<TestMsg,test_handler>(
      (my_node + 1) % num_nodes,
      makeSharedMessage<TestMsg>()
    );

    theTerm()->addAction(cur_epoch, []{
      fmt::print("{}: running attached action: cur_epoch={}\n", my_node, cur_epoch);
    });
    theTerm()->finishedEpoch(cur_epoch);
  }

  if (my_node == 0) {
    cur_epoch = theTerm()->newEpochRooted(true);

    theTerm()->addAction(cur_epoch, []{
      fmt::print("{}: running attached action: cur_epoch={}\n", my_node, cur_epoch);
    });

    auto msg = makeSharedMessage<TestMsg>();
    envelopeSetEpoch(msg->env, cur_epoch);
    theMsg()->sendMsg<TestMsg,test_handler>((my_node + 1) % num_nodes,msg);
    theTerm()->finishedEpoch(cur_epoch);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}

