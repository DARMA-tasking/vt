
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static node_t my_node = uninitialized_destination;
static node_t num_nodes = uninitialized_destination;

static rdma_handle_t my_handle = no_rdma_handle;

handler_t test_han = uninitialized_handler;

struct TestMsg : runtime::Message {
  rdma_handle_t han;
  TestMsg(rdma_handle_t const& in_han) : Message(), han(in_han) { }
};

static void tell_handle(runtime::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  printf("%d: handle=%lld\n", my_node, msg.han);

  if (my_node == 1) {
    printf("%d: requesting data\n", my_node);
    the_rdma->get_data(msg.han, [](void* data, size_t num_bytes){
      printf("%d: data arrived: data=%p, num_bytes=%zu\n", my_node, data, num_bytes);
    });
  }
}

static double* my_data = nullptr;

static rdma_get_t
test_get_fn(BaseMessage* msg, byte_t num_bytes, tag_t tag) {
  printf(
    "%d: running test_get_fn: msg=%p, num_bytes=%lld, tag=%d\n",
    my_node, msg, num_bytes, tag
  );
  return rdma_get_t{my_data, sizeof(double)*2}; //std::make_tuple<rdma_ptr_t(my_data)
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  test_han = CollectiveOps::register_handler(tell_handle);

  my_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  if (my_node == 0) {
    my_data = new double[64];

    my_handle = the_rdma->register_new_rdma_handler();
    the_rdma->associate_get_function(my_handle, test_get_fn);
    printf("initializing my_handle=%lld\n", my_handle);

    TestMsg* msg = new TestMsg(my_node);
    msg->han = my_handle;
    the_msg->broadcast_msg(test_han, msg, [=]{ delete msg; });
  }

  while (1) {
    the_msg->scheduler();
  }

  return 0;
}
