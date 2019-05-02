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

struct TestTermNestedRooted : action::BaseFixture {

  void kernel(int depth) {
    vtAssert(depth > 0, "Wrong depth");
    using epoch_manip = ::vt::epoch::EpochManip;
    auto epoch = vt::no_epoch;

    if (channel::node == channel::root) {
      epoch = vt::theTerm()->makeEpochRooted(useDS_);
      // check that epoch is effectively rooted
      vtAssert(channel::root == epoch_manip::node(epoch), "Node should be root");
      vtAssert(epoch_manip::isRooted(epoch), "Epoch should be rooted");
    }

    // all ranks should have the same depth
    vt::theCollective()->barrier();
    if (depth > 1) { kernel(depth - 1); }

    if (channel::node == channel::root) {
      action::compute(epoch);
      channel::trigger(epoch);
      action::finalize(epoch, order_);
    }
  }
};

TEST_P(TestTermNestedRooted, test_term_detect_nested_rooted_epoch) /* NOLINT */{
  kernel(depth_);
}

INSTANTIATE_TEST_CASE_P /*NOLINT*/(
  InstantiationName, TestTermNestedRooted,
  ::testing::Combine(
    ::testing::Range(0, 3),
    ::testing::Bool(),
    ::testing::Range(2, 10, 2)
  ),
);

}}} // end namespace vt::tests::unit