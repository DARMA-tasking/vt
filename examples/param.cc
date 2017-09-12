
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static void fn_test(int a, int b, bool x) {
  printf("fn: a=%d, b=%d, x=%s\n", a, b, x ? "true" : "false");
}

static void fn_test2(int x, int y) {
  printf("fn2: x=%d,y=%d\n",x,y);
}

static void fn_test3(int x, double y) {
  printf("fn3: x=%d,y=%f\n",x,y);
}

struct FunctorTest1 {
  void operator()(int x, double y) const {
    printf("FunctorTest1: x=%d,y=%f\n",x,y);
  }
};

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  my_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  if (num_nodes == 1) {
    fprintf(stderr, "Please run with at least two ranks!\n");
    fprintf(stderr, "\t mpirun-mpich-clang -n 2 %s\n", argv[0]);
    exit(1);
  }

  if (my_node == 0) {
    the_param->send_data(1, build_data(10, 20, false), param_function_rhs(fn_test));
    the_param->send_data(1, param_function_rhs(fn_test), 50, 29, false);
    the_param->send_data<param_function(fn_test)>(1, build_data(10, 20, false));
    the_param->send_data<param_function(fn_test)>(1, 45, 23, true);

    the_param->send_data<param_function(fn_test2)>(1, 20, 10);
    the_param->send_data<param_function(fn_test3)>(1, 20, 50.0);

    the_param->send_data<FunctorTest1>(1, build_data(20, 50.0));
    the_param->send_data<FunctorTest1>(1, 20, 100.0);
    the_param->send_data<FunctorTest1>(1, build_data(10, 70.0));
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
