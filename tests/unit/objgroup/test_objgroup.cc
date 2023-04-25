/*
//@HEADER
// *****************************************************************************
//
//                               test_objgroup.cc
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

#include "test_objgroup_common.h"
#include "test_helpers.h"
#include "vt/objgroup/manager.h"

#include <typeinfo>

namespace vt { namespace tests { namespace unit {

struct TestObjGroup : TestParallelHarness {

  void SetUp() override {
    TestParallelHarness::SetUp();
    SET_MIN_NUM_NODES_CONSTRAINT(2);
  }

  void TearDown() override {
    TestParallelHarness::TearDown();
    MyObjA::next_id = 0;
    MyObjB::next_id = 0;
  }

  static int32_t total_verify_expected_;

  template <int test>
  void verify(int value) {
    auto const n = vt::theContext()->getNumNodes();
    switch (test) {
    case 1: EXPECT_EQ(value, n * (n - 1)/2); break;
    case 2: EXPECT_EQ(value, n * 4); break;
    case 3: EXPECT_EQ(value, n - 1); break;
    default: vtAbort("Failure: should not be reached"); break;
    }
    total_verify_expected_++;
  }

  void verifyVec(VectorPayload vec) {
    auto final_size = vec.vec_.size();
    auto n = vt::theContext()->getNumNodes();
    EXPECT_EQ(final_size, n);
    total_verify_expected_++;
  }
};

/*static*/ int32_t TestObjGroup::total_verify_expected_ = 0;

TEST_F(TestObjGroup, test_construct) {

  // create object groups and retrieve proxies
  auto proxy1 = vt::theObjGroup()->makeCollective<MyObjA>("test_construct");
  auto proxy2 = vt::theObjGroup()->makeCollective<MyObjB>("test_construct", 1);
  auto proxy3 = vt::theObjGroup()->makeCollective<MyObjB>(new MyObjB(1), "test_construct");
  auto proxy4 = vt::theObjGroup()->makeCollective<MyObjB>(
    std::make_unique<MyObjB>(1), "test_construct"
  );

  // retrieve object group from proxies
  auto obj1 = proxy1.get();
  auto obj2 = proxy2.get();
  auto obj3 = proxy3.get();
  auto obj4 = proxy4.get();

  EXPECT_EQ(obj1->id_, 1);
  EXPECT_EQ(obj2->id_, 1);
  EXPECT_EQ(obj3->id_, 2);
  EXPECT_EQ(obj3->data_, obj2->data_);
  EXPECT_EQ(obj4->id_, 3);
  EXPECT_EQ(obj4->data_, obj3->data_);
}

TEST_F(TestObjGroup, test_proxy_object_getter) {

  // create a proxy to a object group
  auto proxy = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_object_getter");

  // check the uniqueness of the stored instance.
  // both object getters should return a pointer to the same instance.
  auto obj_ori = proxy.get();
  auto obj_get = vt::theObjGroup()->get(proxy);
  EXPECT_EQ(obj_ori->id_, obj_get->id_);

  // check that retrieved proxy and the original one are really the same.
  auto proxy_ori_bits = proxy.getProxy();
  auto proxy_get_bits = vt::theObjGroup()->getProxy(obj_ori).getProxy();
  EXPECT_EQ(proxy_ori_bits, proxy_get_bits);

  // check that creating multiple proxies from a same object group type
  // will actually create different instance of this object group.
  auto proxy2 = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_object_getter");
  auto proxy3 = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_object_getter");
  auto obj1 = obj_ori;
  auto obj2 = proxy2.get();
  auto obj3 = proxy3.get();
  EXPECT_TRUE(obj1->id_ < obj2->id_);
  EXPECT_TRUE(obj2->id_ < obj3->id_);
}

TEST_F(TestObjGroup, test_proxy_update) {

  // create a proxy to a object group
  auto proxy = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_update");
  auto const obj1 = proxy.get();
  auto const obj1_id = obj1->id_;
  auto const node = vt::theContext()->getNode();

  // update the group object for the proxy
  proxy[node].update(objgroup::proxy::ObjGroupReconstructTag);

  // should normally have two distinct instances
  auto const obj2 = proxy.get();

  vt_debug_print(
    normal, objgroup,
    "test_proxy_update: obj1->id_:{}, obj2->id_:{}",
    obj1_id, obj2->id_
  );
  EXPECT_TRUE(obj1_id < obj2->id_);
}

TEST_F(TestObjGroup, test_proxy_schedule) {
  auto my_node = vt::theContext()->getNode();
  auto num_nodes = vt::theContext()->getNumNodes();
  // create a proxy to a object group
  auto proxy = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_schedule");
  MyObjA *obj = nullptr;

  runInEpochCollective([&]{
    // self-send a message and then broadcast
    proxy[my_node].send<&MyObjA::handler>();
    proxy.broadcast<&MyObjA::handler>();

    obj = proxy.get();
    vt_debug_print(
      normal, objgroup,
      "obj->recv:{} before term\n", obj->recv_
    );
  });

  // check state to ensure all expected events executed
  vt_debug_print(
    normal, objgroup,
    "obj->recv:{} after term\n", obj->recv_
  );
  EXPECT_EQ(obj->recv_, num_nodes + 1);
}

TEST_F(TestObjGroup, test_proxy_callbacks) {
  auto const my_node = vt::theContext()->getNode();
  MyObjA* obj1 = nullptr;
  MyObjB* obj2 = nullptr;
  MyObjA* obj3 = nullptr;

  runInEpochCollective([&]{
    // create object groups and retrieve proxies
    auto proxy1 = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_callbacks");
    auto proxy2 = vt::theObjGroup()->makeCollective<MyObjB>("test_proxy_callbacks", 1);
    auto proxy3 = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_callbacks");

    if (my_node == 0) {
      proxy1[0].send<&MyObjA::handler>();
      proxy1[0].send<&MyObjA::handler>();
      proxy1[1].send<&MyObjA::handler>();
    } else if (my_node == 1) {
      proxy2.broadcast<&MyObjB::handler>();
      proxy3[0].send<&MyObjA::handler>();
    }

    // check received messages for each group
    obj1 = proxy1.get();
    obj2 = proxy2.get();
    obj3 = proxy3.get();
  });

  switch (my_node) {
  case 0:  EXPECT_EQ(obj1->recv_, 2); break;
  case 1:  EXPECT_EQ(obj1->recv_, 1); break;
  default: EXPECT_EQ(obj1->recv_, 0); break;
  }
  EXPECT_EQ(obj2->recv_, 1);
  EXPECT_EQ(obj3->recv_, my_node == 0 ? 1 : 0);
}

TEST_F(TestObjGroup, test_proxy_reduce) {
  auto const my_node = vt::theContext()->getNode();

  TestObjGroup::total_verify_expected_ = 0;

  vt::theCollective()->barrier();

  runInEpochCollective([&]{
    // create four proxy instances of a same object group type
    auto proxy1 = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_reduce");
    auto proxy2 = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_reduce");
    auto proxy3 = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_reduce");
    auto proxy4 = vt::theObjGroup()->makeCollective<MyObjA>("test_proxy_reduce");

    // Multiple reductions should not interfere each other, even if
    // performed by the same subset of nodes within the same epoch.
    // Proxies should be able to do perform reduction
    // on any valid operator and data type.
    using namespace vt::collective;

    proxy1.reduce<&TestObjGroup::verify<1>, PlusOp>(proxy1[0], my_node);
    proxy2.reduce<&TestObjGroup::verify<2>, PlusOp>(proxy1[0], 4);
    proxy3.reduce<&TestObjGroup::verify<3>, MaxOp>(proxy1[0], my_node);
    proxy4.reduce<&TestObjGroup::verifyVec, PlusOp>(
      proxy1[0], VectorPayload{std::vector<int>{my_node}}
    );
  });

  auto const root_node = 0;
  if (my_node == root_node) {
    EXPECT_EQ(TestObjGroup::total_verify_expected_, 4);
  }
}

TEST_F(TestObjGroup, test_proxy_invoke) {
  auto const& this_node = theContext()->getNode();

  auto proxy = vt::theObjGroup()->makeCollective<MyObjA>(
    "test_proxy_invoke"
  );


  // Message handler
  proxy[this_node].invoke<MyMsg, &MyObjA::handler>();

  EXPECT_EQ(proxy.get()->recv_, 1);

  // Non-message function
  auto const accumulate_result =
    proxy[this_node].invoke<&MyObjA::accumulateVec>(
      std::vector<int32_t>{2, 4, 5}
    );

  EXPECT_EQ(accumulate_result, 11);
  EXPECT_EQ(proxy.get()->recv_, 2);

  // Non-copyable
  std::unique_ptr<int32_t> s{};
  auto result = proxy[this_node].invoke<&MyObjA::modifyNonCopyableStruct>(
    std::move(s)
  );

  EXPECT_TRUE(result);
  EXPECT_EQ(*result, 10);
  EXPECT_EQ(proxy.get()->recv_, 3);
}

TEST_F(TestObjGroup, test_pending_send) {
  auto my_node = vt::theContext()->getNode();
  // create a proxy to a object group
  auto proxy = vt::theObjGroup()->makeCollective<MyObjA>("test_pending_send");
  auto obj = proxy.get();

  runInEpochCollective([&]{
    // self-send a message and then broadcast
    auto pending = proxy[my_node].send<&MyObjA::handler>();

    EXPECT_EQ(obj->recv_, 0);

    runInEpochCollective([&]{ pending.release(); } );
    EXPECT_EQ(obj->recv_, 1 );

    auto pending2 = proxy.broadcast<&MyObjA::handlerOnlyFromSelf>();

    EXPECT_EQ(obj->recv_, 1 );

    runInEpochCollective([&]{ pending2.release(); } );
    EXPECT_EQ(obj->recv_, 2);
  });

  // check state to ensure all expected events executed
  vt_debug_print(
    normal, objgroup,
    "obj->recv:{} after term\n", obj->recv_
  );
}

struct MyTestMsg : vt::Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_required();

  explicit MyTestMsg(NodeType const node) : node_{node} {
    ::fmt::print("Creating MyTestMsg on node: {}\n", node_);
  }

  MyTestMsg() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);

    s | node_;

    if (s.isPacking() || s.isUnpacking()) {
      was_serialized_ = true;
    }
  }

  NodeType fromNode() const { return node_; }

  bool wasSerialized() const { return was_serialized_; }

private:
  NodeType node_{-1};
  bool was_serialized_{false};
};

struct MyTestObj {
  void handleBroadcastMsg(MyTestMsg* msg) {
    auto const from_node = msg->fromNode();
    auto const to_node = theContext()->getNode();
    auto const was_serialized = msg->wasSerialized();

    ::fmt::print(
      "handleBroadcastMsg(): from node: {}, to node: {}, msg was serialized: {}\n",
      from_node, to_node, was_serialized
    );

    // Even if msg was sent locally, it is still serialized,
    // and the handler gets a fresh copy of it.
    if (from_node == to_node) {
      EXPECT_TRUE(was_serialized);
    }
  }

  void handleInvokeMsg(MyTestMsg* msg) {
    auto const from_node = msg->fromNode();
    auto const to_node = theContext()->getNode();
    auto const was_serialized = msg->wasSerialized();

    ::fmt::print(
      "handleInvokeMsg(): from node: {}, to node: {}, msg was serialized: {}\n",
      from_node, to_node, was_serialized
    );

    // invoke() shouldn't serialize message.
    if (from_node == to_node) {
      EXPECT_FALSE(was_serialized);
    }
  }
};

TEST_F(TestObjGroup, test_objgroup_serialize_when_broadcast) {
  runInEpochCollective([]{
    auto const proxy = vt::theObjGroup()->makeCollective<MyTestObj>(
      "test_objgroup_serialize_when_broadcast"
    );
    auto const this_node = theContext()->getNode();
    auto msg = ::vt::makeMessage<MyTestMsg>(this_node);

    proxy.broadcastMsg<MyTestMsg, &MyTestObj::handleBroadcastMsg>(msg);
  });
}

TEST_F(TestObjGroup, test_objgroup_dont_serialize_when_invoke) {
  runInEpochCollective([]{
    auto const proxy = vt::theObjGroup()->makeCollective<MyTestObj>(
      "test_objgroup_dont_serialize_when_invoke"
    );
    auto const this_node = theContext()->getNode();
    auto msg = ::vt::makeMessage<MyTestMsg>(this_node);

    proxy[this_node].invoke<MyTestMsg, &MyTestObj::handleInvokeMsg>(this_node);
  });
}

struct MyTestLabelObj {};

TEST_F(TestObjGroup, test_objgroup_labels) {
  runInEpochCollective([]{
    std::string const label = "test_objgroup_labels";
    auto const proxy = vt::theObjGroup()->makeCollective<MyTestLabelObj>(label);
    auto const proxyLabel = vt::theObjGroup()->getLabel(proxy);

    EXPECT_EQ(label, proxyLabel);
  });
}

}}} // end namespace vt::tests::unit
