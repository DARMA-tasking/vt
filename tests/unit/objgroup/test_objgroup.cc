/*
//@HEADER
// ************************************************************************
//
//                       test_objgroup.cc
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#include "test_objgroup_common.h"
#include <typeinfo>

namespace vt { namespace tests { namespace unit {

struct TestObjGroup : TestParallelHarness {

  void SetUp() override {
    TestParallelHarness::SetUp();
    auto const nb_nodes = vt::theContext()->getNumNodes();
    vtAssert(nb_nodes > 1, "Failure: require two or more nodes");
  }

  void TearDown() override {
    TestParallelHarness::TearDown();
    MyObjA::next_id = 0;
    MyObjB::next_id = 0;
  }

  template <int test>
  struct Verify {
    // check reduction results for scalar ops
    void operator()(SysMsg* msg) {
      auto const n = vt::theContext()->getNumNodes();
      auto const value = msg->getConstVal();

      switch (test) {
        case 1: EXPECT_EQ(value, n * (n - 1)/2); break;
        case 2: EXPECT_EQ(value, n * 4); break;
        case 3: EXPECT_EQ(value, n - 1); break;
        default: vtAbort("Failure: should not be reached"); break;
      }
    }
    // check reduction result for vector append
    void operator()(VecMsg* msg) {
      auto final_size = msg->getConstVal().vec_.size();
      auto n = vt::theContext()->getNumNodes();
      EXPECT_EQ(final_size, n * 2);
    }
  };
};

TEST_F(TestObjGroup, test_proxy_object_getter) {

  // create a proxy to a object group
  auto proxy = vt::theObjGroup()->makeCollective<MyObjA>();

  // check the uniqueness of the stored instance.
  // both object getters should return a pointer to the same instance.
  auto obj_ori = proxy.get();
  auto obj_get = vt::theObjGroup()->get(proxy);
  EXPECT_EQ(obj_ori->id_, obj_get->id_);

  // check that retrieved proxy and the original one are really the same.
  auto proxy_ori_bits = proxy.getProxy();
  auto proxy_get_bits = vt::theObjGroup()->proxy(obj_ori).getProxy();
  EXPECT_EQ(proxy_ori_bits, proxy_get_bits);

  // check that creating multiple proxies from a same object group type
  // will actually create different instance of this object group.
  auto proxy2 = vt::theObjGroup()->makeCollective<MyObjA>();
  auto proxy3 = vt::theObjGroup()->makeCollective<MyObjA>();
  auto obj1 = obj_ori;
  auto obj2 = proxy2.get();
  auto obj3 = proxy3.get();
  EXPECT_TRUE(obj1->id_ < obj2->id_);
  EXPECT_TRUE(obj2->id_ < obj3->id_);
}

TEST_F(TestObjGroup, test_proxy_update) {

  // create a proxy to a object group
  auto proxy = vt::theObjGroup()->makeCollective<MyObjA>();
  auto const obj1 = proxy.get();

  // update the group object for the proxy
  vt::theObjGroup()->update(proxy);

  // should normally have two distinct instances
  auto const obj2 = proxy.get();

  debug_print(
    objgroup, node,
    "test_proxy_update: obj1->id_:{}, obj2->id_:{}",
    obj1->id_, obj2->id_
  );
  EXPECT_TRUE(obj1->id_ < obj2->id_);
}

TEST_F(TestObjGroup, test_proxy_schedule) {

  // create a proxy to a object group
  auto proxy = vt::theObjGroup()->makeCollective<MyObjA>();

  // self-send a message and then broadcast
  auto my_node = vt::theContext()->getNode();
  proxy[my_node].send<MyMsg, &MyObjA::handler>();
  proxy.broadcast<MyMsg, &MyObjA::handler>();

  auto obj = proxy.get();
  debug_print(
    objgroup, node,
    "obj->recv:{} before running scheduler\n", obj->recv_
  );

  // run the object group scheduler to push
  // along postponed events
  while (vt::theObjGroup()->scheduler()) {}

  debug_print(
    objgroup, node,
    "obj->recv:{} after running scheduler\n", obj->recv_
  );
  EXPECT_EQ(obj->recv_, 2);
}

TEST_F(TestObjGroup, test_proxy_construct_send) {

  auto const my_node = vt::theContext()->getNode();

  // create object groups and retrieve proxies
  auto proxy1 = vt::theObjGroup()->makeCollective<MyObjA>();
  auto proxy2 = vt::theObjGroup()->makeCollective<MyObjB>(1);
  auto proxy3 = vt::theObjGroup()->makeCollective<MyObjA>();

  if (my_node == 0) {
    proxy1[0].send<MyMsg, &MyObjA::handler>();
    proxy1[0].send<MyMsg, &MyObjA::handler>();
    proxy1[1].send<MyMsg, &MyObjA::handler>();
  } else if (my_node == 1) {
    proxy2.broadcast<MyMsg, &MyObjB::handler>();
    proxy3[0].send<MyMsg, &MyObjA::handler>();
  }

  vt::theCollective()->barrier();

  // wait for all events to be processed (for all proxies)
  while (vt::theObjGroup()->scheduler()) {}

  // check received messages for each group
  auto obj1 = proxy1.get();
  auto obj2 = proxy2.get();
  auto obj3 = proxy3.get();

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
  auto const epoch = vt::theTerm()->makeEpochCollective();

  // create four proxy instances of a same object group type
  auto proxy1 = vt::theObjGroup()->makeCollective<MyObjA>();
  auto proxy2 = vt::theObjGroup()->makeCollective<MyObjA>();
  auto proxy3 = vt::theObjGroup()->makeCollective<MyObjA>();
  auto proxy4 = vt::theObjGroup()->makeCollective<MyObjA>();

  auto msg1 = vt::makeMessage<SysMsg>(my_node);
  auto msg2 = vt::makeMessage<SysMsg>(4);
  auto msg3 = vt::makeMessage<SysMsg>(my_node);
  auto msg4 = vt::makeMessage<VecMsg>(my_node);

  // Multiple reductions should not interfere each other, even if
  // performed by the same subset of nodes within the same epoch.
  // Proxies should be able to do perform reduction
  // on any valid operator and data type.
  using namespace vt::collective;

  proxy1.reduce<PlusOp<int>, Verify<1>>(msg1, epoch);
  proxy2.reduce<PlusOp<int>, Verify<2>>(msg2, epoch);
  proxy3.reduce< MaxOp<int>, Verify<3>>(msg3, epoch);
  proxy4.reduce<PlusOp<VectorPayload>, Verify<4>>(msg4, epoch);

  vt::theTerm()->finishedEpoch(epoch);
}

}}} // end namespace vt::tests::unit