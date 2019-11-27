/*
//@HEADER
// *****************************************************************************
//
//                      test_termination_action_epochs.cc
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

INSTANTIATE_TEST_SUITE_P(
  InstantiationName, TestTermCollect,
  ::testing::Combine(
    ::testing::Range(0, 4),
    ::testing::Values(false),
    ::testing::Values(1)
  )
);

INSTANTIATE_TEST_SUITE_P(
  InstantiationName, TestTermRooted,
  ::testing::Combine(
    ::testing::Range(0, 3),
    ::testing::Bool(),
    ::testing::Values(1)
  )
);

}}} // end namespace vt::tests::unit
