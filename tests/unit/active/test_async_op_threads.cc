/*
//@HEADER
// *****************************************************************************
//
//                           test_async_op_threads.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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
#include <mpi.h>
#include "test_parallel_harness.h"

#include <vt/messaging/async_op_mpi.h>
#include <vt/messaging/active.h>
#include <vt/objgroup/headers.h>
#include <vt/transport.h>

#include <gtest/gtest.h>

#if vt_check_enabled(fcontext)

namespace vt { namespace tests { namespace unit { namespace threads {

using TestAsyncOpThreads = TestParallelHarness;

static std::size_t stack_size_before_running_handler = 0;

struct MyCol : vt::Collection<MyCol, vt::Index1D> {

  using MyMsg = vt::CollectionMessage<MyCol>;

  void handler(MyMsg* msg) {
    auto const this_node = theContext()->getNode();
    auto const num_nodes = theContext()->getNumNodes();
    auto const to_node = (this_node + 1) % num_nodes;
    from_node_ = this_node - 1;
    if (from_node_ < 0) {
      from_node_ = num_nodes - 1;
    }

    // save a reference to the epoch stack
    auto& epoch_stack = theTerm()->getEpochStack();

    auto comm = theContext()->getComm();
    int const tag = 299999;

    MPI_Request req1;
    send_val_ = this_node;
    {
      VT_ALLOW_MPI_CALLS; // MPI_Isend
      MPI_Isend(&send_val_, 1, MPI_INT, to_node, tag, comm, &req1);
    }
    auto op1 = std::make_unique<messaging::AsyncOpMPI>(req1);

    MPI_Request req2;
    {
      VT_ALLOW_MPI_CALLS; // MPI_Irecv
      MPI_Irecv(&recv_val_, 1, MPI_INT, from_node_, tag, comm, &req2);
    }
    auto op2 = std::make_unique<messaging::AsyncOpMPI>(
      req2,
      [this]{
        done_ = true;
        // stack should be the size before running this method since we haven't
        // resumed the thread yet!
        EXPECT_EQ(theTerm()->getEpochStack().size(), stack_size_before_running_handler);
      }
    );

    auto cur_ep = theMsg()->getEpoch();
    // push twice
    theMsg()->pushEpoch(cur_ep);
    theMsg()->pushEpoch(cur_ep);

    auto const stack_size_after_push = epoch_stack.size();

    // Register these async operations to block the user-level thread until
    // completion of the MPI request; since these operations are enclosed in an
    // epoch, they should inhibit the current epoch from terminating before they
    // are done.
    vt_print(gen, "call blockOnAsyncOp(op1)\n");
    theMsg()->blockOnAsyncOp(std::move(op1));
    vt_print(gen, "done with op1\n");

    EXPECT_EQ(epoch_stack.size(), stack_size_after_push);

    vt_print(gen, "call blockOnAsyncOp(op2)\n");
    theMsg()->blockOnAsyncOp(std::move(op2));
    vt_print(gen, "done with op2\n");

    EXPECT_EQ(epoch_stack.size(), stack_size_after_push);

    check();

    // pop twice down to starting size
    theMsg()->popEpoch(cur_ep);
    theMsg()->popEpoch(cur_ep);
  }

  void handlerInvoke(MyMsg* msg) {
    auto const this_node = theContext()->getNode();
    auto const num_nodes = theContext()->getNumNodes();
    auto const to_node = (this_node + 1) % num_nodes;
    from_node_ = this_node - 1;
    if (from_node_ < 0) {
      from_node_ = num_nodes - 1;
    }

    auto comm = theContext()->getComm();
    int const tag = 299999;

    MPI_Request req1;
    send_val_ = this_node;
    {
      VT_ALLOW_MPI_CALLS; // MPI_Isend
      MPI_Issend(&send_val_, 1, MPI_INT, to_node, tag, comm, &req1);
    }
    auto op1 = std::make_unique<messaging::AsyncOpMPI>(req1);

    MPI_Request req2;
    {
      VT_ALLOW_MPI_CALLS; // MPI_Irecv
      MPI_Irecv(&recv_val_, 1, MPI_INT, from_node_, tag, comm, &req2);
    }
    auto op2 = std::make_unique<messaging::AsyncOpMPI>(
      req2,
      [this]{ done_ = true; }
    );

    auto p = getCollectionProxy();
    p[this_node].invoke<&MyCol::handlerToInvoke>(std::move(op1),std::move(op2));
  }

  void handlerToInvoke(
    std::unique_ptr<messaging::AsyncOpMPI> op1,
    std::unique_ptr<messaging::AsyncOpMPI> op2
  ) {

    vt_print(gen, "call blockOnAsyncOp(op1) inside invoke\n");
    theMsg()->blockOnAsyncOp(std::move(op1));
    vt_print(gen, "done with op1 inside invoke\n");

    vt_print(gen, "call blockOnAsyncOp(op2)  inside invoke\n");
    theMsg()->blockOnAsyncOp(std::move(op2));
    vt_print(gen, "done with op2 inside invoke\n");

    check();
  }


  void check() {
    vt_print(gen, "running check method\n");
    EXPECT_EQ(from_node_, recv_val_);
    EXPECT_TRUE(done_);
    check_done_ = true;
  }

  int send_val_ = 0;
  int recv_val_ = -1000;
  bool done_ = false;
  NodeType from_node_ = 0;
  bool check_done_ = false;
};

TEST_F(TestAsyncOpThreads, test_async_op_threads_1) {
  auto const this_node = theContext()->getNode();

  stack_size_before_running_handler = theTerm()->getEpochStack().size();

  vt::Index1D range(static_cast<int>(theContext()->getNumNodes()));
  auto p = vt::makeCollection<MyCol>("test_async_op_threads_invoke")
    .bounds(range)
    .bulkInsert()
    .wait();

  auto ep = theTerm()->makeEpochRooted(term::UseDS{true});

  // When this returns all the MPI requests should be done
  runInEpoch(ep, [p, this_node]{
    p[this_node].send<typename MyCol::MyMsg, &MyCol::handler>();
  });

  // Ensure the check method actually ran.
  EXPECT_TRUE(p[this_node].tryGetLocalPtr()->check_done_);
}

TEST_F(TestAsyncOpThreads, test_async_op_threads_invoke_2) {
  auto const this_node = theContext()->getNode();

  vt::Index1D range(static_cast<int>(theContext()->getNumNodes()));
  auto p = vt::makeCollection<MyCol>("test_async_op_threads_invoke")
    .bounds(range)
    .bulkInsert()
    .wait();

  auto ep = theTerm()->makeEpochRooted(term::UseDS{true});

  // When this returns all the MPI requests should be done
  runInEpoch(ep, [p, this_node]{
    p[this_node].send<typename MyCol::MyMsg, &MyCol::handlerInvoke>();
  });

  // Ensure the check method actually ran.
  EXPECT_TRUE(p[this_node].tryGetLocalPtr()->check_done_);
}


}}}} // end namespace vt::tests::unit::threads

#endif /*vt_check_enabled(fcontext)*/
