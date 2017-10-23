
#include "transport.h"

#include <cstdlib>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <functional>
#include <memory>

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
          printf("%d: resid:i=%d:val=%f:iter=%d\n", my_node, i, local_resid_buf[i], cur_iter);
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
  printf("%d: finished computation: error=%f, iter=%d\n", my_node, resid_val, cur_iter);
}

static void boundaryFinished(bool const is_left) {
  wait_count--;

  #if DEBUG_JACOBI
  printf(
    "%d: finished get %s: wait_count=%d\n", my_node, (is_left ? "left" : "right"),
    wait_count
  );
  #endif

  if (wait_count == 0) {
    resid_val = kernel(cur_iter);

    theRDMA()->putTypedData(jac_resid, &resid_val, 1, my_node, no_tag, no_action, []{
      theBarrier()->barrierThen([]{
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
  printf(
    "%d: doKernel: iter=%d, c1=%p, c2=%p, l_bound=%lld, r_bound=%lld, wait_count=%d\n",
    my_node, iter, c1, c2, l_bound, r_bound, wait_count
  );
  #endif

  if (wait_count == 0) {
    kernel(iter);
  } else {
    if (my_node != 0) {
      #if DEBUG_JACOBI
      printf("%d: doKernel: iter=%d, get left wait_count=%d\n", my_node, iter, wait_count);
      #endif
      auto fn = std::bind(boundaryFinished, true);
      theRDMA()->getTypedDataInfoBuf(jac_t1_han, c1, 1, l_bound, no_tag, fn);
    }

    if (my_node != num_nodes - 1) {
      #if DEBUG_JACOBI
      printf("%d: doKernel: iter=%d, get right wait_count=%d\n", my_node, iter, wait_count);
      #endif
      auto fn = std::bind(boundaryFinished, false);
      theRDMA()->getTypedDataInfoBuf(jac_t1_han, c1, 1, r_bound, no_tag, fn);
    }
  }
}

static void startJacobi1dHandler(StartWorkMsg* msg) {
  bool const& converged = msg->converged;

  #if DEBUG_JACOBI
  printf(
    "%d: startJacobi1dHandler: cur_iter=%d: converged=%s\n",
    my_node, cur_iter, print_bool(converged)
  );
  #endif

  if (my_node == 0) {
    printf("iter=%d: starting: max_residual=%f\n", cur_iter, max_resid_val);
  }

  if (cur_iter == 0) {
    if (my_node == 0) {
      t1[0] = t2[0] = bound_l_val[0];
    }
    if (my_node == num_nodes - 1) {
      t1[blk_size + 1] = t2[blk_size + 1] = bound_r_val[0];
    }
  }

  theBarrier()->barrierThen([=]{
    #if DEBUG_JACOBI
    printf(
      "%d: jacobi1d: barrierThen next iter=%d: converged=%s\n",
        my_node, cur_iter, print_bool(converged)
    );
    #endif

    if (not converged) {
      #if DEBUG_JACOBI
      printf("%d: jacobi1d: next iter=%d\n", my_node, cur_iter);
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

static int exitEarly(NodeType node, int exit_code, char* reason) {
  if (node == 0) {
    CollectiveOps::abort(std::string(reason), exit_code);
  }

  CollectiveOps::finalize();
  return exit_code;
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (argc == 2 && strncmp(argv[1], "gtest", 5) == 0) {
    total_size = 1024;
    max_iterations = 64;
  } else {
    if (argc != 3) {
      char buf[256];
      sprintf(buf, "usage: %s <total-num-elements> <max-iterations>", argv[0]);
      return exitEarly(my_node, 1, buf);
    }

    total_size = atoi(argv[1]);
    max_iterations = atoi(argv[2]);
  }

  if (num_nodes == 1) {
    char buf[256];
    sprintf(buf, "Need >= 2 ranks:\n mpirun-mpich-clang -n 2 %s", argv[0]);
    return exitEarly(my_node, 1, buf);
  }

  blk_size = total_size / num_nodes;

  assert(blk_size * num_nodes == total_size);
  assert(blk_size * num_nodes == total_size);

  t1 = new double[blk_size + 2];
  t2 = new double[blk_size + 2];
  local_resid_buf = new double[num_nodes];

  for (int i = 0; i < blk_size + 2; i++) {
    t1[i] = 0.0;
    t2[i] = 0.0;
  }

  printf("%d: total_size=%lld, blk_size=%lld\n", my_node, total_size, blk_size);

  jac_t1_han = theRDMA()->registerCollectiveTyped(t1 + 1, blk_size, total_size);
  jac_t2_han = theRDMA()->registerCollectiveTyped(t2 + 1, blk_size, total_size);
  jac_resid = theRDMA()->registerCollectiveTyped(&resid_val, 1, num_nodes);

  theBarrier()->barrierThen([]{
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
