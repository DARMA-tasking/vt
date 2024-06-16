/*
//@HEADER
// *****************************************************************************
//
//                            test_harness_macros.h
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

#if !defined INCLUDED_PERF_COMMON_TEST_HARNESS_MACROS_H
#define INCLUDED_PERF_COMMON_TEST_HARNESS_MACROS_H

#include "test_harness_base.h"

#if KOKKOS_ENABLED_CHECKPOINT
#include "Kokkos_Core.hpp"
#endif

#include <vector>
#include <memory>

namespace vt { namespace tests { namespace perf { namespace common {

/**
  * \brief  Helper macros for registering the test and generating main function.
  * Example usage:
  *
  * struct MyTestMsg : vt::Message {};
  * void TestHandler(MyTestMsg* msg) {}
  *
  * struct MyTest : PerfTestHarness {};
  *
  * VT_PERF_TEST(MyTest, my_test_name) {
  *   if (my_node_ == 0) {
  *     auto m = makeMessage<MyTestMsg>();
  *     theMsg()->sendMsg<MyTestMsg, TestHandler>(1, m);
  *   }
  * }
  *
  * VT_PERF_TEST_MAIN()
  */
struct PerfTestRegistry{
  static void AddTest(std::unique_ptr<TestHarnessBase>&& test) {
    tests_.push_back(std::move(test));
  }

  static std::vector<std::unique_ptr<TestHarnessBase>>&
  GetTests() {
    return tests_;
  }

  private:
  inline static std::vector<std::unique_ptr<TestHarnessBase>> tests_ = {};
};

#define VT_PERF_TEST(StructName, TestName)                                 \
  struct StructName##TestName : StructName {                               \
    StructName##TestName() {                                               \
      name_ = #TestName;                                                   \
    }                                                                      \
    void SetUp() override {                                                \
      StructName::SetUp();                                                 \
    }                                                                      \
    void TearDown() override {                                             \
      StructName::TearDown();                                              \
    }                                                                      \
    void TestFunc() override;                                              \
  };                                                                       \
                                                                           \
  static struct StructName##TestName##_registerer_t {                      \
    StructName##TestName##_registerer_t() {                                \
      PerfTestRegistry::AddTest(std::make_unique<StructName##TestName>()); \
    }                                                                      \
  } StructName##TestName##_registerer;                                     \
  void StructName##TestName::TestFunc()

inline void tryInitializeKokkos() {
#if KOKKOS_ENABLED_CHECKPOINT
  Kokkos::initialize();
#endif
}

inline void tryFinalizeKokkos() {
#if KOKKOS_ENABLED_CHECKPOINT
  Kokkos::finalize();
#endif
}

#define VT_PERF_TEST_MAIN()                                                  \
  int main(int argc, char** argv) {                                          \
    using namespace vt::tests::perf::common;                                 \
    MPI_Init(&argc, &argv);                                                  \
    tryInitializeKokkos();                                                   \
    int rank;                                                                \
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);                                    \
    auto& tests = PerfTestRegistry::GetTests();                              \
    for (uint32_t test_num = 0; test_num < tests.size(); ++test_num) {       \
      auto* test = dynamic_cast<PerfTestHarness*>(tests.at(test_num).get()); \
      test->Initialize(argc, argv);                                          \
      auto const num_runs = test->GetNumRuns();                              \
      StopWatch timer;                                                       \
      if (rank == 0) {                                                       \
        fmt::print(                                                          \
          "{}{}RUNNING TEST:{} {} (Number of runs = {}) ...\n",              \
          test_num > 0 ? "\n\n\n\n" : "", vt::debug::bold(),                 \
          vt::debug::reset(), vt::debug::reg(test->GetName()),               \
          vt::debug::reg(fmt::format("{}", num_runs)));                      \
      }                                                                      \
      for (uint32_t run_num = 1; run_num <= num_runs; ++run_num) {           \
        test->SetUp();                                                       \
                                                                             \
        timer.Start();                                                       \
        test->TestFunc();                                                    \
        PerfTestHarness::SpinScheduler();                                    \
                                                                             \
        if (test->ShouldOutputGlobalTimer()) {                               \
          test->AddResult({test->GetName(), timer.Stop()});                  \
        }                                                                    \
                                                                             \
        if (run_num == num_runs) {                                           \
          test->SyncResults();                                               \
        }                                                                    \
                                                                             \
        test->TearDown();                                                    \
      }                                                                      \
      test->DumpResults();                                                   \
    }                                                                        \
    tests.clear();                                                           \
    tryFinalizeKokkos();                                                     \
    MPI_Finalize();                                                          \
    return 0;                                                                \
  }

}}}} // namespace vt::tests::perf::common

#endif /*INCLUDED_PERF_COMMON_TEST_HARNESS_MACROS_H*/
