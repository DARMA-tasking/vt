
#include "transport.h"

#include <cstdlib>
#include <unordered_map>

using namespace vt;

static NodeType my_node = uninitialized_destination;
static NodeType num_nodes = uninitialized_destination;

static RDMA_HandleType my_handle = no_rdma_handle;
static int const num_elms = 10;

struct Msg : ::vt::Message {
  int state = -100;
};

using RDMAMsgType = Msg;

static RDMAMsgType* rdma_state = nullptr;
static constexpr RDMA_ElmType const rdma_num_elements = 10;
static std::unordered_map<RDMA_ElmType, double*> rdma_data;

static RDMA_GetType get_fn(
  RDMAMsgType* msg, ByteType num_bytes, ByteType offset, TagType tag
) {
  printf(
    "%d: running get_fn: msg=%p, num_bytes=%lld, offset=%lld, tag=%d, state=%d\n",
    my_node, msg, num_bytes, offset, tag, msg->state
  );

  auto iter = rdma_data.find(offset);
  if (iter == rdma_data.end()) {
    double* new_data_ptr = new double[10];
    for (int i = 0; i < 10; i++) {
      new_data_ptr[i] = i * 1.0 * (offset+1);
    }
    rdma_data.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(offset),
      std::forward_as_tuple(new_data_ptr)
    );
    iter = rdma_data.find(offset);
  }

  return RDMA_GetType{iter->second, sizeof(double)*rdma_num_elements};
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

  rdma_state = new RDMAMsgType();

  my_handle = RDMACollectionManager::registerUnsizedCollection(num_elms);
  theRDMA()->associateGetFunction<RDMAMsgType>(rdma_state, my_handle, get_fn, true);

  theBarrier()->barrier();

  RDMACollectionManager::getElement(my_handle, 1, [](void* data, size_t num_bytes){
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
