
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

static void doGetHandler(Msg* m) {
  using namespace vt::rdma;

  RDMACollectionManager::getElement(my_handle, 1, [](void* data, size_t num_bytes){
    double* const ptr = static_cast<double*>(data);
    size_t const num_elems = num_bytes / sizeof(double);
    printf("%d: data arrived: data=%p, num_bytes=%zu\n", my_node, data, num_bytes);
    for (auto i = 0; i < num_elems; i++) {
      printf("\t: my_data[%d] = %f\n", i, ptr[i]);
    }
  });
}

static void initData(ByteType const& offset, double* const data_ptr) {
  for (auto i = 0; i < rdma_num_elements; i++) {
    data_ptr[i] = i * 1.0 * (offset + 1);
  }
}

static RDMA_PtrType obtain_data_ptr(
  RDMA_ElmType const& elm,
  RDMA_PtrType const& in_ptr = nullptr,
  bool const& initDemand = true
) {
  auto iter = rdma_data.find(elm);
  if (iter == rdma_data.end()) {
    double* new_ptr = nullptr;
    if (initDemand) {
      new_ptr = new double[rdma_num_elements];
      if (in_ptr) {
        //std::memcpy(new_ptr, in_ptr, rdma_num_elements * sizeof(double));
      } else {
        initData(elm, new_ptr);
      }
    } else if (in_ptr != nullptr) {
      new_ptr = reinterpret_cast<double* const>(in_ptr);
    }
    if (new_ptr) {
      rdma_data.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(elm),
        std::forward_as_tuple(new_ptr)
      );
      iter = rdma_data.find(elm);
    } else {
      return nullptr;
    }
  }

  if (in_ptr) {
    std::memcpy(iter->second, in_ptr, rdma_num_elements * sizeof(double));
  }
  return iter->second;
}

static RDMA_GetType get_fn(
  RDMAMsgType* msg, ByteType num_bytes, ByteType offset, TagType tag, bool
) {
  printf(
    "%d: running get_fn: msg=%p, num_bytes=%lld, offset=%lld, tag=%d, state=%d\n",
    my_node, msg, num_bytes, offset, tag, msg->state
  );

  auto const& ret_ptr = obtain_data_ptr(offset);

  return RDMA_GetType{ret_ptr, sizeof(double)*rdma_num_elements};
}

static void put_fn(
  RDMAMsgType* msg, RDMA_PtrType ptr, ByteType num_bytes, ByteType offset,
  TagType tag, bool
) {
  printf(
    "%d: put_fn: ptr=%p, num_bytes=%lld, tag=%d, offset=%lld\n",
    my_node, ptr, num_bytes, tag, offset
  );

  auto const& ret_ptr = obtain_data_ptr(offset, ptr, true);
}

static RDMA_PutRetType serialize_put_fn(RDMA_PutRetType put_in) {
  printf(
    "%d: serialize_put_fn: ptr=%p, num_bytes=%lld\n",
    my_node, std::get<0>(put_in), std::get<1>(put_in)
  );

  return RDMA_PutRetType{std::get<0>(put_in), rdma_num_elements*sizeof(double)};
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
  theRDMA()->associatePutFunction<RDMAMsgType>(rdma_state, my_handle, put_fn, true);

  theBarrier()->barrier();

  if (my_node == 0) {
    double* test_data = new double[rdma_num_elements];
    initData(10, test_data);
    // this message that causes a `get' races with the following `put'
    theMsg()->sendMsg<Msg, doGetHandler>(3, makeSharedMessage<Msg>());
    RDMACollectionManager::putElement(
      my_handle, 1, test_data, serialize_put_fn, no_action, []{
        printf("%d: put finished\n", my_node);
        theMsg()->sendMsg<Msg, doGetHandler>(1, makeSharedMessage<Msg>());
      }
    );
  }

  printf("%d: handle=%lld, create handle\n", my_node, my_handle);

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
