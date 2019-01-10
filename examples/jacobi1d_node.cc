/*
//@HEADER
// ************************************************************************
//
//                          jacobi1d_node.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "vt/transport.h"

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>
#include <string>

#define DEBUG_JACOBI 0

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static RDMA_HandleType jac_t1_han = no_rdma_handle;
static RDMA_HandleType jac_t2_han = no_rdma_handle;
static RDMA_HandleType jac_resid = no_rdma_handle;

using ActionBoolType = std::function<void(bool)>;
using JacobiSizeType = int64_t;

static JacobiSizeType blk_size = -1;
static double* t1 = nullptr;
static double* t2 = nullptr;
static constexpr double bound_l_val[1] = { 1.0 };
static constexpr double bound_r_val[1] = { -1.0 };
static double* local_resid_buf = nullptr;
static double resid_val = 0.0;
static double max_resid_val = 0.0;

static JacobiSizeType total_size = 1024;
static JacobiSizeType max_iterations = 128;
static constexpr double const error_tolerance = 0.01;

static int cur_iter = 0;
static int wait_count = 0;

struct StartWorkMsg : ShortMessage {
  bool const converged;

  StartWorkMsg(bool const& in_conv) : converged(in_conv) { }
};

struct JacobiKernelMsg : ShortMessage {
  int iter = 0;

  JacobiKernelMsg(int const& in_iter) : iter(in_iter) { }
};

using FinishedMsg = ShortMessage;

static void startJacobi1dHandler(StartWorkMsg* msg);

static double kernel(int const iter) {
  double* const c1 = iter % 2 == 0 ? t1 : t2;
  double* const c2 = iter % 2 != 0 ? t1 : t2;

  double diff = 0.0, local_error = 0.0;
  for (int i = 1; i < blk_size + 1; i++) {
    c1[i] = 0.3333 * (c2[i-1] + c2[i] + c2[i+1]);

    diff = c2[i] - c1[i];
    if (diff < 0.0) {
      diff *= -1.0;
    }
    local_error = local_error > diff ? local_error : diff;
  }

  return local_error;
}

static void calcResid(ActionBoolType action) {
  if (resid_val < error_tolerance) {
    // obviously very unscalable, but just for the sake of testing
    theRDMA()->getTypedDataInfoBuf(
      jac_resid, local_resid_buf, num_nodes, 0, no_tag, [=]{
        bool found_greater = false;
        max_resid_val = 0.0;
        for (int i = 0; i < num_nodes; i++) {
          #if DEBUG_JACOBI
          fmt::print("{}: resid:i={}:val={}:iter={}\n", my_node, i, local_resid_buf[i], cur_iter);
          #endif
          max_resid_val = std::max(max_resid_val, local_resid_buf[i]);
          if (local_resid_buf[i] > error_tolerance) {
            found_greater = true;
          }
        }
        action(!found_greater);
      }
    );
  } else {
    action(false);
  }
}

static void finished() {
  fmt::print("{}: finished computation: error={}, iter={}\n", my_node, resid_val, cur_iter);
}

static void boundaryFinished(bool const is_left) {
  wait_count--;

  #if DEBUG_JACOBI
  fmt::print(
    "{}: finished get {}: wait_count={}\n", my_node, (is_left ? "left" : "right"),
    wait_count
  );
  #endif

  if (wait_count == 0) {
    resid_val = kernel(cur_iter);

    theRDMA()->putTypedData(jac_resid, &resid_val, 1, my_node, no_tag, no_action, []{
      theCollective()->barrierThen([]{
        if (cur_iter >= max_iterations) {
          finished();
        } else {
          cur_iter++;

          if (my_node == 0) {
            calcResid([](bool has_converged){
              theMsg()->broadcastMsg<StartWorkMsg, startJacobi1dHandler>(
                makeSharedMessage<StartWorkMsg>(has_converged)
              );

              auto msg = std::make_unique<StartWorkMsg>(has_converged);
              startJacobi1dHandler(msg.get());
            });
          }
        }
      });
    });
  }
}

static void doKernel(JacobiKernelMsg* msg) {
  auto const& iter = msg->iter;

  double* const c1 = msg->iter % 2 == 0 ? t1 : t2;
  double* const c2 = msg->iter % 2 != 0 ? t1 : t2;

  auto const& l_bound = (blk_size * my_node) - 1;
  auto const& r_bound = ((blk_size + 1) * my_node) + 1;

  auto const is_far_left = my_node == 0;
  auto const is_far_right = my_node == num_nodes - 1;

  wait_count = (is_far_left ? 0 : 1) + (is_far_right ? 0 : 1);

  #if DEBUG_JACOBI
  fmt::print(
    "{}: doKernel: iter={}, c1={}, c2={}, l_bound={}, r_bound={}, wait_count={}\n",
    my_node, iter, c1, c2, l_bound, r_bound, wait_count
  );
  #endif

  if (wait_count == 0) {
    kernel(iter);
  } else {
    if (my_node != 0) {
      #if DEBUG_JACOBI
      fmt::print("{}: doKernel: iter={}, get left wait_count={}\n", my_node, iter, wait_count);
      #endif
      auto fn = std::bind(boundaryFinished, true);
      theRDMA()->getTypedDataInfoBuf(jac_t1_han, c1, 1, l_bound, no_tag, fn);
    }

    if (my_node != num_nodes - 1) {
      #if DEBUG_JACOBI
      fmt::print("{}: doKernel: iter={}, get right wait_count={}\n", my_node, iter, wait_count);
      #endif
      auto fn = std::bind(boundaryFinished, false);
      theRDMA()->getTypedDataInfoBuf(jac_t1_han, c1, 1, r_bound, no_tag, fn);
    }
  }
}

static void startJacobi1dHandler(StartWorkMsg* msg) {
  bool const& converged = msg->converged;

  #if DEBUG_JACOBI
  fmt::print(
    "{}: startJacobi1dHandler: cur_iter={}: converged={}\n",
    my_node, cur_iter, print_bool(converged)
  );
  #endif

  if (my_node == 0) {
    fmt::print("iter={}: starting: max_residual={}\n", cur_iter, max_resid_val);
  }

  if (cur_iter == 0) {
    if (my_node == 0) {
      t1[0] = t2[0] = bound_l_val[0];
    }
    if (my_node == num_nodes - 1) {
      t1[blk_size + 1] = t2[blk_size + 1] = bound_r_val[0];
    }
  }

  theCollective()->barrierThen([=]{
    #if DEBUG_JACOBI
    fmt::print(
      "{}: jacobi1d: barrierThen next iter={}: converged={}\n",
        my_node, cur_iter, print_bool(converged)
    );
    #endif

    if (not converged) {
      #if DEBUG_JACOBI
      fmt::print("{}: jacobi1d: next iter={}\n", my_node, cur_iter);
      #endif

      if (my_node == 0) {
        theMsg()->broadcastMsg<JacobiKernelMsg, doKernel>(
          makeSharedMessage<JacobiKernelMsg>(cur_iter)
        );
        JacobiKernelMsg msg(cur_iter);
        doKernel(&msg);
      }
    } else {
      finished();
    }
  });
}

static int exitEarly(
  NodeType const node, int const exit_code, std::string const reason
) {
  if (node == 0) {
    CollectiveOps::output(std::string{reason});
    CollectiveOps::finalize();
    return 0;
  }

  CollectiveOps::finalize();
  return exit_code;
}

#define sstmac_app_name jacobi1d_node_vt

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  std::string name(argv[0]);

  if (argc == 1) {
    total_size = 1024;
    max_iterations = 64;
    ::fmt::print(
      stderr, "{}: using default arguments since none provided\n", name
    );
  } else {
    if (argc != 3) {
      std::string const buf = fmt::format(
        "usage: {} <total-num-elements> <max-iterations>", name
      );
      return exitEarly(my_node, 1, buf);
    }

    total_size = atoi(argv[1]);
    max_iterations = atoi(argv[2]);
  }

  if (num_nodes == 1) {
    std::string const buf = fmt::format(
      "Need >= 2 ranks:\n mpirun-mpich-clang -n 2 {}\0", name
    );
    return exitEarly(my_node, 1, buf);
  }

  blk_size = total_size / num_nodes;

  vtAssertExpr(blk_size * num_nodes == total_size);
  vtAssertExpr(blk_size * num_nodes == total_size);

  t1 = new double[blk_size + 2];
  t2 = new double[blk_size + 2];
  local_resid_buf = new double[num_nodes];

  for (int i = 0; i < blk_size + 2; i++) {
    t1[i] = 0.0;
    t2[i] = 0.0;
  }

  fmt::print("{}: total_size={}, blk_size={}\n", my_node, total_size, blk_size);

  jac_t1_han = theRDMA()->registerCollectiveTyped(t1 + 1, blk_size, total_size);
  jac_t2_han = theRDMA()->registerCollectiveTyped(t2 + 1, blk_size, total_size);
  jac_resid = theRDMA()->registerCollectiveTyped(&resid_val, 1, num_nodes);

  theCollective()->barrierThen([]{
    if (my_node == 0) {
      theMsg()->broadcastMsg<StartWorkMsg, startJacobi1dHandler>(
        makeSharedMessage<StartWorkMsg>(false)
      );
      auto msg = std::make_unique<StartWorkMsg>(false);
      startJacobi1dHandler(msg.get());
    }
  });

  while (!rt->isTerminated()) {
    runScheduler();
  }

  delete [] t1;
  delete [] t2;
  delete [] local_resid_buf;

  CollectiveOps::finalize();

  return 0;
}
