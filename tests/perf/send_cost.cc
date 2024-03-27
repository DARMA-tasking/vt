/*
//@HEADER
// *****************************************************************************
//
//                                 send_cost.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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
#include "common/test_harness.h"
#include "vt/collective/collective_alg.h"
#include "vt/configs/error/config_assert.h"
#include "vt/configs/error/hard_error.h"
#include "vt/context/context.h"
#include "vt/messaging/message/shared_message.h"
#include "vt/scheduler/scheduler.h"
#include <cstdint>
#include <vt/collective/collective_ops.h>
#include <vt/objgroup/manager.h>
#include <vt/vrt/collection/manager.h>
#include <vt/messaging/active.h>

#include <fmt-vt/core.h>

#include <array>

using namespace vt;
using namespace vt::tests::perf::common;

// static constexpr std::array<size_t, 7> const payloadSizes = {
//   1, 64, 128, 2048, 16384, 524288, 268435456};

static constexpr std::array<size_t, 3> const payloadSizes = {1, 64, 128};

vt::EpochType the_epoch = vt::no_epoch;

struct SendTest : PerfTestHarness { };

////////////////////////////////////////
//////////////// RAW MPI ///////////////
////////////////////////////////////////

// VT_PERF_TEST(SendTest, test_send) {
//   auto const thisNode = vt::theContext()->getNode();

//   if (thisNode == 0) {
//     vt::theTerm()->disableTD();
//   }

//   auto const lastNode = theContext()->getNumNodes() - 1;

//   auto const prevNode = (thisNode - 1 + num_nodes_) % num_nodes_;
//   auto const nextNode = (thisNode + 1) % num_nodes_;
//   int data = thisNode;

//   for (auto size : payloadSizes) {
//     std::vector<int32_t> dataVec(size, data);
//     std::vector<int32_t> recvData(size, data);

//     StartTimer(fmt::format("Payload size {}", size));

//     MPI_Request request;
//     MPI_Irecv(
//       &recvData[0], size, MPI_INT, prevNode, 0, MPI_COMM_WORLD, &request);
//     MPI_Send(&dataVec[0], size, MPI_INT, nextNode, 0, MPI_COMM_WORLD);

//     MPI_Wait(&request, MPI_STATUS_IGNORE);

//     StopTimer(fmt::format("Payload size {}", size));
//   }

//   if (vt::theContext()->getNode() == 0) {
//     vt::theTerm()->enableTD();
//   }
// }

////////////////////////////////////////
///////////// OBJECT GROUP /////////////
////////////////////////////////////////

struct NodeObj {
  struct PingMsg : Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_required();
    std::vector<int32_t> vec_;

    PingMsg() : Message() { }
    explicit PingMsg(std::vector<int32_t> data) : Message() { vec_ = data; }

    explicit PingMsg(size_t size) : Message() {
      vec_.resize(size, vt::theContext()->getNode() + 1);
    }
    PingMsg(size_t size, int32_t val) : Message() { vec_.resize(size, val); }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      MessageParentType::serialize(s);
      s | vec_;
    }
  };

  void rightHalf(NodeObj::PingMsg* msg) {
    for (int i = 0; i < msg->vec_.size(); i++) {
      data_[(data_.size() / 2) + i] += msg->vec_[i];
    }
  }

  void rightHalfComplete(NodeObj::PingMsg* msg) {
    for (int i = 0; i < msg->vec_.size(); i++) {
      data_[(data_.size() / 2) + i] = msg->vec_[i];
    }
  }

  void leftHalf(NodeObj::PingMsg* msg) {
    for (int i = 0; i < msg->vec_.size(); i++) {
      data_[i] += msg->vec_[i];
    }
  }

  void leftHalfComplete(NodeObj::PingMsg* msg) {
    for (int i = 0; i < msg->vec_.size(); i++) {
      data_[i] = msg->vec_[i];
    }
  }

  void sendHandler(NodeObj::PingMsg* msg) {
    uint32_t start = isEven_ ? 0 : data_.size() / 2;
    uint32_t end = isEven_ ? data_.size() / 2 : data_.size();
    for (int i = 0; start < end; start++) {
      data_[start] += msg->vec_[i++];
    }
  }

  void reducedHan(NodeObj::PingMsg* msg) {
    for (int i = 0; i < msg->vec_.size(); i++) {
      data_[data_.size() / 2 + i] = msg->vec_[i];
    }
  }

  explicit NodeObj(SendTest* test_obj) : test_obj_(test_obj) {
    data_.resize(268435456, theContext()->getNode() + 1);
    isEven_ = theContext()->getNode() % 2 == 0;
  }

  void initialize() { proxy_ = vt::theObjGroup()->getProxy<NodeObj>(this); }
  void printData() {
    for (auto v : data_) {
      fmt::print("[{}] {}\n", theContext()->getNode(), v);
    }
  }

  void printDataFinal(std::vector<int32_t> vec) {
    for (auto v : vec) {
      // fmt::print("[{}] {}\n", theContext()->getNode(), v);
    }
    handled_ = true;
  }

  bool isEven_ = false;
  bool handled_ = false;
  SendTest* test_obj_ = nullptr;
  vt::objgroup::proxy::Proxy<NodeObj> proxy_ = {};

  std::vector<int32_t> data_ = {};
};

static inline int opal_hibit(int value, int start) {
  unsigned int mask;
  --start;
  mask = 1 << start;

  for (; start >= 0; --start, mask >>= 1) {
    if (value & mask) {
      break;
    }
  }

  return start;
}

// VT_PERF_TEST(SendTest, test_allreduce) {
//   auto grp_proxy =
//     vt::theObjGroup()->makeCollective<NodeObj>("test_objgroup_send", this);
//   grp_proxy[my_node_].invoke<&NodeObj::initialize>();

//   if (theContext()->getNode() == 0) {
//     theTerm()->disableTD();
//   }

//   vt::runInEpochCollective([=] {
//     grp_proxy.allreduce<&NodeObj::printDataFinal, vt::collective::PlusOp>(
//       std::vector<int32_t>(268435456, theContext()->getNode() + 1));
//   });

//   vtAssert(grp_proxy[theContext()->getNode()].get()->handled_, "");
//   if (vt::theContext()->getNode() == 0) {
//     vt::theTerm()->enableTD();
//   }
// }

VT_PERF_TEST(SendTest, test_objgroup_send) {
  auto grp_proxy =
    vt::theObjGroup()->makeCollective<NodeObj>("test_objgroup_send", this);
  grp_proxy[my_node_].invoke<&NodeObj::initialize>();

  if (theContext()->getNode() == 0) {
    theTerm()->disableTD();
  }

  auto const thisNode = vt::theContext()->getNode();
  auto const lastNode = theContext()->getNumNodes() - 1;

  int nsteps = 2;
  auto nprocs_rem = 0;
  size_t count = 32; //1 << 6;
  auto* buf = (int32_t*)malloc(sizeof(int32_t) * count);
  auto nprocs_pof2 = 1 << nsteps;
  auto rank = theContext()->getNode();
  auto vrank = theContext()->getNode();
  int *rindex = NULL, *rcount = NULL, *sindex = NULL, *scount = NULL;
  rindex = (int*)malloc(sizeof(*rindex) * nsteps);
  sindex = (int*)malloc(sizeof(*sindex) * nsteps);
  rcount = (int*)malloc(sizeof(*rcount) * nsteps);
  scount = (int*)malloc(sizeof(*scount) * nsteps);

  int step = 0;
  auto wsize = count;
  sindex[0] = rindex[0] = 0;

  fmt::print(
    "[{}] Starting with numNodes = {} dataSize = {} \n", rank, nprocs_pof2,
    count);
  for (int mask = 1; mask < nprocs_pof2; mask <<= 1) {
    /*
             * On each iteration: rindex[step] = sindex[step] -- beginning of the
             * current window. Length of the current window is storded in wsize.
             */
    int vdest = vrank ^ mask;
    /* Translate vdest virtual rank to real rank */
    int dest = (vdest < nprocs_rem) ? vdest * 2 : vdest + nprocs_rem;

    if (rank < dest) {
      /*
                 * Recv into the left half of the current window, send the right
                 * half of the window to the peer (perform reduce on the left
                 * half of the current window)
                 */
      rcount[step] = wsize / 2;
      scount[step] = wsize - rcount[step];
      sindex[step] = rindex[step] + rcount[step];
    } else {
      /*
                 * Recv into the right half of the current window, send the left
                 * half of the window to the peer (perform reduce on the right
                 * half of the current window)
                 */
      scount[step] = wsize / 2;
      rcount[step] = wsize - scount[step];
      rindex[step] = sindex[step] + scount[step];
    }

    /* Send part of data from the rbuf, recv into the tmp_buf */
    // err = ompi_coll_base_sendrecv(
    //   (char*)rbuf + (ptrdiff_t)sindex[step] * extent, scount[step], dtype, dest,
    //   MCA_COLL_BASE_TAG_ALLREDUCE,
    //   (char*)tmp_buf + (ptrdiff_t)rindex[step] * extent, rcount[step], dtype,
    //   dest, MCA_COLL_BASE_TAG_ALLREDUCE, comm, MPI_STATUS_IGNORE, rank);

    fmt::print(
      "[{}] Sending to rank {} data + offset({}) with count = {}\nPerforming "
      "reduce on data starting with offset {} and count {}\n",
      rank, dest, sindex[step], scount[step], rindex[step], rcount[step]);

    theCollective()->barrier();
    /* Local reduce: rbuf[] = tmp_buf[] <op> rbuf[] */
    // ompi_op_reduce(
    //   op, (char*)tmp_buf + (ptrdiff_t)rindex[step] * extent,
    //   (char*)rbuf + (ptrdiff_t)rindex[step] * extent, rcount[step], dtype);

    /* Move the current window to the received message */
    if (step + 1 < nsteps) {
      rindex[step + 1] = rindex[step];
      sindex[step + 1] = rindex[step];
      wsize = rcount[step];
      step++;
    }
  }
  step = nsteps - 1;

  for (int mask = nprocs_pof2 >> 1; mask > 0; mask >>= 1) {
    int vdest = vrank ^ mask;
    /* Translate vdest virtual rank to real rank */
    int dest = (vdest < nprocs_rem) ? vdest * 2 : vdest + nprocs_rem;

    /*
             * Send rcount[step] elements from rbuf[rindex[step]...]
             * Recv scount[step] elements to rbuf[sindex[step]...]
             */
    // err = ompi_coll_base_sendrecv((char *)rbuf + (ptrdiff_t)rindex[step] * extent,
    //                               rcount[step], dtype, dest,
    //                               MCA_COLL_BASE_TAG_ALLREDUCE,
    //                               (char *)rbuf + (ptrdiff_t)sindex[step] * extent,
    //                               scount[step], dtype, dest,
    //                               MCA_COLL_BASE_TAG_ALLREDUCE, comm,
    //                               MPI_STATUS_IGNORE, rank);
    fmt::print(
      "[{}] Sending to rank {} data + offset({}) with count = {}\nReceiving "
      "data starting with offset {} and count {}\n",
      rank, dest, rindex[step], rcount[step], sindex[step], scount[step]);
    step--;
  }

  if (vt::theContext()->getNode() == 0) {
    vt::theTerm()->enableTD();
  }
}

VT_PERF_TEST_MAIN()
/*
//@HEADER
// *****************************************************************************
//
//                                 send_cost.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#include "common/test_harness.h"
#include "vt/collective/collective_alg.h"
#include "vt/configs/error/config_assert.h"
#include "vt/context/context.h"
#include "vt/scheduler/scheduler.h"
#include <cstdint>
#include <unistd.h>
#include <vt/collective/collective_ops.h>
#include <vt/objgroup/manager.h>
#include <vt/vrt/collection/manager.h>
#include <vt/messaging/active.h>

#include <array>

using namespace vt;
using namespace vt::tests::perf::common;

static constexpr std::array<size_t, 9> const payloadSizes = {
  1, 64, 128, 2048, 16384, 32768, 524288, 1048576, 2097152};

bool obj_send_done = false;
bool col_send_done = false;

struct SendTest : PerfTestHarness {
  SendTest() {
    DisableGlobalTimer();
  }
};

////////////////////////////////////////
//////////////// RAW MPI ///////////////
////////////////////////////////////////

VT_PERF_TEST(SendTest, test_send) {
  auto const thisNode = vt::theContext()->getNode();

  auto const prevNode = (thisNode - 1 + num_nodes_) % num_nodes_;
  auto const nextNode = (thisNode + 1) % num_nodes_;
  int data = thisNode;

  for (auto size : payloadSizes) {
    std::vector<int32_t> dataVec(size, data);

    StartTimer(fmt::format("Payload size {}", size));

    std::vector<int32_t> recvData(size, data);
    MPI_Request request;
    MPI_Irecv(
      &recvData[0], size, MPI_INT, prevNode, 0, MPI_COMM_WORLD, &request);
    MPI_Send(&dataVec[0], size, MPI_INT, nextNode, 0, MPI_COMM_WORLD);

    MPI_Wait(&request, MPI_STATUS_IGNORE);

    StopTimer(fmt::format("Payload size {}", size));
  }
}

////////////////////////////////////////
///////////// OBJECT GROUP /////////////
////////////////////////////////////////

struct NodeObj {
  struct ObjGroupMsg : Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_required();

    ObjGroupMsg() : Message() { }

    ~ObjGroupMsg() {
      if (owning_) {
        delete payload_;
      }
    }

    explicit ObjGroupMsg(std::vector<int32_t>* payload)
      : Message(),
        payload_(payload),
        start_(std::chrono::steady_clock::now().time_since_epoch()) { }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      MessageParentType::serialize(s);

      if (s.isUnpacking()) {
        payload_ = new std::vector<int32_t>();
        owning_ = true;
      }

      s | *payload_;
      s | start_;
    }

    std::vector<int32_t>* payload_ = nullptr;
    bool owning_ = false;
    DurationMilli start_ = {};
  };

  void sendHandler(NodeObj::ObjGroupMsg* msg) {
    auto now = std::chrono::steady_clock::now();

    test_obj_->AddResult(
      {fmt::format("ObjGroup Payload size {}", msg->payload_->size()),
       (DurationMilli{now.time_since_epoch()} - msg->start_).count()});

    obj_send_done = true;
  }

  explicit NodeObj(SendTest* test_obj) : test_obj_(test_obj) { }

  void initialize() { proxy_ = vt::theObjGroup()->getProxy<NodeObj>(this); }

  bool handled_ = false;
  SendTest* test_obj_ = nullptr;
  vt::objgroup::proxy::Proxy<NodeObj> proxy_ = {};
};

VT_PERF_TEST(SendTest, test_objgroup_send) {
  auto grp_proxy =
    vt::theObjGroup()->makeCollective<NodeObj>("test_objgroup_send", this);
  grp_proxy[my_node_].invoke<&NodeObj::initialize>();

  auto const thisNode = vt::theContext()->getNode();
  auto const nextNode = (thisNode + 1) % num_nodes_;

  for (auto size : payloadSizes) {
    auto* payload = new std::vector<int32_t>();
    payload->resize(size, thisNode);

    theCollective()->barrier();

    grp_proxy[nextNode].send<&NodeObj::sendHandler>(payload);
    theSched()->runSchedulerWhile([] { return !obj_send_done; });

    obj_send_done = false;

    delete payload;
  }
}

////////////////////////////////////////
////////////// COLLECTION //////////////
////////////////////////////////////////

struct Hello : vt::Collection<Hello, vt::Index1D> {
  struct TestDataMsg : vt::CollectionMessage<Hello> {
    vt_msg_serialize_required();
    using MessageParentType = vt::CollectionMessage<Hello>;
    TestDataMsg() = default;
    ~TestDataMsg() {
      if (owning_) {
        delete payload_;
      }
    }
    explicit TestDataMsg(std::vector<int32_t>* payload)
      : start_(std::chrono::steady_clock::now().time_since_epoch()),
        payload_(payload) { }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      MessageParentType::serialize(s);
      s | start_;

      if (s.isUnpacking()) {
        owning_ = true;
        payload_ = new std::vector<int32_t>();
      }

      s | *payload_;
    }

    DurationMilli start_ = {};
    std::vector<int32_t>* payload_ = nullptr;
    bool owning_ = false;
  };

  Hello() = default;

  void Handler(TestDataMsg* msg) {
    auto now = std::chrono::steady_clock::now();
    test_obj_->AddResult(
      {fmt::format("Collection Payload size {}", msg->payload_->size()),
       (DurationMilli{now.time_since_epoch()} - msg->start_).count()});
    col_send_done = true;
  }

  SendTest* test_obj_ = nullptr;
};

VT_PERF_TEST(SendTest, test_collection_send) {
  auto range = vt::Index1D(int32_t{num_nodes_});
  auto proxy = vt::makeCollection<Hello>("test_collection_send")
                 .bounds(range)
                 .bulkInsert()
                 .wait();

  auto const thisNode = vt::theContext()->getNode();
  auto const nextNode = (thisNode + 1) % num_nodes_;

  proxy[thisNode].tryGetLocalPtr()->test_obj_ = this;

  for (auto size : payloadSizes) {
    std::vector<int32_t> payload(size, thisNode);

    theCollective()->barrier();
    proxy[nextNode].send<&Hello::Handler>(&payload);

    // We run 1 coll elem per node, so it should be ok
    theSched()->runSchedulerWhile([] { return !col_send_done; });
    col_send_done = false;
  }
}

VT_PERF_TEST_MAIN()
