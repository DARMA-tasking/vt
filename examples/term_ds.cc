/*
//@HEADER
// *****************************************************************************
//
//                                  term_ds.cc
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
    cur_epoch = theTerm()->makeEpochCollective();

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
    cur_epoch = theTerm()->makeEpochRooted(term::UseDS{true});

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

