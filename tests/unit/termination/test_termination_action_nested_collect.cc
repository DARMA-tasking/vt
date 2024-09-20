/*
//@HEADER
// *****************************************************************************
//
//                  test_termination_action_nested_collect.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

struct TestTermNestedCollect : action::BaseFixture {
  void kernel(int depth, EpochType parent) {
    vtAssert(depth > 0, "Wrong depth");
    auto epoch = vt::theTerm()->makeEpochCollective(
      term::ParentEpochCapture{parent}
    );

    // all ranks should have the same depth
    vt::theCollective()->barrier();
    if (depth > 1) { kernel(depth - 1, epoch); }

    if (channel::node == channel::root) {
      action::compute(epoch);
      channel::trigger(epoch);
      action::add(epoch, order_);
    }
    vt::theCollective()->barrier();
    action::finalize(epoch, order_);
  }
};

TEST_P(TestTermNestedCollect, test_term_detect_nested_collect_epoch) {
  kernel(depth_, no_epoch);
}

INSTANTIATE_TEST_SUITE_P(
  InstantiationName, TestTermNestedCollect,
  ::testing::Combine(
    ::testing::Range(0, 3),
    ::testing::Values(false),
    ::testing::Range(2, 10, 2)
  )
);

}}} // end namespace vt::tests::unit
