/*
//@HEADER
// ************************************************************************
//
//                   test_termination_action_epochs.cc
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

struct TestTermCollect : action::BaseFixture {};
struct TestTermRooted  : action::BaseFixture {};

// collective epochs
TEST_P(TestTermCollect, test_term_detect_collect_epoch) /* NOLINT */{
  auto&& sequence = action::generateEpochs(5);

  for (auto&& epoch : sequence) {
    action::ok = false;
    if (channel::node == channel::root) {
      action::compute(epoch);
      channel::trigger(epoch);
      action::add(epoch, order_);
    }
    vt::theCollective()->barrier();
    action::finalize(epoch, order_);
  }
}

// rooted epochs
TEST_P(TestTermRooted, test_term_detect_rooted_epoch) /* NOLINT */{
  if (channel::node == channel::root) {
    auto&& sequence = action::generateEpochs(5, true, useDS_);

    for (auto&& epoch : sequence) {
      debug_print(
        term, node,
        "epoch={:x}, order_={}, useDS_={}\n",
        epoch, order_, useDS_
      );

      action::compute(epoch);
      action::finalize(epoch, order_);
    }
  }
}

INSTANTIATE_TEST_CASE_P /* NOLINT */(
  InstantiationName, TestTermCollect,
  ::testing::Combine(
    ::testing::Range(0, 4),
    ::testing::Values(false),
    ::testing::Values(1)
  ),
);

INSTANTIATE_TEST_CASE_P /* NOLINT */(
  InstantiationName, TestTermRooted,
  ::testing::Combine(
    ::testing::Range(0, 3),
    ::testing::Bool(),
    ::testing::Values(1)
  ),
);

}}} // end namespace vt::tests::unit