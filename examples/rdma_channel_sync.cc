
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static node_t my_node = uninitialized_destination;
static node_t num_nodes = uninitialized_destination;

static rdma_handle_t my_handle_1 = no_rdma_handle;

static int const put_len = 2;
static int const my_data_len = 8;
static double* my_data = nullptr;

static bool use_paired_sync = true;

struct TestMsg : runtime::Message {
  rdma_handle_t han;
  TestMsg(rdma_handle_t const& in_han) : Message(), han(in_han) { }
};

static void put_channel_setup(TestMsg* msg);

static void read_data_fn(TestMsg* msg) {
  printf("%d: read_data_fn: handle=%lld\n", my_node, msg->han);

  if (my_node == 0) {
    the_rdma->sync_local_put_channel(msg->han, [=]{
      for (auto i = 0; i < put_len*2; i++) {
        printf("%d: han=%lld \t: my_data[%d] = %f\n", my_node, msg->han, i, my_data[i]);
      }

      the_rdma->setup_get_channel_with_remote(my_handle_1, 2, [=]{
        TestMsg* msg1 = make_shared_message<TestMsg>(my_handle_1);
        the_msg->send_msg<TestMsg, put_channel_setup>(2, msg1);
      });
    });
  } else if (my_node == 2) {
    the_rdma->sync_local_get_channel(msg->han, [=]{
      for (auto i = 0; i < put_len*2; i++) {
        printf("%d: han=%lld \t: my_data[%d] = %f\n", my_node, msg->han, i, my_data[i]);
      }
    });
  }
}

static void put_channel_setup(TestMsg* msg) {
  auto const& handle = msg->han;

  printf("%d: put_channel_setup: handle=%lld\n", my_node, msg->han);

  if (my_node == 1) {
    the_rdma->create_put_channel(handle, [=]{
      int const num_elm = 2;

      if (use_paired_sync) {
        the_rdma->put_typed_data(handle, my_data, num_elm, no_byte, no_action, [=]{
          TestMsg* back = make_shared_message<TestMsg>(handle);
          the_msg->send_msg<TestMsg, read_data_fn>(0, back);
        });
      } else {
        the_rdma->put_typed_data(handle, my_data, num_elm);
        the_rdma->sync_remote_put_channel(handle, [=]{
          TestMsg* back = make_shared_message<TestMsg>(handle);
          the_msg->send_msg<TestMsg, read_data_fn>(0, back);
        });
      }
    });
  }
  else if (my_node == 2) {
    the_rdma->create_get_channel(handle, [=]{
      printf(
        "%d: creating get channel complete\n", my_node
      );
      int const num_elm = 2;

      if (use_paired_sync) {
        the_rdma->get_typed_data_info_buf(handle, my_data, num_elm, [=]{
          TestMsg* back = make_shared_message<TestMsg>(handle);
          the_msg->send_msg<TestMsg, read_data_fn>(2, back);
        });
      } else {
        the_rdma->get_typed_data_info_buf(handle, my_data, num_elm);
        // the_rdma->get_typed_data_info_buf(handle, my_data+2, num_elm);
        the_rdma->sync_local_get_channel(handle, [=]{
          TestMsg* back = make_shared_message<TestMsg>(handle);
          the_msg->send_msg<TestMsg, read_data_fn>(2, back);
        });
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

  if (my_node < 3) {
    // initialize my_data buffer, all but node 0 get -1.0
    for (auto i = 0; i < 4; i++) {
      my_data[i] = my_node != 0 ? (my_node+1)*i+1 : -1.0;
      printf("%d: \t: my_data[%d] = %f\n", my_node, i, my_data[i]);
    }
  }

  if (my_node == 0) {
    my_handle_1 = the_rdma->register_new_typed_rdma_handler(my_data, put_len);

    printf(
      "%d: initializing my_handle_1=%llx\n", my_node, my_handle_1
    );

    the_rdma->setup_put_channel_with_remote(my_handle_1, 1, [=]{
      TestMsg* msg1 = make_shared_message<TestMsg>(my_handle_1);
      the_msg->send_msg<TestMsg, put_channel_setup>(1, msg1);
    });
  }

  while (1) {
    the_msg->scheduler();
  }

  return 0;
}
