/*
//@HEADER
// ************************************************************************
//
//                 test_termination_action_callable.cc
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

#if !defined INCLUDED_TERMINATION_ACTION_CALLABLE_H
#define INCLUDED_TERMINATION_ACTION_CALLABLE_H

namespace vt { namespace tests { namespace unit {

static bool finished[2] = {false, false};
static int num = 0;

// no need for a parameterized fixture
struct TestTermCallable : action::SimpleFixture {

  void verify(int cur) {
    int const nxt = (cur + 1) % 2;
    EXPECT_FALSE(finished[cur]);
    EXPECT_TRUE(not finished[nxt] or num == 1);
    finished[cur] = true;
    num++;
  }
};

TEST_F(TestTermCallable, test_add_action_unique) /*NOLINT*/{

  finished[0] = finished[1] = false;
  num = 0;

  vt::theCollective()->barrier();

  // create an epoch and a related termination flag
  auto epoch = ::vt::theTerm()->makeEpochCollective();

  // assign an arbitrary action to be triggered at
  // the end of the epoch and toggle the previous flag.
  ::vt::theTerm()->addActionEpoch(epoch, [&]{
    debug_print(
      term, node,
      "current epoch:{:x} finished\n",
      epoch
    );
    verify(0);
  });

  // assign a callable to be triggered after
  // the action submitted for the given epoch.
  ::vt::theTerm()->addActionUnique(epoch, [&]{
    debug_print(
      term, node,
      "trigger callable for epoch:{:x}\n",
      epoch
    );
    verify(1);
  });

  if (channel::node == channel::root) {
    action::compute(epoch);
  }

  ::vt::theTerm()->finishedEpoch(epoch);
}

}}} // namespace vt::tests::unit::action

#endif /*INCLUDED_TERMINATION_ACTION_CALLABLE_H*/
