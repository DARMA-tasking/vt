
#if ! defined __VIRTUAL_TRANSPORT_TEST_PARALLEL_HARNESS__
#define __VIRTUAL_TRANSPORT_TEST_PARALLEL_HARNESS__

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <string>
#include <memory>

#include "test_config.h"
#include "test_harness.h"

#include "vt/transport.h"

#include <mpi.h>

namespace vt { namespace tests { namespace unit {

/*
 *  gtest runs many tests in the same binary, but there is no way to know when
 *  to call MPI_Finalize, which can only be called once (when it's called
 *  MPI_Init can't be called again)! This singleton uses static initialization
 *  to init/finalize exactly once
 */
struct MPISingletonMultiTest {
  MPISingletonMultiTest(int& argc, char**& argv) {
    MPI_Init(&argc, &argv);
    comm_ = MPI_COMM_WORLD;
    MPI_Barrier(comm_);
  }
  virtual ~MPISingletonMultiTest() {
    MPI_Barrier(comm_);
    MPI_Finalize();
  }

private:
  MPI_Comm comm_;
};

extern std::unique_ptr<MPISingletonMultiTest> mpi_singleton;

template <typename TestBase>
struct TestParallelHarnessAny : TestHarnessAny<TestBase> {
  virtual void SetUp() {
    using namespace vt;

    TestHarnessAny<TestBase>::SetUp();

    if (mpi_singleton == nullptr) {
      mpi_singleton = std::make_unique<MPISingletonMultiTest>(
        this->argc_, this->argv_
      );
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
