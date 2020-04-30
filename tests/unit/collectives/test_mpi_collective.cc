/*
//@HEADER
// *****************************************************************************
//
//                           test_mpi_collective.cc
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
// Questions? Contact darma@sandia.go
//
// *****************************************************************************
//@HEADER
*/

#include <gtest/gtest.h>
#include <vt/transport.h>

#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

using TestMPICollective = TestParallelHarness;

TEST_F(TestMPICollective, test_mpi_collective_1) {
  bool done = false;

  theCollective()->makeCollectiveScope().mpiCollectiveAsync([&done]{
    auto comm = theContext()->getComm();
    MPI_Barrier(comm);
    done = true;
  });

  theTerm()->addAction([&done]{
    EXPECT_TRUE(done);
  });

  while (not vt::rt->isTerminated()) {
    runScheduler();
  }
}

TEST_F(TestMPICollective, test_mpi_collective_2) {
  int done = 0;

  vt::collective::CollectiveScope scope = theCollective()->makeCollectiveScope();

  // These three collective can execute in any order, but it will always be
  // consistent across all the nodes
  scope.mpiCollectiveAsync([&done]{
    auto comm = theContext()->getComm();
    vt_print(barrier, "run MPI_Barrier\n");
    MPI_Barrier(comm);
    done++;
  });

  auto this_node = theContext()->getNode();
  int root = 0;
  int bcast_val = this_node == root ? 29 : 0;

  scope.mpiCollectiveAsync([&done,&bcast_val,root]{
    auto comm = theContext()->getComm();
    vt_print(barrier, "run MPI_Bcast\n");
    MPI_Bcast(&bcast_val, 1, MPI_INT, root, comm);
    done++;
  });

  int reduce_val_out = 0;

  scope.mpiCollectiveAsync([&done,&reduce_val_out]{
    auto comm = theContext()->getComm();
    int val_in = 1;
    vt_print(barrier, "run MPI_Allreduce\n");
    MPI_Allreduce(&val_in, &reduce_val_out, 1, MPI_INT, MPI_SUM, comm);
    done++;
  });

  theTerm()->addAction([&done,&bcast_val,&reduce_val_out]{
    auto num_nodes = theContext()->getNumNodes();
    EXPECT_EQ(done, 3);
    EXPECT_EQ(bcast_val, 29);
    EXPECT_EQ(reduce_val_out, num_nodes);
  });

  while (not vt::rt->isTerminated()) {
    runScheduler();
  }
}

TEST_F(TestMPICollective, test_mpi_collective_3) {
  int done = 0;

  auto this_node = theContext()->getNode();
  int root = 0;
  int bcast_val = this_node == root ? 29 : 0;

  vt::collective::CollectiveScope scope = theCollective()->makeCollectiveScope();

  auto tag = scope.mpiCollectiveAsync([&done,&bcast_val,root]{
    auto comm = theContext()->getComm();
    vt_print(barrier, "run MPI_Bcast\n");
    MPI_Bcast(&bcast_val, 1, MPI_INT, root, comm);
    done++;
  });

  scope.waitCollective(tag);

  EXPECT_EQ(done, 1);
  EXPECT_EQ(bcast_val, 29);

  int reduce_val_out = 0;

  scope.mpiCollectiveWait([&done,&reduce_val_out]{
    auto comm = theContext()->getComm();
    int val_in = 1;
    vt_print(barrier, "run MPI_Allreduce\n");
    MPI_Allreduce(&val_in, &reduce_val_out, 1, MPI_INT, MPI_SUM, comm);
    done++;
  });

  EXPECT_EQ(done, 2);
  EXPECT_EQ(reduce_val_out, theContext()->getNumNodes());
}

TEST_F(TestMPICollective, test_mpi_collective_4) {
  int done = 0;

  auto this_node = theContext()->getNode();
  bool is_even = this_node % 2 == 0;

  // System scope (will have generated tag=1)
  vt::collective::CollectiveScope scope1 = theCollective()->makeCollectiveScope();
  // System scope (will have generated tag=2)
  vt::collective::CollectiveScope scope2 = theCollective()->makeCollectiveScope();
  // User scope with tag=1
  vt::collective::CollectiveScope scope3 = theCollective()->makeCollectiveScope(1);

  int root = 0;
  int bcast_val = this_node == root ? 29 : 0;
  int reduce_val_out = 0;

  auto op1 = [&]{
    scope1.mpiCollectiveAsync([&done,&bcast_val,root]{
      auto comm = theContext()->getComm();
      vt_print(barrier, "run MPI_Bcast\n");
      MPI_Bcast(&bcast_val, 1, MPI_INT, root, comm);
      done++;
    });
  };

  auto op2 = [&]{
    scope2.mpiCollectiveAsync([&done,&reduce_val_out]{
      auto comm = theContext()->getComm();
      int val_in = 1;
      vt_print(barrier, "run MPI_Allreduce\n");
      MPI_Allreduce(&val_in, &reduce_val_out, 1, MPI_INT, MPI_SUM, comm);
      done++;
    });
  };

  auto op3 = [&]{
    scope3.mpiCollectiveAsync([&done]{
      auto comm = theContext()->getComm();
      vt_print(barrier, "run MPI_barrier\n");
      MPI_Barrier(comm);
      done++;
    });
  };

  // Execute them in different orders
  if (is_even) {
    op1(); op2(); op3();
  } else {
    op2(); op3(); op1();
  }

  theTerm()->addAction([&done,&bcast_val,&reduce_val_out]{
    auto num_nodes = theContext()->getNumNodes();
    EXPECT_EQ(done, 3);
    EXPECT_EQ(bcast_val, 29);
    EXPECT_EQ(reduce_val_out, num_nodes);
  });

  while (not vt::rt->isTerminated()) {
    runScheduler();
  }
}

}}} // end namespace vt::tests::unit
