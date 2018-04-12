
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
  fmt::print("{}: read_data_fn: handle={}\n", my_node, msg->han);

  for (auto i = 0; i < put_len*2; i++) {
    fmt::print("{}: han={} \t: my_data[{}] = {}\n", my_node, msg->han, i, my_data[i]);
  }
}

static void put_channel_setup(TestMsg* msg) {
  auto const& handle = msg->han;

  fmt::print("{}: put_channel_setup: handle={}\n", my_node, msg->han);

  if (my_node == 1) {
    int const num_elm = 2;
    theRDMA()->putTypedData(handle, my_data, num_elm, no_byte, no_tag, [=]{
      TestMsg* back = makeSharedMessage<TestMsg>(handle);
      theMsg()->sendMsg<TestMsg, read_data_fn>(0, back);
    });
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  my_data = new double[my_data_len];

  if (my_node < 2) {
    // initialize my_data buffer, all but node 0 get -1.0
    for (auto i = 0; i < 4; i++) {
      my_data[i] = my_node != 0 ? (my_node+1)*i+1 : -1.0;
      fmt::print("{}: \t: my_data[{}] = {}\n", my_node, i, my_data[i]);
    }
  }

  if (my_node == 0) {
    my_handle_1 = theRDMA()->registerNewTypedRdmaHandler(my_data, put_len);

    fmt::print(
      "{}: initializing my_handle_1={}\n", my_node, my_handle_1
    );

    theRDMA()->newPutChannel(my_handle_1, 0, 1, [=]{
      TestMsg* msg1 = makeSharedMessage<TestMsg>(my_handle_1);
      theMsg()->sendMsg<TestMsg, put_channel_setup>(1, msg1);
    });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
