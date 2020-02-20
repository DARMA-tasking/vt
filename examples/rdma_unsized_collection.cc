/*
//@HEADER
// *****************************************************************************
//
//                          rdma_unsized_collection.cc
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/transport.h"

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
    fmt::print(
      "{}: data arrived: data={}, num_bytes={}\n",
      my_node, print_ptr(data), num_bytes
    );
    for (size_t i = 0; i < num_elems; i++) {
      fmt::print("\t: my_data[{}] = {}\n", i, ptr[i]);
    }
  });
}

static void initData(ByteType const& offset, double* const data_ptr) {
  for (RDMA_ElmType i = 0; i < rdma_num_elements; i++) {
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
  fmt::print(
    "{}: running get_fn: msg={}, num_bytes={}, offset={}, tag={}, state={}\n",
    my_node, print_ptr(msg), num_bytes, offset, tag, msg->state
  );

  auto const& ret_ptr = obtain_data_ptr(offset);

  return RDMA_GetType{ret_ptr, sizeof(double)*rdma_num_elements};
}

static void put_fn(
  RDMAMsgType* msg, RDMA_PtrType ptr, ByteType num_bytes, ByteType offset,
  TagType tag, bool
) {
  fmt::print(
    "{}: put_fn: ptr={}, num_bytes={}, tag={}, offset={}\n",
    my_node, print_ptr(ptr), num_bytes, tag, offset
  );

  obtain_data_ptr(offset, ptr, true);
}

static RDMA_PutRetType serialize_put_fn(RDMA_PutRetType put_in) {
  fmt::print(
    "{}: serialize_put_fn: ptr={}, num_bytes={}\n",
    my_node, print_ptr(std::get<0>(put_in)), std::get<1>(put_in)
  );

  return RDMA_PutRetType{std::get<0>(put_in), rdma_num_elements*sizeof(double)};
}

int main(int argc, char** argv) {
  using namespace ::vt::rdma;

  CollectiveOps::initialize(argc, argv);

  my_node = theContext()->getNode();
  num_nodes = theContext()->getNumNodes();

  if (num_nodes < 4) {
    fmt::print("requires exactly 4 nodes\n");
    CollectiveOps::finalize();
    return 0;
  }

  rdma_state = new RDMAMsgType();

  my_handle = RDMACollectionManager::registerUnsizedCollection(num_elms);
  theRDMA()->associateGetFunction<RDMAMsgType,get_fn>(
    rdma_state, my_handle, get_fn, true
  );
  theRDMA()->associatePutFunction<RDMAMsgType,put_fn>(
    rdma_state, my_handle, put_fn, true
  );

  theCollective()->barrier();

  if (my_node == 0) {
    double* test_data = new double[rdma_num_elements];
    initData(10, test_data);
    // this message that causes a `get' races with the following `put'
    theMsg()->sendMsg<Msg, doGetHandler>(3, makeSharedMessage<Msg>());
    RDMACollectionManager::putElement(
      my_handle, 1, test_data, serialize_put_fn, []{
        fmt::print("{}: put finished\n", my_node);
        theMsg()->sendMsg<Msg, doGetHandler>(1, makeSharedMessage<Msg>());
      }
    );
  }

  fmt::print("{}: handle={}, create handle\n", my_node, my_handle);

  while (!rt->isTerminated()) {
    runScheduler();
  }

  CollectiveOps::finalize();

  return 0;
}
