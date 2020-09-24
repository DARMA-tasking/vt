/*
//@HEADER
// *****************************************************************************
//
//                            test_epoch_scopes.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include <list>
#include <set>

namespace vt { namespace tests { namespace unit {

using TestEpochScopes = TestParallelHarness;

///////////////////////////////////////////////////////////////////////////////
// Start by exercising the scope allocation/de-allocation code to ensure that
// unique scopes are returned under varying conditions
///////////////////////////////////////////////////////////////////////////////

TEST_F(TestEpochScopes, test_epoch_scope_1) {

  // Create a list of scopes up to the limit
  std::list<epoch::EpochCollectiveScope> scopes;

  for (epoch::EpochScopeType i = 0; i < epoch::scope_limit; i++) {
    scopes.emplace_back(vt::theEpoch()->makeScopeCollective());
  }

  EXPECT_EQ(scopes.size(), epoch::scope_limit);

  // Ensure that all scope IDs are unique
  {
    std::set<epoch::EpochScopeType> scope_ids;
    for (auto&& s : scopes) {
      // fmt::print("scope={:x}\n", s.getScope());
      scope_ids.insert(s.getScope());
    }
    EXPECT_EQ(scopes.size(), scope_ids.size());
  }

  // Remove/deallocate the first three
  for (int i = 0; i < 3; i++) {
    scopes.pop_front();
  }

  // Add others to replace them
  for (int i = 0; i < 3; i++) {
    scopes.emplace_back(vt::theEpoch()->makeScopeCollective());
  }

  // Erase every other scope
  bool remove = false;
  auto iter = scopes.begin();
  while (iter != scopes.end()) {
    if (remove) {
      // fmt::print("erasing scope={:x}\n", iter->getScope());
      iter = scopes.erase(iter);
    }
    remove = not remove;
    ++iter;
  }

  // Add them back in
  while (scopes.size() < epoch::scope_limit) {
    scopes.emplace_back(vt::theEpoch()->makeScopeCollective());
  }

  // Ensure that all scope IDs are unique
  {
    std::set<epoch::EpochScopeType> scope_ids;
    for (auto&& s : scopes) {
      // fmt::print("scope={:x}\n", s.getScope());
      scope_ids.insert(s.getScope());
    }
    EXPECT_EQ(scopes.size(), scope_ids.size());
  }
}

///////////////////////////////////////////////////////////////////////////////
// Test creating epochs from a scope in different orders on nodes. Then, try
// sending messages to ensure the scope is propagated. Try creating child epochs
// from those to ensure the scope propagates to the children.
///////////////////////////////////////////////////////////////////////////////

struct TestMsg : vt::Message {
  explicit TestMsg(epoch::EpochScopeType in_scope)
    : scope_(in_scope)
  { }

  epoch::EpochScopeType scope_ = epoch::no_scope;
};

static void rootedHandler(TestMsg* msg);

static void shiftRightHandler(TestMsg* msg) {
  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  /// Check that the scope matches as expected
  auto ep = theMsg()->getEpoch();
  auto cur_scope = epoch::EpochManip::getScope(ep);
  fmt::print(
    "{}: ep={:x} cur_scope={:x} msg->scope_={:x}\n",
    this_node, ep, cur_scope, msg->scope_
  );
  EXPECT_EQ(msg->scope_, cur_scope);

  /// Create a new rooted epoch to see if the scope correctly
  /// transfers/propagates to it
  auto rooted_ep = theTerm()->makeEpochRooted(term::UseDS{true});
  auto rooted_scope = epoch::EpochManip::getScope(rooted_ep);

  fmt::print(
    "{}: rooted_ep={:x} cur_scope={:x} rooted_scope={:x}\n",
    this_node, rooted_ep, cur_scope, rooted_scope
  );

  // Scope must immediately transfer onto the new rooted epoch
  EXPECT_EQ(rooted_scope, cur_scope);
  theMsg()->pushEpoch(rooted_ep);

  auto next_node = (this_node + 1) % num_nodes;
  auto nmsg = makeMessage<TestMsg>(cur_scope);
  theMsg()->sendMsg<TestMsg, rootedHandler>(next_node, nmsg);

  theMsg()->popEpoch(rooted_ep);
  theTerm()->finishedEpoch(rooted_ep);
}

static void rootedHandler(TestMsg* msg) {
  auto const this_node = theContext()->getNode();

  // Check that the rooted scope is correct in the handler
  auto ep = theMsg()->getEpoch();
  auto cur_scope = epoch::EpochManip::getScope(ep);
  fmt::print(
    "{}: ROOTED ep={:x} cur_scope={:x} msg->scope_={:x}\n",
    this_node, ep, cur_scope, msg->scope_
  );
  EXPECT_EQ(msg->scope_, cur_scope);
}

TEST_F(TestEpochScopes, test_epoch_scope_2) {
  // Test creating epochs from a scope

  auto const this_node = theContext()->getNode();
  auto const num_nodes = theContext()->getNumNodes();

  // Must have at least 2 nodes to run this test
  if (num_nodes < 2) {
    return;
  }

  static constexpr epoch::EpochScopeType const num = 3;

  std::vector<epoch::EpochCollectiveScope> scopes;

  // All nodes construct epoch scopes in the same order and push them
  for (epoch::EpochScopeType i = 0; i < num; i++) {
    scopes.push_back(vt::theEpoch()->makeScopeCollective());
  }

  for (auto&& s : scopes) {
    fmt::print("scope={:x}\n", s.getScope());
  }

  auto send_from_scope = [&](epoch::EpochCollectiveScope& s) {
    auto ep = s.makeEpochCollective("test");
    auto scope = s.getScope();
    theMsg()->pushEpoch(ep);

    fmt::print("{}: epoch={:x} scope={:x}\n", this_node, ep, scope);

    EXPECT_EQ(scope, epoch::EpochManip::getScope(ep));

    auto next_node = (this_node + 1) % num_nodes;
    auto msg = makeMessage<TestMsg>(scope);
    theMsg()->sendMsg<TestMsg, shiftRightHandler>(next_node, msg);

    theMsg()->popEpoch(ep);
    vt::theTerm()->finishedEpoch(ep);
  };

  // Create epochs in different orders
  if (this_node < num_nodes/2) {
    for (epoch::EpochScopeType i = 0; i < num; i++) {
      send_from_scope(scopes[i]);
    }
  } else {
    for (epoch::EpochScopeType v = scopes.size(); v > 0; v--) {
      auto const i = v-1;
      send_from_scope(scopes[i]);
    }
  }
}

}}} // end namespace vt::tests::unit
