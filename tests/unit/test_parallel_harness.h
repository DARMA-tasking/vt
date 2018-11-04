
#if ! defined __VIRTUAL_TRANSPORT_TEST_PARALLEL_HARNESS__
#define __VIRTUAL_TRANSPORT_TEST_PARALLEL_HARNESS__

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <string>

#include "test_config.h"
#include "test_harness.h"

#include "vt/transport.h"

#include <mpi.h>

namespace vt { namespace tests { namespace unit {

static bool mpi_is_initialized = false;

template <typename TestBase>
struct TestParallelHarnessAny : TestHarnessAny<TestBase> {
  virtual void SetUp() {
    using namespace vt;

    TestHarnessAny<TestBase>::SetUp();

    if (not mpi_is_initialized) {
      MPI_Init(&this->argc_, &this->argv_);
      mpi_is_initialized = true;
    }

    CollectiveOps::initialize(this->argc_, this->argv_, no_workers, true);

    #if DEBUG_TEST_HARNESS_PRINT
      auto const& my_node = theContext()->getNode();
      auto const& num_nodes = theContext()->getNumNodes();
      fmt::print("my_node={}, num_nodes={}\n", my_node, num_nodes);
    #endif
  }

  virtual void TearDown() {
    using namespace vt;

    while (!rt->isTerminated()) {
      runScheduler();
    }

    #if DEBUG_TEST_HARNESS_PRINT
      auto const& my_node = theContext()->getNode();
      fmt::print("my_node={}, tearing down runtime\n", my_node);
    #endif

    CollectiveOps::finalize();

    TestHarnessAny<TestBase>::TearDown();
  }
};

using TestParallelHarness = TestParallelHarnessAny<testing::Test>;

template <typename ParamT>
using TestParallelHarnessParam = TestParallelHarnessAny<
  testing::TestWithParam<ParamT>
>;

using TestParameterHarnessNode = TestParallelHarnessParam<vt::NodeType>;

}}} // end namespace vt::tests::unit

#endif /* __VIRTUAL_TRANSPORT_TEST_PARALLEL_HARNESS__ */
