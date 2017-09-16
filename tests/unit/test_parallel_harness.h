
#if ! defined __VIRTUAL_TRANSPORT_TEST_PARALLEL_HARNESS__
#define __VIRTUAL_TRANSPORT_TEST_PARALLEL_HARNESS__

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <vector>
#include <string>

#include "test_config.h"
#include "test_harness.h"

#include "transport.h"

struct TestParallelHarness : TestHarness {
  bool finished = false;

  virtual void SetUp() {
    using namespace vt;

    TestHarness::SetUp();

    CollectiveOps::initializeContext(0, nullptr);
    CollectiveOps::initializeRuntime();

    auto const& my_node = theContext->getNode();
    auto const& num_nodes = theContext->getNumNodes();

    #if DEBUG_TEST_HARNESS_PRINT
      printf("my_node=%d, num_nodes=%d\n", my_node, num_nodes);
    #endif

    finished = false;
  }

  virtual void TearDown() {
    using namespace vt;

    while (not finished) {
      runScheduler();
    }

    TestHarness::TearDown();
  }
};

#endif /* __VIRTUAL_TRANSPORT_TEST_PARALLEL_HARNESS__ */
