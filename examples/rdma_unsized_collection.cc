
#include "transport.h"
#include <cstdlib>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static RDMA_HandleType my_handle = no_rdma_handle;
static int const num_elms = 10;

struct Msg : ::vt::Message {

};

using RDMAMsgType = Msg;

static RDMA_GetType get_fn(
  RDMAMsgType* msg, ByteType num_bytes, ByteType offset, TagType tag
) {
  printf(
    "%d: running get_fn: msg=%p, num_bytes=%lld, offset=%lld, tag=%d\n",
    my_node, msg, num_bytes, offset, tag
  );
  double* x = new double[10];
  for (int i = 0; i < 10; i++) {
    x[i] = i * 1.0;
  }
  return RDMA_GetType{x, sizeof(double)*10};
}

int main(int argc, char** argv) {
  using namespace ::vt::rdma;

  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes < 4) {
    CollectiveOps::abort("requires exactly 4 nodes", 0);
    return 0;
  }

  my_handle = RDMACollectionManager::registerUnsizedCollection(num_elms);
  theRDMA()->associateGetFunction<RDMAMsgType>(nullptr, my_handle, get_fn, true);

  theBarrier()->barrier();

  RDMACollectionManager::getElement(my_handle, 5, [](void* data, size_t num_bytes){
    double* const ptr = static_cast<double*>(data);
    size_t const num_elems = num_bytes / sizeof(double);
    printf("%d: data arrived: data=%p, num_bytes=%zu\n", my_node, data, num_bytes);
    for (auto i = 0; i < num_elems; i++) {
      printf("\t: my_data[%d] = %f\n", i, ptr[i]);
    }
  });


  printf("%d: handle=%lld, create handle\n", my_node, my_handle);

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
