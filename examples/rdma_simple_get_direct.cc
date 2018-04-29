
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static RDMA_HandleType my_handle = no_rdma_handle;

static int const my_data_len = 8;
static double* my_data = nullptr;

struct TestMsg : vt::Message {
  RDMA_HandleType han;
  TestMsg(RDMA_HandleType const& in_han) : Message(), han(in_han) { }
};

static void tellHandle(TestMsg* msg) {
  if (my_node != 0) {
    fmt::print("{}: handle={}, requesting data\n", my_node, msg->han);
    int const num_elm = 2;
    theRDMA()->getTypedDataInfoBuf(msg->han, my_data, num_elm, no_byte, no_tag, [=]{
      for (auto i = 0; i < num_elm; i++) {
        fmt::print("node {}: \t: my_data[{}] = {}\n", my_node, i, my_data[i]);
      }
    });
  }
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  my_data = new double[my_data_len];

  // initialize my_data buffer, all but node 0 get -1.0
  for (auto i = 0; i < my_data_len; i++) {
    my_data[i] = my_node == 0 ? (my_node+1)*i+1 : -1.0;
  }

  if (my_node == 0) {
    my_handle = theRDMA()->registerNewTypedRdmaHandler(my_data, my_data_len);

    TestMsg* msg = new TestMsg(my_node);
    msg->han = my_handle;
    theMsg()->broadcastMsg<TestMsg, tellHandle>(msg, [=]{ delete msg; });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
