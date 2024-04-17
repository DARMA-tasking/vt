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
#include "common/timers.h"
#include "vt/collective/collective_alg.h"
#include "vt/configs/error/config_assert.h"
#include "vt/configs/error/hard_error.h"
#include "vt/context/context.h"
#include "vt/messaging/message/shared_message.h"
#include "vt/scheduler/scheduler.h"
#include <chrono>
#include <cstdint>
#include <unistd.h>
#include <vt/collective/collective_ops.h>
#include <vt/objgroup/manager.h>
#include <vt/vrt/collection/manager.h>
#include <vt/messaging/active.h>

#include <fmt-vt/core.h>

#include <array>

using namespace vt;
using namespace vt::tests::perf::common;

static constexpr std::array<size_t, 7> const payloadSizes = {
  1, 64, 128, 2048, 16384, 524288, 268435456};

vt::EpochType the_epoch = vt::no_epoch;
bool send_done = false;

struct SendTest : PerfTestHarness { };

////////////////////////////////////////
//////////////// RAW MPI ///////////////
////////////////////////////////////////

VT_PERF_TEST(SendTest, test_send) {
  auto const thisNode = vt::theContext()->getNode();

  if (thisNode == 0) {
    vt::theTerm()->disableTD();
  }

  auto const lastNode = theContext()->getNumNodes() - 1;

  auto const prevNode = (thisNode - 1 + num_nodes_) % num_nodes_;
  auto const nextNode = (thisNode + 1) % num_nodes_;
  int data = thisNode;

  for (auto size : payloadSizes) {
    std::vector<int32_t> dataVec(size, data);
    std::vector<int32_t> recvData(size, data);

    StartTimer(fmt::format("Payload size {}", size));

    MPI_Request request;
    MPI_Irecv(
      &recvData[0], size, MPI_INT, prevNode, 0, MPI_COMM_WORLD, &request);
    MPI_Send(&dataVec[0], size, MPI_INT, nextNode, 0, MPI_COMM_WORLD);

    MPI_Wait(&request, MPI_STATUS_IGNORE);

    StopTimer(fmt::format("Payload size {}", size));
  }

  if (vt::theContext()->getNode() == 0) {
    vt::theTerm()->enableTD();
  }
}

////////////////////////////////////////
///////////// OBJECT GROUP /////////////
////////////////////////////////////////

struct NodeObj {
  struct PingMsg : Message {
    using MessageParentType = vt::Message;
    vt_msg_serialize_required();
    std::vector<int32_t> vec_;
    DurationMilli start_;

    PingMsg() : Message() { }
    explicit PingMsg(size_t size)
      : Message(),
        start_(std::chrono::steady_clock::now().time_since_epoch()) {
      vec_.resize(size, vt::theContext()->getNode());
    }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      MessageParentType::serialize(s);
      s | vec_;
      s | start_;
    }
  };

  void sendHandler(NodeObj::PingMsg* msg) {
    test_obj_->AddResult(
      {fmt::format("ObjGroup Payload size {}", msg->vec_.size()),
       (DurationMilli{std::chrono::steady_clock::now().time_since_epoch()} -
        msg->start_)
         .count()});
    send_done = true;
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

  if (theContext()->getNode() == 0) {
    theTerm()->disableTD();
  }

  auto const thisNode = vt::theContext()->getNode();
  auto const lastNode = theContext()->getNumNodes() - 1;

  auto const prevNode = (thisNode - 1 + num_nodes_) % num_nodes_;
  auto const nextNode = (thisNode + 1) % num_nodes_;

  for (auto size : payloadSizes) {
    theCollective()->barrier();

    grp_proxy[nextNode].send<&NodeObj::sendHandler>(size);
    theSched()->runSchedulerWhile([]{ return !send_done; });

    send_done = false;
  }

  if (vt::theContext()->getNode() == 0) {
    vt::theTerm()->enableTD();
  }
}

////////////////////////////////////////
////////////// COLLECTION //////////////
////////////////////////////////////////

struct Hello : vt::Collection<Hello, vt::Index1D> {
  struct TestDataMsg : vt::CollectionMessage<Hello> {
    vt_msg_serialize_required();
    using MessageParentType = vt::CollectionMessage<Hello>;
    //  vt_msg_serialize_if_needed_by_parent_or_type1(vt::IdxBase);
    TestDataMsg() = default;
    explicit TestDataMsg(size_t size) {
      vec_.resize(size, theContext()->getNode());
    }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      MessageParentType::serialize(s);
      s | vec_;
    }

    std::vector<int32_t> vec_ = {};
  };
  Hello() = default;

  void Handler(TestDataMsg* msg) { handled_ = true; }

  void CheckHandledAndReset() {
    vtAssert(
      handled_, fmt::format("[{}] Recv not run!", theContext()->getNode()));

    handled_ = false;
  }

  bool handled_ = false;
};

VT_PERF_TEST(SendTest, test_collection_send) {
  auto range = vt::Index1D(int32_t{num_nodes_});
  auto proxy = vt::makeCollection<Hello>("hello_world_collection_reduce")
                 .bounds(range)
                 .bulkInsert()
                 .wait();

  auto const thisNode = vt::theContext()->getNode();
  auto const nextNode = (thisNode + 1) % num_nodes_;

  for (auto size : payloadSizes) {
    auto msg = vt::makeMessage<Hello::TestDataMsg>(size_t{size});
    StartTimer(fmt::format("Collection Payload size {}", size));

    vt::runInEpochCollective(
      [&] { proxy[nextNode].sendMsg<&Hello::Handler>(msg); });

    StopTimer(fmt::format("Collection Payload size {}", size));

    vt::runInEpochCollective(
      [&] { proxy[thisNode].invoke<&Hello::CheckHandledAndReset>(); });
  }
}

VT_PERF_TEST_MAIN()
