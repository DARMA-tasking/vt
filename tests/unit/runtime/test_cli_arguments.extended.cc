/*
//@HEADER
// *****************************************************************************
//
//                        test_cli_arguments.extended.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"

#include <vt/configs/arguments/app_config.h>

namespace vt { namespace tests { namespace unit {

struct TestCliArguments : TestParallelHarness { };

#if not vt_check_enabled(production_build)
TEST_F(TestCliArguments, test_vt_assert) {
#if defined(__clang__)
  #if __clang_major__ == 9 || __clang_major__ == 10
    GTEST_SKIP() << "Skipping test for Clang 9 or 10.";
  #endif
#else
  EXPECT_EQ(theConfig()->vt_no_assert_fail, false);

  ASSERT_THROW(
    vtAssert(false, "Should throw."),
    std::runtime_error
  );
#endif
}
#endif

TEST_F(TestCliArguments, test_assert_no_fail) {
  EXPECT_EQ(theConfig()->vt_no_assert_fail, false);
  theConfig()->vt_no_assert_fail = true;

  vtAssert(false, "Should not abort.");
  SUCCEED();
}

}}} // end namespace vt::tests::unit
