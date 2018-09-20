
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
  fmt::print("{}: read_data_fn: handle={}\n", my_node, msg->han);

  if (my_node == 0) {
    int const len = 10;
    for (auto i = 0; i < len; i++) {
      fmt::print("\t: my_data[{}] = {}\n", i, my_data[i]);
    }
  }
}

static void put_data_fn(TestMsg* msg) {
  fmt::print("{}: put_data_fn: handle={}\n", my_node, msg->han);

  if (my_node < 4) {
    fmt::print("{}: putting data\n", my_node);

    int const local_data_len = 3;
    double* local_data = new double[local_data_len];
    for (auto i = 0; i < local_data_len; i++) {
      local_data[i] = (i+1)*1000*(my_node+1);
    }
    theRDMA()->putData(
      msg->han, local_data, sizeof(double)*local_data_len,
      (my_node-1)*local_data_len, no_tag, vt::rdma::rdma_default_byte_size,
      [=]{
        delete [] local_data;
      }, [=]{
        fmt::print("{}: after put: sending msg back to 0\n", my_node);
        auto msg = makeSharedMessage<TestMsg>(my_node);
        msg->han = my_handle;
        theMsg()->sendMsg<TestMsg,read_data_fn>(0, msg);
      }
    );
  }
}

static void put_handler_fn(
  BaseMessage* msg, RDMA_PtrType in_ptr, ByteType in_num_bytes, ByteType offset,
  TagType tag, bool
) {
  fmt::print(
    "{}: put_handler_fn: my_data={}, in_ptr={}, in_num_bytes={}, tag={}, "
    "offset={}\n",
    my_node, print_ptr(my_data), print_ptr(in_ptr), in_num_bytes, tag, offset
  );

  for (auto i = 0; i < in_num_bytes/sizeof(double); i++) {
    ::fmt::print(
      "{}: put_handler_fn: data[{}] = {}\n",
      my_node, i, static_cast<double*>(in_ptr)[i]
    );
  }

  std::memcpy(my_data + offset, in_ptr, in_num_bytes);
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
    theRDMA()->associatePutFunction<BaseMessage,put_handler_fn>(
      nullptr, my_handle, put_handler_fn, false
    );
    fmt::print("{}: initializing my_handle={}\n", my_node, my_handle);

    auto msg = makeSharedMessage<TestMsg>(my_node);
    msg->han = my_handle;
    theMsg()->broadcastMsg<TestMsg,put_data_fn>(msg);
  }

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
