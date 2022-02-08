/*
//@HEADER
// *****************************************************************************
//
//                                test_harness.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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
#if ! defined INCLUDED_UNIT_TEST_HARNESS_H
#define INCLUDED_UNIT_TEST_HARNESS_H

#include <vt/config.h>
#include <gtest/gtest.h>

#include <vector>
#include <string>

#include "test_config.h"
#include "data_message.h"

namespace vt {

namespace debug {
arguments::AppConfig *preConfigRef();
} // namespace debug

namespace tests { namespace unit {

template <typename TestBase>
struct TestHarnessAny : TestBase {
  static void store_cmdline_args(int argc, char **argv) {
    orig_args_ = std::vector<std::string>(argv, argv + argc);

    vt::debug::preConfigRef()->vt_throw_on_abort = true;
  }

  static std::vector<std::string> orig_args_;
};

template <typename TestBase>
std::vector<std::string> TestHarnessAny<TestBase>::orig_args_;

using TestHarness = TestHarnessAny<testing::Test>;

template <typename ParamT>
using TestHarnessParam = TestHarnessAny<testing::TestWithParam<ParamT>>;

}} // end namespace tests::unit

} // namespace vt

#endif /* INCLUDED_UNIT_TEST_HARNESS_H */
