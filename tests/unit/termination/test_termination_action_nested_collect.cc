/*
//@HEADER
// ************************************************************************
//
//              test_termination_action_nested_collect.h
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

struct TestTermActionNestedCollectEpoch : action::BaseFixture {

  void kernel(int depth){
    vtAssertExpr(depth > 0);

    // explicitly set 'child' epoch param
    auto ep = vt::theTerm()->newEpochCollective(true);
    vt::theTerm()->getWindow(ep)->addEpoch(ep);

    // all ranks should have the same depth
    vt::theCollective()->barrier();
    if (depth > 1) {
      kernel(depth-1);
    }

    if (channel::me == channel::root){
      action::compute(ep);
      channel::trigger(ep);
    }
    action::finalize(ep,order_);
  }
};

TEST_P(TestTermActionNestedCollectEpoch, test_term_detect_nested_collect_epoch){
  kernel(depth_);
}

INSTANTIATE_TEST_CASE_P(
  InstantiationName, TestTermActionNestedCollectEpoch,
  ::testing::Combine(
    ::testing::Values(
      action::Order::before, action::Order::after, action::Order::misc
    ),
    ::testing::Values(false),
    ::testing::Range(2,10,2)
  )
);


}}} // end namespace vt::tests::unit