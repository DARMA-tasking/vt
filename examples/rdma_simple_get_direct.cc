
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static node_t my_node = uninitialized_destination;
static node_t num_nodes = uninitialized_destination;

static rdma_handle_t my_handle = no_rdma_handle;

static int const my_data_len = 8;
static double* my_data = nullptr;

struct TestMsg : runtime::Message {
  rdma_handle_t han;
  TestMsg(rdma_handle_t const& in_han) : Message(), han(in_han) { }
};

static void tell_handle(TestMsg* msg) {
  if (my_node != 0) {
    printf("%d: handle=%lld, requesting data\n", my_node, msg->han);
    int const num_elm = 2;
    the_rdma->get_typed_data_info_buf(msg->han, my_data, num_elm, no_byte, no_tag, [=]{
      for (auto i = 0; i < num_elm; i++) {
        printf("node %d: \t: my_data[%d] = %f\n", my_node, i, my_data[i]);
      }
    });
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  my_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  my_data = new double[my_data_len];

  // initialize my_data buffer, all but node 0 get -1.0
  for (auto i = 0; i < my_data_len; i++) {
    my_data[i] = my_node == 0 ? (my_node+1)*i+1 : -1.0;
  }

  if (my_node == 0) {
    my_handle = the_rdma->register_new_typed_rdma_handler(my_data, my_data_len);

    TestMsg* msg = new TestMsg(my_node);
    msg->han = my_handle;
    the_msg->broadcast_msg<TestMsg, tell_handle>(msg, [=]{ delete msg; });
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
