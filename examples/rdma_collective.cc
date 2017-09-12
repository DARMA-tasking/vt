
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static RDMA_HandleType my_handle = no_rdma_handle;
static HandlerType test_han = uninitialized_handler;

static int const my_data_len = 8;
static int const local_data_len = 24;
static double* my_data = nullptr;
static double* local_data = nullptr;

struct TestMsg : vt::Message {
  RDMA_HandleType han;

  TestMsg(RDMA_HandleType const& in_han) : Message(), han(in_han) { }
};

static void announce(TestMsg* msg) {
  auto const& rdma_handle = msg->han;

  printf("%d: handle=%lld, requesting data\n", my_node, rdma_handle);

  if (my_node == 1) {
    theRDMA->new_get_channel(my_handle, 2, 1, [=]{
      printf("set up channel with 2\n");

      theRDMA->get_typed_data_info_buf(rdma_handle, local_data, local_data_len, 5, no_tag, [=]{
        printf("%d: handle=%lld, finished getting data\n", my_node, rdma_handle);
        for (int i = 0; i < local_data_len; i++) {
          printf("%d: \t local_data[%d] = %f\n", my_node, i, local_data[i]);
          assert(local_data[i] == 5.0+i);
        }
      });
    });
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize_context(argc, argv);
  CollectiveOps::initialize_runtime();

  my_node = theContext->get_node();
  num_nodes = theContext->get_num_nodes();

  if (num_nodes < 4) {
    fprintf(stderr, "requires at least 4 nodes\n");
    return 0;
  }

  my_data = new double[my_data_len];
  local_data = new double[local_data_len];

  // initialize my_data buffer, all but node 0 get -1.0
  for (auto i = 0; i < my_data_len; i++) {
    my_data[i] = (my_node)*my_data_len + i;
  }

  for (auto i = 0; i < local_data_len; i++) {
    local_data[i] = 0.0;
  }

  my_handle = theRDMA->register_collective_typed(
    my_data, my_data_len, my_data_len*num_nodes
  );

  theBarrier->barrier();

  printf("%d: handle=%lld, create handle\n", my_node, my_handle);

  if (my_node == 0) {
    theRDMA->new_get_channel(my_handle, 0, 1, [=]{
      TestMsg* msg = make_shared_message<TestMsg>(my_node);
      msg->han = my_handle;
      theMsg->broadcast_msg<TestMsg, announce>(msg);
    });
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
