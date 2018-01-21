
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

static void tell_handle(TestMsg* msg) {
  printf("%d: handle=%lld\n", my_node, msg->han);

  if (my_node == 1 || my_node == 2) {
    printf("%d: requesting data\n", my_node);
    theRDMA()->getData(msg->han, my_node, sizeof(double)*3, no_byte, [](void* data, size_t num_bytes){
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
static TestMsg* test_msg = nullptr;

static RDMA_GetType
test_get_fn(TestMsg* msg, ByteType num_bytes, ByteType offset, TagType tag, bool) {
  printf(
    "%d: running test_get_fn: msg=%p, num_bytes=%lld, tag=%d\n",
    my_node, msg, num_bytes, tag
  );
  return RDMA_GetType{
    my_data+tag, num_bytes == no_byte ? sizeof(double)*10 : num_bytes
  };
}

int main(int argc, char** argv) {
  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (my_node == 0) {
    auto const len = 64;
    my_data = new double[len];

    for (auto i = 0; i < len; i++) {
      my_data[i] = i+1;
    }

    my_handle = theRDMA()->registerNewRdmaHandler();
    theRDMA()->associateGetFunction<TestMsg>(test_msg, my_handle, test_get_fn, true);
    printf("initializing my_handle=%lld\n", my_handle);

    TestMsg* msg = new TestMsg(my_node);
    msg->han = my_handle;
    theMsg()->broadcastMsg<TestMsg, tell_handle>(msg, [=]{ delete msg; });
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
