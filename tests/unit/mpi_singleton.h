/*
//@HEADER
// *****************************************************************************
//
//                               mpi_singleton.h
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

#if !defined INCLUDED_UNIT_MPI_SINGLETON_H
#define INCLUDED_UNIT_MPI_SINGLETON_H

#include <memory>
#include <mpi.h>

namespace vt { namespace tests { namespace unit {

extern int test_argc;
extern char** test_argv;

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

    MPI_Comm_size(comm_, &num_ranks_);
    MPI_Barrier(comm_);
  }

  virtual ~MPISingletonMultiTest() {
    MPI_Barrier(comm_);
    MPI_Finalize();
  }

public:
  static MPISingletonMultiTest* Get() {
    static std::unique_ptr<MPISingletonMultiTest> mpi_singleton = nullptr;

    if (!mpi_singleton) {
      mpi_singleton =
        std::make_unique<MPISingletonMultiTest>(test_argc, test_argv);
    }

    return mpi_singleton.get();
  }

  MPI_Comm getComm() { return comm_; }
  int getNumRanks() { return num_ranks_; }

private:
  MPI_Comm comm_;
  int num_ranks_;
};

}}} // namespace vt::tests::unit

#endif /* INCLUDED_UNIT_MPI_SINGLETON_H */
