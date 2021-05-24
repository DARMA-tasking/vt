/*
//@HEADER
// *****************************************************************************
//
//                            test_harness_macros.h
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

#if !defined __VIRTUAL_TRANSPORT_TEST_PERFORMANCE_PARALLEL_HARNESS_MACROS__
#define __VIRTUAL_TRANSPORT_TEST_PERFORMANCE_PARALLEL_HARNESS_MACROS__

namespace vt { namespace tests { namespace perf { namespace common {

static constexpr uint32_t NUM_ITERS = 20;

  #define VT_PERF_TEST(StructName, TestName)               \
    struct StructName##TestName : StructName {             \
      void SetUp(int argc, char** argv) override {         \
        StructName::SetUp(argc, argv);                     \
        name_ = #TestName;                                 \
      }                                                    \
      void TearDown() override { StructName::TearDown(); } \
      void TestFunc();                                     \
    };                                                     \
    using TestType = StructName##TestName;                 \
    void StructName##TestName::TestFunc()

  #define VT_TIME_EXEC(func)            \
    {                                   \
      ScopedTimer(#func);               \
      func;                             \
      PerfTestHarness::SpinScheduler(); \
    }

  #define RUN_PERF(func)                       \
    for (uint32_t i = 0; i < NUM_ITERS; ++i) { \
      VT_TIME_EXEC(func);                      \
    }

  #define VT_PERF_TEST_MAIN()         \
    int main(int argc, char** argv) { \
      TestType t;                     \
      t.SetUp(argc, argv);            \
      RUN_PERF(t.TestFunc());         \
      t.TearDown();                   \
                                      \
      return 0;                       \
    }

}}}} // namespace vt::tests::perf::common

#endif // __VIRTUAL_TRANSPORT_TEST_PERFORMANCE_PARALLEL_HARNESS_MACROS__
