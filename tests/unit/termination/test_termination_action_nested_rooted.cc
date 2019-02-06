/*
//@HEADER
// ************************************************************************
//
//              test_termination_action_nested_rooted.cc
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

#include "test_termination_action_common.h"

namespace vt { namespace tests { namespace unit {

struct TestTermActionNestedRootedEpoch : action::BaseFixture {

  void kernel(int depth) {
    vtAssertExpr(depth > 0);

    auto ep = vt::no_epoch;

    if (channel::me == channel::root) {
      // explicitly set 'child' epoch param
      ep = vt::theTerm()->newEpochRooted(useDS_,true);
      vt::theTerm()->getWindow(ep)->addEpoch(ep);
      // check that epoch is effectively rooted
      vtAssertExpr(channel::root == epoch::EpochManip::node(ep));
      vtAssertExpr(epoch::EpochManip::isRooted(ep));
    }

    // all ranks should have the same depth
    vt::theCollective()->barrier();
    if (depth > 1) {
      kernel(depth-1);
    }

    if (channel::me == channel::root) {
      action::compute(ep);
      channel::trigger(ep);
      action::finalize(ep,order_);
    }
  }
};

TEST_P(TestTermActionNestedRootedEpoch, test_term_detect_nested_rooted_epoch) {
  kernel(depth_);
}

INSTANTIATE_TEST_CASE_P(
  InstantiationName, TestTermActionNestedRootedEpoch,
  ::testing::Combine(
    ::testing::Values(
      action::Order::before, action::Order::after, action::Order::misc
    ),
    ::testing::Bool(),
    ::testing::Range(2,10,2)
  )
);

}}} // end namespace vt::tests::unit