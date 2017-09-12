
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static RDMA_HandleType my_handle = no_rdma_handle;

struct TestMsg : runtime::Message {
  RDMA_HandleType han;
  TestMsg(RDMA_HandleType const& in_han) : Message(), han(in_han) { }
};

static void tell_handle(TestMsg* msg) {
  printf("%d: handle=%lld\n", my_node, msg->han);

  if (my_node == 1 || my_node == 2) {
    printf("%d: requesting data\n", my_node);
    the_rdma->get_data(msg->han, my_node, sizeof(double)*3, no_byte, [](void* data, size_t num_bytes){
      double* const ptr = static_cast<double*>(data);
      size_t const num_elems = num_bytes / sizeof(double);
      printf("%d: data arrived: data=%p, num_bytes=%zu\n", my_node, data, num_bytes);
      for (auto i = 0; i < num_elems; i++) {
        printf("\t: my_data[%d] = %f\n", i, ptr[i]);
      }
    });
  }
}

static double* my_data = nullptr;

static RDMA_GetType
test_get_fn(BaseMessage* msg, ByteType num_bytes, ByteType offset, TagType tag) {
  printf(
    "%d: running test_get_fn: msg=%p, num_bytes=%lld, tag=%d\n",
    my_node, msg, num_bytes, tag
  );
  return RDMA_GetType{
    my_data+tag, num_bytes == no_byte ? sizeof(double)*10 : num_bytes
  };
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  my_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  if (my_node == 0) {
    auto const len = 64;
    my_data = new double[len];

    for (auto i = 0; i < len; i++) {
      my_data[i] = i+1;
    }

    my_handle = the_rdma->register_new_rdma_handler();
    the_rdma->associate_get_function(my_handle, test_get_fn, true);
    printf("initializing my_handle=%lld\n", my_handle);

    TestMsg* msg = new TestMsg(my_node);
    msg->han = my_handle;
    the_msg->broadcast_msg<TestMsg, tell_handle>(msg, [=]{ delete msg; });
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
