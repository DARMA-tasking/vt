/*
//@HEADER
// *****************************************************************************
//
//                                test_invoke.cc
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

#include "test_parallel_harness.h"
#include "vt/runnable/invoke.h"

#include <vector>
#include <numeric>
#include <gtest/gtest.h>

namespace vt { namespace tests { namespace unit {

struct TestInvoke : TestParallelHarness {
  virtual void addAdditionalArgs() override {
    static char traceon[]{"--vt_trace=1"};
    addArgs(traceon);
  }
};

static int test_value = -1;

struct A {
  virtual ~A() = default;

  virtual int memberFunc(const std::vector<int>& vec) = 0;
};

struct B : A {
  ~B() override = default;

  int memberFunc(const std::vector<int>& vec) override {
    test_value = 30;
    return std::accumulate(std::begin(vec), std::end(vec), 0);
  }
};

void voidWithArg(int in_val) { test_value = in_val; }

std::unique_ptr<A> nonCopyableFun() {
  test_value = 20;
  return std::make_unique<B>();
}

TEST_F(TestInvoke, test_invoke_call) {
  vt::runnable::invoke<decltype(&voidWithArg), &voidWithArg>(10);
  EXPECT_EQ(test_value, 10);

  auto b = vt::runnable::invoke<decltype(&nonCopyableFun), &nonCopyableFun>();
  EXPECT_EQ(test_value, 20);

  auto accumulate_result =
    vt::runnable::invoke<decltype(&A::memberFunc), &A::memberFunc>(
      b.get(), std::vector<int32_t>{10, 20, 30}
    );
  EXPECT_EQ(accumulate_result, 60);
  EXPECT_EQ(test_value, 30);
}


}}} // end namespace vt::tests::unit
