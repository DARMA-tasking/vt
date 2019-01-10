/*
//@HEADER
// ************************************************************************
//
//                          test_harness.h
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
#if ! defined __VIRTUAL_TRANSPORT_TEST_HARNESS__
#define __VIRTUAL_TRANSPORT_TEST_HARNESS__

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <string>

#include "test_config.h"
#include "data_message.h"

namespace vt { namespace tests { namespace unit {

extern int test_argc;
extern char** test_argv;

template <typename TestBase>
struct TestHarnessAny : TestBase {
  virtual void SetUp() {
    argc_ = orig_args_.size();
    argv_ = new char*[argc_];

    for (int i = 0; i < argc_; i++) {
      auto const len = orig_args_[i].size();
      argv_[i] = new char[len + 1];
      argv_[i][len] = 0;
      orig_args_[i].copy(argv_[i], len);
    }
  }

  virtual void TearDown() {
    for (int i = 0; i < argc_; i++) {
      delete [] argv_[i];
    }
    delete [] argv_;
  }

  static void store_cmdline_args(int argc, char **argv) {
    orig_args_ = std::vector<std::string>(argv, argv + argc);
  }

  static std::vector<std::string> orig_args_;

  int argc_ = 0;
  char** argv_ = nullptr;
};

template <typename TestBase>
std::vector<std::string> TestHarnessAny<TestBase>::orig_args_;

using TestHarness = TestHarnessAny<testing::Test>;

template <typename ParamT>
using TestHarnessParam = TestHarnessAny<testing::TestWithParam<ParamT>>;

}}} // end namespace vt::tests::unit

#endif /* __VIRTUAL_TRANSPORT_TEST_HARNESS__ */
