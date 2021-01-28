/*
//@HEADER
// *****************************************************************************
//
//                              test_async_op.cc
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

#include <vt/transport.h>
#include <vt/runtime/mpi_access.h>

#include <gtest/gtest.h>
#include "test_parallel_harness.h"

namespace vt { namespace tests { namespace unit {

using TestAsyncOp = TestParallelHarness;

struct AsyncOpMPI : messaging::AsyncOp {
  AsyncOpMPI() = default;

  bool poll() override {
    VT_ALLOW_MPI_CALLS; // MPI_Test
    int flag = 0;
    MPI_Status stat;
    MPI_Test(&req_, &flag, &stat);
    return flag;
  }

  void done() override {
    if (trigger_) {
      trigger_();
    }
  }

  void setRequest(MPI_Request in_req) { req_ = in_req; }
  void setTrigger(ActionType in_trigger) { trigger_ = in_trigger; }

public:
  int data_ = 0;

private:
  MPI_Request req_ = MPI_REQUEST_NULL;
  ActionType trigger_ = nullptr;
};

using MyMsg = Message;

struct MyObjGroup {

  void handler(MyMsg* msg) {
    auto const this_node = theContext()->getNode();
    auto const num_nodes = theContext()->getNumNodes();
    auto const to_node = (this_node + 1) % num_nodes;
    auto from_node = this_node - 1;
    if (from_node < 0) {
      from_node = num_nodes - 1;
    }

    auto comm = theContext()->getComm();
    int const tag = 299999;

    MPI_Request req1;
    auto op1 = std::make_unique<AsyncOpMPI>();
    op1->data_ = this_node;
    {
      VT_ALLOW_MPI_CALLS; // MPI_Isend
      MPI_Isend(&op1->data_, 1, MPI_INT, to_node, tag, comm, &req1);
    }
    op1->setRequest(req1);

    MPI_Request req2;
    auto op2 = std::make_unique<AsyncOpMPI>();
    auto trigger = [data = &op2->data_, from_node, this]{
      EXPECT_EQ(*data, from_node);
      done = true;
    };
    op2->setTrigger(trigger);
    {
      VT_ALLOW_MPI_CALLS; // MPI_Irecv
      MPI_Irecv(&op2->data_, 1, MPI_INT, from_node, tag, comm, &req2);
    }
    op2->setRequest(req2);

    // Register these async operations for polling; since these operations are
    // enclosed in an epoch, they should inhibit the current epoch from
    // terminating before they are done.
    theMsg()->registerAsyncOp(std::move(op1));
    theMsg()->registerAsyncOp(std::move(op2));
  }

  bool done = false;
};

TEST_F(TestAsyncOp, test_async_op_1) {
  auto const this_node = theContext()->getNode();
  auto p = theObjGroup()->makeCollective<MyObjGroup>();
  auto ep = theTerm()->makeEpochRooted(term::UseDS{true});
  // When this returns all the MPI requests should be done
  runInEpoch(ep, [p, this_node]{
    p[this_node].send<MyMsg, &MyObjGroup::handler>();
  });
  // Assert that async MPI ops are done after return
  EXPECT_TRUE(p[this_node].get()->done);
}

}}} // end namespace vt::tests::unit
