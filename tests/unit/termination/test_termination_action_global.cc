/*
//@HEADER
// ************************************************************************
//
//                  test_termination_action_global.cc
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

struct TestTermGlobal : action::BaseFixture {};

TEST_P(TestTermGlobal, test_term_detect_broadcast) /* NOLINT*/{
  if (channel::node == channel::root) {
    // start computation
    channel::broadcast<channel::basicHandler>(1, vt::no_epoch);
    // trigger detection and check status
    action::finalize(vt::no_epoch, order_);
  }
}

// routed messages
TEST_P(TestTermGlobal, test_term_detect_routed) /* NOLINT*/{
  // there should be at least 3 nodes for this case
  if (channel::all > 2 and channel::node == channel::root) {
    //start computation
    action::compute(vt::no_epoch);
    // trigger detection and check status
    action::finalize(vt::no_epoch, order_);
  }
}

INSTANTIATE_TEST_CASE_P /* NOLINT*/(
  InstantiationName, TestTermGlobal,
  ::testing::Combine(
    ::testing::Range(0, 3),
    ::testing::Values(false),
    ::testing::Values(1)
  )
);

}}} // end namespace vt::tests::unit