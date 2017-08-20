
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static node_t my_node = uninitialized_destination;
static node_t num_nodes = uninitialized_destination;

static rdma_handle_t my_handle_1 = no_rdma_handle;

handler_t han_x = uninitialized_handler;
handler_t test_han = uninitialized_handler;
handler_t test_han2 = uninitialized_handler;
handler_t put_channel_setup_han = uninitialized_handler;

static int const put_len = 2;
static int const my_data_len = 8;
static double* my_data = nullptr;

struct TestMsg : runtime::Message {
  rdma_handle_t han;
  TestMsg(rdma_handle_t const& in_han) : Message(), han(in_han) { }
};

static void read_data_fn(runtime::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);

  printf("%d: read_data_fn: handle=%lld\n", my_node, msg.han);

  for (auto i = 0; i < put_len*2; i++) {
    printf("%d: han=%lld \t: my_data[%d] = %f\n", my_node, msg.han, i, my_data[i]);
  }
}

static void put_channel_setup(runtime::BaseMessage* in_msg) {
  TestMsg& msg = *static_cast<TestMsg*>(in_msg);
  auto const& handle = msg.han;

  printf("%d: put_channel_setup: handle=%lld\n", my_node, msg.han);

  if (my_node == 1) {
    the_rdma->create_put_channel(handle, [=]{
      int const num_elm = 2;
      the_rdma->put_typed_data(handle, my_data, num_elm);
      TestMsg* back = make_shared_message<TestMsg>(handle);
      the_msg->send_msg(0, test_han2, back);
    });
  }
  else if (my_node == 2) {
    the_rdma->create_get_channel(handle, [=]{
      int const num_elm = 2;
      the_rdma->get_typed_data_info_buf(handle, my_data, num_elm);
      TestMsg* back = make_shared_message<TestMsg>(handle);
      the_msg->send_msg(2, test_han2, back);
    });
  }
}

static void han_x_fn(runtime::BaseMessage* in_msg) {
  printf("han_x_fn\n");
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  han_x = the_msg->collective_register_handler(han_x_fn, 10);
  //test_han = the_msg->collective_register_handler(put_data_fn);
  test_han2 = the_msg->collective_register_handler(read_data_fn);
  put_channel_setup_han = the_msg->collective_register_handler(put_channel_setup);

  my_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  my_data = new double[my_data_len];

  // initialize my_data buffer, all but node 0 get -1.0
  for (auto i = 0; i < my_data_len; i++) {
    my_data[i] = my_node != 0 ? (my_node+1)*i+1 : -1.0;
    printf("%d: \t: my_data[%d] = %f\n", my_node, i, my_data[i]);
  }

  if (my_node == 0) {
    my_handle_1 = the_rdma->register_new_typed_rdma_handler(my_data, put_len);

    printf(
      "%d: initializing my_handle_1=%llx\n", my_node, my_handle_1
    );

    if (0) {
      the_rdma->setup_put_channel_with_remote(my_handle_1, 1, [=]{
        TestMsg* msg1 = make_shared_message<TestMsg>(my_handle_1);
        the_msg->send_msg(1, put_channel_setup_han, msg1);
      });
    } else {
      the_rdma->setup_get_channel_with_remote(my_handle_1, 2, [=]{
        TestMsg* msg1 = make_shared_message<TestMsg>(my_handle_1);
        the_msg->send_msg(2, put_channel_setup_han, msg1);
      });
    }

    // if (0) {
    //   TestMsg* msg1 = make_shared_message<TestMsg>(my_handle_1);
    //   the_msg->send_msg(1, put_channel_setup_han, msg1);
    // } else {
    //   TestMsg* msg2 = make_shared_message<TestMsg>(my_handle_1);
    //   the_msg->send_msg(2, put_channel_setup_han, msg2);
    // }
  }

  while (1) {
    the_msg->scheduler();
  }

  return 0;
}
