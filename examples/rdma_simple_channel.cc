
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static RDMA_HandleType my_handle_1 = no_rdma_handle;

static int const put_len = 2;
static int const my_data_len = 8;
static double* my_data = nullptr;

struct TestMsg : vt::Message {
  RDMA_HandleType han;
  TestMsg(RDMA_HandleType const& in_han) : Message(), han(in_han) { }
};

static void read_data_fn(TestMsg* msg) {
  printf("%d: read_data_fn: handle=%lld\n", my_node, msg->han);

  for (auto i = 0; i < put_len*2; i++) {
    printf("%d: han=%lld \t: my_data[%d] = %f\n", my_node, msg->han, i, my_data[i]);
  }
}

static void put_channel_setup(TestMsg* msg) {
  auto const& handle = msg->han;

  printf("%d: put_channel_setup: handle=%lld\n", my_node, msg->han);

  if (my_node == 1) {
    int const num_elm = 2;
    theRDMA->putTypedData(handle, my_data, num_elm, no_byte, no_tag, [=]{
      TestMsg* back = make_shared_message<TestMsg>(handle);
      theMsg->sendMsg<TestMsg, read_data_fn>(0, back);
    });
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initializeContext(argc, argv);
  CollectiveOps::initializeRuntime();

  my_node = theContext->getNode();
  num_nodes = theContext->getNumNodes();

  my_data = new double[my_data_len];

  if (my_node < 2) {
    // initialize my_data buffer, all but node 0 get -1.0
    for (auto i = 0; i < 4; i++) {
      my_data[i] = my_node != 0 ? (my_node+1)*i+1 : -1.0;
      printf("%d: \t: my_data[%d] = %f\n", my_node, i, my_data[i]);
    }
  }

  if (my_node == 0) {
    my_handle_1 = theRDMA->registerNewTypedRdmaHandler(my_data, put_len);

    printf(
      "%d: initializing my_handle_1=%llx\n", my_node, my_handle_1
    );

    theRDMA->newPutChannel(my_handle_1, 0, 1, [=]{
      TestMsg* msg1 = make_shared_message<TestMsg>(my_handle_1);
      theMsg->sendMsg<TestMsg, put_channel_setup>(1, msg1);
    });
  }

  while (1) {
    run_scheduler();
  }

  return 0;
}
