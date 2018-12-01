
#include <vector>
#include <memory>

#include <gtest/gtest.h>

#include "test_harness.h"
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

std::unique_ptr<MPISingletonMultiTest> mpi_singleton = nullptr;

int test_argc = 0;
char** test_argv = nullptr;


}}} // end namespace vt::tests::unit

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);

  vt::tests::unit::test_argc = argc;
  vt::tests::unit::test_argv = argv;
  vt::tests::unit::TestHarness::store_cmdline_args(argc, argv);

  int const ret = RUN_ALL_TESTS();

  if (vt::tests::unit::mpi_singleton) {
    vt::tests::unit::mpi_singleton = nullptr;
  }

  return ret;
}
