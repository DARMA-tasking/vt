/*
//@HEADER
// *****************************************************************************
//
//                                   main.cc
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

bool should_init_MPI(int& argc, char** argv) {
  if (argc < 2) {
    return false;
  }


  // Extra flag (--MPI_TEST or --NO_MPI_TEST) is added as the last argument
  std::string test_arg = argv[argc - 1];
  assert(
    (test_arg == "--MPI_TEST" or test_arg == "--NO_MPI_TEST") &&
    "Last argument should be either --MPI_TEST or --NO_MPI_TEST"
  );

  // 'Remove' the argument so we don't pass it further to vt
  --argc;

  return test_arg == "--MPI_TEST";
}

int main(int argc, char **argv) {

  /**
   * Initalize MPI (if needed) before GTEST so we can check for number of ranks
   * during GTEST code generation.
   */
  if (should_init_MPI(argc, argv)) {
    vt::tests::unit::mpi_singleton =
      std::make_unique<vt::tests::unit::MPISingletonMultiTest>(argc, argv);
  }

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
