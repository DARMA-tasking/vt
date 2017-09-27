
#include <vector>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "test_harness.h"

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  ::testing::InitGoogleMock(&argc, argv);

  vt::tests::unit::TestHarness::store_cmdline_args(argc, argv);

  int const ret = RUN_ALL_TESTS();

  return ret;
}
