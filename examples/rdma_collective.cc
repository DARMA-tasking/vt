
#include "transport.h"
#include <cstdlib>

using namespace runtime;

static node_t my_node = uninitialized_destination;
static node_t num_nodes = uninitialized_destination;

static rdma_handle_t my_handle = no_rdma_handle;
static handler_t test_han = uninitialized_handler;

static int const my_data_len = 8;
static int const local_data_len = 24;
static double* my_data = nullptr;
static double* local_data = nullptr;

struct TestMsg : runtime::Message {
  rdma_handle_t han;

  TestMsg(rdma_handle_t const& in_han) : Message(), han(in_han) { }
};

static void announce(TestMsg* msg) {
  auto const& rdma_handle = msg->han;

  printf("%d: handle=%lld, requesting data\n", my_node, rdma_handle);

  if (my_node == 1) {
    //the_rdma->get_typed_data_info_buf(rdma_handle, local_data, 16, no_byte, no_tag, [=]{
    the_rdma->create_get_channel(rdma_handle, 2, [=]{
      printf("set up channel with 2\n");

      the_rdma->get_typed_data_info_buf(rdma_handle, local_data, local_data_len, 5, no_tag, [=]{
        printf("%d: handle=%lld, finished getting data\n", my_node, rdma_handle);
        for (int i = 0; i < local_data_len; i++) {
          printf("%d: \t local_data[%d] = %f\n", my_node, i, local_data[i]);
          assert(local_data[i] == 5.0+i);
        }
      });
    });
    // the_rdma->get_region(rdma_handle, local_data, rdma::Region{5,5+local_data_len}, [=]{
    //   printf("%d: handle=%lld, finished getting data\n", my_node, rdma_handle);

    //   for (int i = 0; i < local_data_len; i++) {
    //     printf("%d: \t local_data[%d] = %f\n", my_node, i, local_data[i]);
    //     assert(local_data[i] == 5.0+i);
    //   }
    // });
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  my_node = the_context->get_node();
  num_nodes = the_context->get_num_nodes();

  my_data = new double[my_data_len];
  local_data = new double[local_data_len];

  // initialize my_data buffer, all but node 0 get -1.0
  for (auto i = 0; i < my_data_len; i++) {
    my_data[i] = (my_node)*my_data_len + i;
  }

  for (auto i = 0; i < local_data_len; i++) {
    local_data[i] = 0.0;
  }

  my_handle = the_rdma->register_collective_typed(
    my_data, my_data_len, my_data_len*num_nodes
  );

  the_barrier->barrier();

  printf("%d: handle=%lld, create handle\n", my_node, my_handle);

  if (my_node == 0) {
    the_rdma->setup_get_channel_with_remote(my_handle, 1, [=]{
      TestMsg* msg = make_shared_message<TestMsg>(my_node);
      msg->han = my_handle;
      the_msg->broadcast_msg<TestMsg, announce>(msg);
    });
  }

  while (1) {
    the_msg->scheduler();
  }

  return 0;
}
