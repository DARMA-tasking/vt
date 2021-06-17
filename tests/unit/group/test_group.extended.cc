/*
//@HEADER
// *****************************************************************************
//
//                           test_group.extended.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2021 National Technology & Engineering Solutions of Sandia, LLC
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include "vt/group/group_manager.h"

namespace vt { namespace tests { namespace unit {

struct MySimpleMsg : ::vt::Message { };

static void msgHandlerGroup(MySimpleMsg* msg) {
  auto const cur_node = ::vt::theContext()->getNode();
  vtAssert(cur_node % 2 == 0, "This handler should only execute on even nodes");

  ::fmt::print("msgHandlerGroup: triggered on node={}\n", cur_node);
}

// demonstrate collective group creation and broadcast to that group
static inline void activeMessageGroupCollective() {
  NodeType const this_node = ::vt::theContext()->getNode();

  auto const is_even_node = this_node % 2 == 0;

  if (this_node == 0) {
    int val = 10;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    vt::theSched()->runSchedulerWhile(
      [&val]{
        return --val >= 0;
      }
    );
  }

  auto group = theGroup()->newGroupCollective(
    is_even_node, [](GroupType group_id){
      fmt::print("Group is created: id={:x}\n", group_id);

      // node 1 broadcasts to the group of even nodes
      auto const my_node = ::vt::theContext()->getNode();
      if (my_node == 1) {
        auto msg = makeMessage<MySimpleMsg>();
        envelopeSetGroup(msg->env, group_id);
        theMsg()->broadcastMsg<MySimpleMsg,msgHandlerGroup>(msg);
      }
    }
  );
  (void)group;
}

struct TestGroupExtended : TestParallelHarness {};

TEST_F(TestGroupExtended, test_group_collective) {
  vt::runInEpochCollective([] { activeMessageGroupCollective(); });
}

}}} // end namespace vt::tests::unit
