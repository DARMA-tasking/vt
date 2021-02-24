/*
//@HEADER
// *****************************************************************************
//
//                                test_epoch.cc
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

#include <cstring>
#include <memory>

#include <gtest/gtest.h>

#include "test_harness.h"
#include "test_parallel_harness.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

struct TestEpoch      : TestHarness                       { };
struct TestEpochParam : TestHarnessParam<::vt::EpochType> { };
struct TestEpochBits  : TestHarness                       { };

TEST_F(TestEpoch, basic_test_first_epoch_collective_1) {
  auto const epoch        = epoch::first_epoch;
  auto const is_rooted    = epoch::EpochManip::isRooted(epoch);
  auto const get_seq      = epoch::EpochManip::seq(epoch);
  auto const scope        = epoch::EpochManip::getScope(epoch);

  EXPECT_TRUE(!is_rooted);
  EXPECT_EQ(get_seq, 1U);
  EXPECT_EQ(scope, epoch::global_epoch_scope);
}

TEST_P(TestEpochParam, basic_test_epoch_collective_1) {
  EpochType const start_seq  = GetParam();
  auto epoch                 = epoch::EpochManip::generateEpoch(false);
  epoch::EpochManip::setSeq(epoch, start_seq);
  auto const is_rooted       = epoch::EpochManip::isRooted(epoch);
  auto const get_seq         = epoch::EpochManip::seq(epoch);
  auto const scope           = epoch::EpochManip::getScope(epoch);

  EXPECT_TRUE(!is_rooted);
  EXPECT_EQ(get_seq, start_seq);
  EXPECT_EQ(scope, epoch::global_epoch_scope);
}

TEST_P(TestEpochParam, basic_test_epoch_rooted_1) {
  auto const& n              = 48;
  EpochType const start_seq  = GetParam();
  epoch::EpochScopeType in_scope = 3;
  auto epoch                 = epoch::EpochManip::generateEpoch(
    true, n, in_scope
  );
  epoch::EpochManip::setSeq(epoch, start_seq);
  auto const is_rooted       = epoch::EpochManip::isRooted(epoch);
  auto const get_seq         = epoch::EpochManip::seq(epoch);
  auto const ep_node         = epoch::EpochManip::node(epoch);
  auto const scope           = epoch::EpochManip::getScope(epoch);

  EXPECT_TRUE(is_rooted);
  EXPECT_EQ(get_seq, start_seq);
  EXPECT_EQ(ep_node, n);
  EXPECT_EQ(scope, in_scope);
}

TEST_P(TestEpochParam, basic_test_epoch_scope_1) {
  EpochType const start_seq  = GetParam();
  epoch::EpochScopeType in_scope = 1;
  auto epoch                 = epoch::EpochManip::generateEpoch(
    false, uninitialized_destination, in_scope
  );
  epoch::EpochManip::setSeq(epoch, start_seq);
  auto const is_rooted       = epoch::EpochManip::isRooted(epoch);
  auto const get_seq         = epoch::EpochManip::seq(epoch);
  auto const scope           = epoch::EpochManip::getScope(epoch);

  EXPECT_TRUE(!is_rooted);
  EXPECT_EQ(get_seq, start_seq);
  EXPECT_EQ(scope, in_scope);
}

TEST_P(TestEpochParam, basic_test_epoch_category_1) {
  EpochType const start_seq  = GetParam();
  epoch::EpochScopeType in_scope = 2;
  auto epoch                 = epoch::EpochManip::generateEpoch(
    false, uninitialized_destination, in_scope,
    epoch::eEpochCategory::InsertEpoch
  );
  epoch::EpochManip::setSeq(epoch, start_seq);
  auto const is_rooted       = epoch::EpochManip::isRooted(epoch);
  auto const get_seq         = epoch::EpochManip::seq(epoch);
  auto const cat             = epoch::EpochManip::category(epoch);
  auto const scope           = epoch::EpochManip::getScope(epoch);

  EXPECT_TRUE(!is_rooted);
  EXPECT_EQ(get_seq, start_seq);
  EXPECT_EQ(cat, epoch::eEpochCategory::InsertEpoch);
  EXPECT_EQ(scope, in_scope);
}

TEST_P(TestEpochParam, basic_test_epoch_all_1) {
  auto const& n              = 48;
  epoch::EpochScopeType in_scope = 1;
  EpochType const start_seq  = GetParam();
  auto epoch                 = epoch::EpochManip::generateEpoch(
    true, n, in_scope, epoch::eEpochCategory::InsertEpoch
  );
  epoch::EpochManip::setSeq(epoch, start_seq);
  auto const is_rooted       = epoch::EpochManip::isRooted(epoch);
  auto const get_seq         = epoch::EpochManip::seq(epoch);
  auto const ep_node         = epoch::EpochManip::node(epoch);
  auto const cat             = epoch::EpochManip::category(epoch);
  auto const scope           = epoch::EpochManip::getScope(epoch);

  EXPECT_TRUE(is_rooted);
  EXPECT_EQ(get_seq, start_seq);
  EXPECT_EQ(ep_node, n);
  EXPECT_EQ(cat, epoch::eEpochCategory::InsertEpoch);
  EXPECT_EQ(scope, in_scope);
}

INSTANTIATE_TEST_SUITE_P(
  InstantiationName, TestEpochParam,
  ::testing::Range(static_cast<EpochType>(1), static_cast<EpochType>(100), 10)
);

TEST_F(TestEpochBits, test_epoch_bit_assignment) {
  EXPECT_EQ(vt::epoch::eEpochColl::cEpochIsRooted, 63);
  EXPECT_EQ(vt::epoch::eEpochRoot::rEpochIsRooted, 63);
}

}}} // end namespace vt::tests::unit
