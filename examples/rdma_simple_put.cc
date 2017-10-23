
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static RDMA_HandleType my_handle = no_rdma_handle;

struct TestMsg : vt::Message {
  RDMA_HandleType han;
  TestMsg(RDMA_HandleType const& in_han) : Message(), han(in_han) { }
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
    theRDMA()->putData(msg->han, local_data, sizeof(double)*local_data_len, [=]{
      delete [] local_data;
    }, [=]{
      printf("%d: after put: sending msg back to 0\n", my_node);
      TestMsg* msg = new TestMsg(my_node);
      msg->han = my_handle;
      theMsg()->sendMsg<TestMsg,read_data_fn>(0, msg, [=]{ delete msg; });
    });
  }
}

static void put_handler_fn(
  BaseMessage* msg, RDMA_PtrType in_ptr, ByteType in_num_bytes, ByteType offset, TagType tag
) {
  printf(
    "%d: put_handler_fn: my_data=%p, in_ptr=%p, in_num_bytes=%lld, tag=%d\n",
    my_node, my_data, in_ptr, in_num_bytes, tag
  );

  std::memcpy(my_data, in_ptr, in_num_bytes);
}


int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes != 4) {
    CollectiveOps::abort("requires exactly 4 nodes", 0);
    return 0;
  }

  if (my_node == 0) {
    auto const len = 64;
    my_data = new double[len];

    for (auto i = 0; i < len; i++) {
      my_data[i] = i+1;
    }

    //my_handle = theRDMA()->register_new_typed_rdma_handler(my_data, 10);
    my_handle = theRDMA()->registerNewRdmaHandler();
    theRDMA()->associatePutFunction(my_handle, put_handler_fn, false);
    printf("%d: initializing my_handle=%lld\n", my_node, my_handle);

    TestMsg* msg = new TestMsg(my_node);
    msg->han = my_handle;
    theMsg()->broadcastMsg<TestMsg,put_data_fn>(msg, [=]{ delete msg; });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
