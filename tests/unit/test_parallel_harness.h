/*
//@HEADER
// *****************************************************************************
//
//                           test_parallel_harness.h
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

#if ! defined __VIRTUAL_TRANSPORT_TEST_PARALLEL_HARNESS__
#define __VIRTUAL_TRANSPORT_TEST_PARALLEL_HARNESS__

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
        test_argc, test_argv
      );
    }

    CollectiveOps::initialize(test_argc, test_argv, no_workers, true);

#   if vt_feature_cmake_test_trace_on
    vt::arguments::ArgConfig::vt_trace = true;
#   endif

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
