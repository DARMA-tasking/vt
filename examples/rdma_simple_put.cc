
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static node_t my_node = uninitialized_destination;
static node_t num_nodes = uninitialized_destination;

static rdma_handle_t my_handle = no_rdma_handle;

struct TestMsg : runtime::Message {
  rdma_handle_t han;
  TestMsg(rdma_handle_t const& in_han) : Message(), han(in_han) { }
};

static double* my_data = nullptr;

static void read_data_fn(TestMsg* msg) {
  printf("%d: read_data_fn: handle=%lld\n", my_node, msg->han);

  if (my_node == 0) {
    int const len = 10;
    for (auto i = 0; i < len; i++) {
      printf("\t: my_data[%d] = %f\n", i, my_data[i]);
    }
  }
}

static void put_data_fn(TestMsg* msg) {
  printf("%d: put_data_fn: handle=%lld\n", my_node, msg->han);

  if (my_node == 1) {
    printf("%d: putting data\n", my_node);

    int const local_data_len = 3;
    double* local_data = new double[local_data_len];
    for (auto i = 0; i < local_data_len; i++) {
      local_data[i] = (i+1)*1000*(my_node+1);
    }
    the_rdma->put_data(msg->han, local_data, sizeof(double)*local_data_len, [=]{
      delete [] local_data;
    }, [=]{
      printf("%d: after put: sending msg back to 0\n", my_node);
      TestMsg* msg = new TestMsg(my_node);
      msg->han = my_handle;
      the_msg->send_msg<TestMsg,read_data_fn>(0, msg, [=]{ delete msg; });
    });
  }
}

static void put_handler_fn(
  BaseMessage* msg, rdma_ptr_t in_ptr, byte_t in_num_bytes, byte_t offset, tag_t tag
) {
  printf(
    "%d: put_handler_fn: my_data=%p, in_ptr=%p, in_num_bytes=%lld, tag=%d\n",
    my_node, my_data, in_ptr, in_num_bytes, tag
  );

  std::memcpy(my_data, in_ptr, in_num_bytes);
}


int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  my_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  if (num_nodes != 4) {
    fprintf(stderr, "requires exactly 4 nodes\n");
    return 0;
  }

  if (my_node == 0) {
    auto const len = 64;
    my_data = new double[len];

    for (auto i = 0; i < len; i++) {
      my_data[i] = i+1;
    }

    //my_handle = the_rdma->register_new_typed_rdma_handler(my_data, 10);
    my_handle = the_rdma->register_new_rdma_handler();
    the_rdma->associate_put_function(my_handle, put_handler_fn, false);
    printf("initializing my_handle=%lld\n", my_handle);

    TestMsg* msg = new TestMsg(my_node);
    msg->han = my_handle;
    the_msg->broadcast_msg<TestMsg,put_data_fn>(msg, [=]{ delete msg; });
    //the_msg->send_msg<TestMsg,put_data_fn>(0, msg, [=]{ delete msg; });
  }

  while (1) {
    the_msg->scheduler();
  }

  return 0;
}
