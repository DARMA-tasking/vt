
#include <cstring>
#include <memory>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"
#include "test_parallel_harness.h"

#include "vt/transport.h"

namespace vt { namespace tests { namespace unit {

struct TestEpoch      : TestHarness                       { };
struct TestEpochParam : TestHarnessParam<::vt::EpochType> { };

TEST_F(TestEpoch, basic_test_first_epoch_unrooted_1) {
  auto const epoch        = epoch::first_epoch;
  auto const is_rooted    = epoch::EpochManip::isRooted(epoch);
  auto const is_user      = epoch::EpochManip::isUser(epoch);
  auto const has_category = epoch::EpochManip::hasCategory(epoch);
  auto const get_seq      = epoch::EpochManip::seq(epoch);
  auto const ep_node      = epoch::EpochManip::node(epoch);
  auto const next         = epoch::EpochManip::next(epoch);
  auto const next_seq     = epoch::EpochManip::seq(next);

  EXPECT_TRUE(!is_rooted);
  EXPECT_TRUE(!is_user);
  EXPECT_TRUE(!has_category);
  EXPECT_EQ(get_seq, 1);
  EXPECT_EQ(ep_node, 0);
  EXPECT_EQ(next, 2);
  EXPECT_EQ(next_seq, 2);
}

TEST_P(TestEpochParam, basic_test_epoch_unrooted_1) {
  EpochType const start_seq  = GetParam();
  auto const epoch           = epoch::EpochManip::makeEpoch(start_seq, false);
  auto const is_rooted       = epoch::EpochManip::isRooted(epoch);
  auto const is_user         = epoch::EpochManip::isUser(epoch);
  auto const has_category    = epoch::EpochManip::hasCategory(epoch);
  auto const get_seq         = epoch::EpochManip::seq(epoch);
  auto const ep_node         = epoch::EpochManip::node(epoch);
  auto const next            = epoch::EpochManip::next(epoch);
  auto const next_seq        = epoch::EpochManip::seq(next);

  EXPECT_TRUE(!is_rooted);
  EXPECT_TRUE(!is_user);
  EXPECT_TRUE(!has_category);
  EXPECT_EQ(get_seq, start_seq);
  EXPECT_EQ(ep_node, 0);
  EXPECT_EQ(next_seq, start_seq + 1);
}

TEST_P(TestEpochParam, basic_test_epoch_rooted_1) {
  auto const& n              = 48;
  EpochType const start_seq  = GetParam();
  auto const epoch           = epoch::EpochManip::makeEpoch(start_seq, true, n);
  auto const is_rooted       = epoch::EpochManip::isRooted(epoch);
  auto const is_user         = epoch::EpochManip::isUser(epoch);
  auto const has_category    = epoch::EpochManip::hasCategory(epoch);
  auto const get_seq         = epoch::EpochManip::seq(epoch);
  auto const ep_node         = epoch::EpochManip::node(epoch);
  auto const next            = epoch::EpochManip::next(epoch);
  auto const next_seq        = epoch::EpochManip::seq(next);

  EXPECT_TRUE(is_rooted);
  EXPECT_TRUE(!is_user);
  EXPECT_TRUE(!has_category);
  EXPECT_EQ(get_seq, start_seq);
  EXPECT_EQ(ep_node, n);
  EXPECT_EQ(next_seq, start_seq + 1);
}

TEST_P(TestEpochParam, basic_test_epoch_user_1) {
  EpochType const start_seq  = GetParam();
  auto const epoch           = epoch::EpochManip::makeEpoch(
    start_seq, false, uninitialized_destination, true
  );
  auto const is_rooted       = epoch::EpochManip::isRooted(epoch);
  auto const is_user         = epoch::EpochManip::isUser(epoch);
  auto const has_category    = epoch::EpochManip::hasCategory(epoch);
  auto const get_seq         = epoch::EpochManip::seq(epoch);
  auto const ep_node         = epoch::EpochManip::node(epoch);
  auto const next            = epoch::EpochManip::next(epoch);
  auto const next_seq        = epoch::EpochManip::seq(next);

  EXPECT_TRUE(!is_rooted);
  EXPECT_TRUE(is_user);
  EXPECT_TRUE(!has_category);
  EXPECT_EQ(get_seq, start_seq);
  EXPECT_EQ(ep_node, 0);
  EXPECT_EQ(next_seq, start_seq + 1);
}

TEST_P(TestEpochParam, basic_test_epoch_category_1) {
  EpochType const start_seq  = GetParam();
  auto const epoch           = epoch::EpochManip::makeEpoch(
    start_seq, false, uninitialized_destination, false,
    epoch::eEpochCategory::InsertEpoch
  );
  auto const is_rooted       = epoch::EpochManip::isRooted(epoch);
  auto const is_user         = epoch::EpochManip::isUser(epoch);
  auto const has_category    = epoch::EpochManip::hasCategory(epoch);
  auto const get_seq         = epoch::EpochManip::seq(epoch);
  auto const ep_node         = epoch::EpochManip::node(epoch);
  auto const cat             = epoch::EpochManip::category(epoch);
  auto const next            = epoch::EpochManip::next(epoch);
  auto const next_seq        = epoch::EpochManip::seq(next);

  EXPECT_TRUE(!is_rooted);
  EXPECT_TRUE(!is_user);
  EXPECT_TRUE(has_category);
  EXPECT_EQ(get_seq, start_seq);
  EXPECT_EQ(ep_node, 0);
  EXPECT_EQ(next_seq, start_seq + 1);
  EXPECT_EQ(cat, epoch::eEpochCategory::InsertEpoch);
}

TEST_P(TestEpochParam, basic_test_epoch_all_1) {
  auto const& n              = 48;
  EpochType const start_seq  = GetParam();
  auto const epoch           = epoch::EpochManip::makeEpoch(
    start_seq, true, n, true, epoch::eEpochCategory::InsertEpoch
  );
  auto const is_rooted       = epoch::EpochManip::isRooted(epoch);
  auto const is_user         = epoch::EpochManip::isUser(epoch);
  auto const has_category    = epoch::EpochManip::hasCategory(epoch);
  auto const get_seq         = epoch::EpochManip::seq(epoch);
  auto const ep_node         = epoch::EpochManip::node(epoch);
  auto const cat             = epoch::EpochManip::category(epoch);
  auto const next            = epoch::EpochManip::next(epoch);
  auto const next_seq        = epoch::EpochManip::seq(next);

  EXPECT_TRUE(is_rooted);
  EXPECT_TRUE(is_user);
  EXPECT_TRUE(has_category);
  EXPECT_EQ(get_seq, start_seq);
  EXPECT_EQ(ep_node, n);
  EXPECT_EQ(next_seq, start_seq + 1);
  EXPECT_EQ(cat, epoch::eEpochCategory::InsertEpoch);
}

INSTANTIATE_TEST_CASE_P(
  InstantiationName, TestEpochParam,
  ::testing::Range(static_cast<EpochType>(1), static_cast<EpochType>(100), 10)
);

}}} // end namespace vt::tests::unit
