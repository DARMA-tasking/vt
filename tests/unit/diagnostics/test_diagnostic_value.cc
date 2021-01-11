/*
//@HEADER
// *****************************************************************************
//
//                          test_diagnostic_value.cc
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

#include <gtest/gtest.h>

#include <checkpoint/checkpoint.h>

#include "test_parallel_harness.h"

#include <vt/transport.h>

#if vt_check_enabled(diagnostics)

namespace vt { namespace tests { namespace unit {

using TestDiagnosticValue = TestParallelHarness;

struct TestDiagnostic : vt::runtime::component::Diagnostic {
  TestDiagnostic() {
    // choose a high ID as to not conflict with other components' automatically
    // assigned IDs at startup
    component_id_ = 10000;
  }
  virtual ~TestDiagnostic() = default;

  void dumpState() override {}
  std::string name() override { return "Test"; }
};

TEST_F(TestDiagnosticValue, test_diagnostic_value_1) {
  using vt::runtime::component::DiagnosticUpdate;
  using vt::runtime::component::DiagnosticUnit;
  using vt::runtime::component::DiagnosticTypeEnum;
  using vt::runtime::component::DiagnosticErasedValue;
  using vt::runtime::component::detail::DiagnosticValue;

  using ValueType = int32_t;

  ValueType init_value = 10;
  std::string test_key = "my-test-key";
  std::string test_desc = "my test description";
  ValueType snapshot = 0;
  ValueType cur_value = 0;

  DiagnosticValue<ValueType> val{
    test_key,
    test_desc,
    DiagnosticUpdate::Sum,
    DiagnosticUnit::Units,
    DiagnosticTypeEnum::PerformanceDiagnostic,
    init_value
  };

  for (int i = 0; i < 100; i++) {
    val.update(i);
    cur_value = init_value + (i*(i+1))/2;
    EXPECT_EQ(val.get(snapshot), cur_value);
  }

  auto diag = std::make_unique<TestDiagnostic>();

  DiagnosticErasedValue out;

  runInEpochCollective([&]{
    val.reduceOver(diag.get(), &out, snapshot);
  });

  auto num_nodes = theContext()->getNumNodes();
  auto this_node = theContext()->getNode();

  if (this_node == 0) {
    EXPECT_TRUE(out.min_.template is<ValueType>());
    EXPECT_TRUE(out.max_.template is<ValueType>());
    EXPECT_TRUE(out.sum_.template is<ValueType>());

    EXPECT_EQ(out.min_.template get<ValueType>(), cur_value);
    EXPECT_EQ(out.max_.template get<ValueType>(), cur_value);
    EXPECT_EQ(out.sum_.template get<ValueType>(), cur_value * num_nodes);
    EXPECT_DOUBLE_EQ(out.avg_, static_cast<double>(cur_value));
    EXPECT_DOUBLE_EQ(out.std_, 0.0);
    EXPECT_EQ(out.update_, DiagnosticUpdate::Sum);
    EXPECT_EQ(out.unit_, DiagnosticUnit::Units);
    EXPECT_TRUE(out.is_valid_value_);
  }
}

TEST_F(TestDiagnosticValue, test_diagnostic_value_2) {
  using vt::runtime::component::DiagnosticUpdate;
  using vt::runtime::component::DiagnosticUnit;
  using vt::runtime::component::DiagnosticTypeEnum;
  using vt::runtime::component::DiagnosticErasedValue;
  using vt::runtime::component::detail::DiagnosticValue;

  using ValueType = double;

  std::string test_key = "my-test-key";
  std::string test_desc = "my test description";
  ValueType snapshot = 0;

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  if (num_nodes != 2) {
    return;
  }

  double num_to_set = this_node == 0 ? 100 : 175;

  DiagnosticValue<ValueType> val{
    test_key,
    test_desc,
    DiagnosticUpdate::Avg,
    DiagnosticUnit::Units,
    DiagnosticTypeEnum::PerformanceDiagnostic,
    0
  };

  if (this_node < 2) {
    for (int i = 0; i < 100 * (this_node + 1); i++) {
      val.update(num_to_set);
      EXPECT_DOUBLE_EQ(val.get(snapshot), num_to_set);
    }

    // make sure that memory footprinting using base pointer works
    vt::runtime::component::detail::DiagnosticBase* base_ptr = &val;
    EXPECT_GT(checkpoint::getMemoryFootprint(*base_ptr), sizeof(*base_ptr));
  }

  auto diag = std::make_unique<TestDiagnostic>();

  DiagnosticErasedValue out;

  runInEpochCollective([&]{
    val.reduceOver(diag.get(), &out, snapshot);
  });

  if (this_node == 0) {
    EXPECT_TRUE(out.min_.template is<ValueType>());
    EXPECT_TRUE(out.max_.template is<ValueType>());
    EXPECT_TRUE(out.sum_.template is<ValueType>());

    EXPECT_DOUBLE_EQ(out.min_.template get<ValueType>(), 100);
    EXPECT_DOUBLE_EQ(out.max_.template get<ValueType>(), 175);
    EXPECT_DOUBLE_EQ(out.avg_, 150); // check properly weighted average
    EXPECT_EQ(out.update_, DiagnosticUpdate::Avg);
    EXPECT_EQ(out.unit_, DiagnosticUnit::Units);
    EXPECT_TRUE(out.is_valid_value_);
  }
}

}}} // end namespace vt::tests::unit

#endif /*vt_check_enabled(diagnostics)*/
