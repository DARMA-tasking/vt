/*
//@HEADER
// *****************************************************************************
//
//                             test_lb_data_comm.cc
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

#include "test_parallel_harness.h"

#include <vt/transport.h>
#include <vt/utils/json/json_appender.h>
#include <vt/utils/json/json_reader.h>
#include <vt/utils/json/decompression_input_container.h>
#include <vt/utils/json/input_iterator.h>

#include <nlohmann/json.hpp>

#if vt_check_enabled(lblite)

namespace vt { namespace tests { namespace unit { namespace comm {

/**
 *  --------------------------------------
 *      Communication Record Summary
 *  --------------------------------------
 *  -------- || --------- Sender ---------
 *  Receiver || ColT | ObjGroup | Handler
 *  --------------------------------------
 *  ColT     ||  R   |    R     |    R
 *  ObjGroup ||  S   |    S     |    S
 *  Handler  ||  S   |    S     |    S
 *  --------------------------------------
 */

using TestLBDataComm = TestParallelHarness;
using LBDataHolder = vt::vrt::collection::balance::LBDataHolder;

LBDataHolder getLBDataForPhase(vt::PhaseType phase) {
  using JSONAppender = vt::util::json::Appender<std::stringstream>;
  using vt::util::json::DecompressionInputContainer;
  using vt::vrt::collection::balance::LBDataHolder;
  using json = nlohmann::json;
  std::stringstream ss{std::ios_base::out | std::ios_base::in};
  nlohmann::json metadata;
  metadata["type"] = "LBDatafile";
  auto ap = std::make_unique<JSONAppender>(
    "phases", metadata, std::move(ss), true
  );
  auto j = vt::theNodeLBData()->getLBData()->toJson(phase);
  ap->addElm(*j);
  ss = ap->finish();
  auto c = DecompressionInputContainer{
    DecompressionInputContainer::AnyStreamTag{}, std::move(ss)
  };
  return LBDataHolder{json::parse(c)};
}

namespace {

auto constexpr dim1 = 64;
auto constexpr num_sends = 4;

struct MyCol : vt::Collection<MyCol, vt::Index1D> { };
struct MyMsg : vt::CollectionMessage<MyCol> { double x, y, z; };
struct MyObjMsg : vt::Message { double x, y, z; };
struct ElementInfo {
  using MapType = std::map<vt::Index1D, vt::elm::ElementIDStruct>;
  ElementInfo() = default;
  ElementInfo(vt::Index1D idx, vt::elm::ElementIDStruct elm_id)
    : elms(std::map<vt::Index1D, vt::elm::ElementIDStruct>{{idx, elm_id}})
  {};
  explicit ElementInfo(MapType in_map) : elms(in_map) { }
  friend ElementInfo operator+(ElementInfo a1, ElementInfo const& a2) {
    for (auto&& x : a2.elms) a1.elms.insert(x);
    return a1;
  }
  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | elms;
  }
  MapType elms;
};
struct ColProxyMsg : vt::Message {
  using ProxyType = vt::CollectionProxy<MyCol>;
  ColProxyMsg(ProxyType in_proxy) : proxy(in_proxy) { }
  ProxyType proxy;
};
struct ProxyMsg;
struct MyObj {
  void dummyHandler(MyObjMsg* msg) {}
  void simulateObjGroupColTSends(ColProxyMsg* msg);
  void simulateObjGroupObjGroupSends(ProxyMsg* msg);
  void simulateObjGroupHandlerSends(MyMsg* msg);
};
struct ProxyMsg : vt::CollectionMessage<MyCol> {
  using ProxyType = vt::objgroup::proxy::Proxy<MyObj>;
  ProxyMsg(ProxyType in_proxy) : obj_proxy(in_proxy) { }
  ProxyType obj_proxy;
};

void handler2(MyCol* col, MyMsg*) {}

void MyObj::simulateObjGroupColTSends(ColProxyMsg* msg) {
  auto proxy = msg->proxy;
  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  for (int i = 0; i < dim1; i++) {
    vt::Index1D idx{i};
    auto node = vt::theCollection()->getMappedNode(proxy, idx);
    if (node == next) {
      for (int j = 0; j < num_sends; j++) {
        proxy(idx).template send<handler2>();
      }
    }
  }
}

void MyObj::simulateObjGroupObjGroupSends(ProxyMsg* msg) {
  auto obj_proxy = msg->obj_proxy;
  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  for (int i = 0; i < num_sends; i++) {
    obj_proxy[next].template send<MyObjMsg, &MyObj::dummyHandler>();
  }
}

void simulateColTColTSends(MyCol* col) {
  auto proxy = col->getCollectionProxy();
  auto index = col->getIndex();
  auto next = (index.x() + 1) % dim1;
  for (int i = 0; i < num_sends; i++) {
    proxy(next).template send<handler2>();
  }
}

void simulateColTObjGroupSends(MyCol* col, ProxyMsg* msg) {
  auto obj_proxy = msg->obj_proxy;
  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  for (int i = 0; i < num_sends; i++) {
    obj_proxy[next].template send<MyObjMsg, &MyObj::dummyHandler>();
  }
}

void bareDummyHandler(MyObjMsg* msg) {}

void simulateColTHandlerSends(MyCol* col) {
  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  for (int i = 0; i < num_sends; i++) {
    auto msg2 = makeMessage<MyObjMsg>();
    vt::theMsg()->sendMsg<bareDummyHandler>(next, msg2);
  }
}

void MyObj::simulateObjGroupHandlerSends(MyMsg* msg) {
  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  for (int i = 0; i < num_sends; i++) {
    auto msg2 = makeMessage<MyObjMsg>();
    vt::theMsg()->sendMsg<bareDummyHandler>(next, msg2);
  }
}

void simulateHandlerColTSends(ColProxyMsg* msg) {
  auto proxy = msg->proxy;
  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  for (int i = 0; i < dim1; i++) {
    vt::Index1D idx{i};
    auto node = vt::theCollection()->getMappedNode(proxy, idx);
    if (node == next) {
      for (int j = 0; j < num_sends; j++) {
        proxy(idx).template send<handler2>();
      }
    }
  }
}

void simulateHandlerObjGroupSends(ProxyMsg* msg) {
  auto obj_proxy = msg->obj_proxy;
  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  for (int i = 0; i < num_sends; i++) {
    obj_proxy[next].template send<MyObjMsg, &MyObj::dummyHandler>();
  }
}

void simulateHandlerHandlerSends(MyMsg* msg) {
  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  for (int i = 0; i < num_sends; i++) {
    auto msg2 = makeMessage<MyObjMsg>();
    vt::theMsg()->sendMsg<bareDummyHandler>(next, msg2);
  }
}

std::map<vt::Index1D, vt::elm::ElementIDStruct> all_elms;

auto idxToElmID = [](vt::Index1D i) -> vt::elm::ElementIDType {
  return all_elms.find(i)->second.id;
};

void recvElementIDs(ElementInfo info) { all_elms = info.elms; }

void doReduce(MyCol* col) {
  auto proxy = col->getCollectionProxy();
  auto index = col->getIndex();
  auto cb = theCB()->makeBcast<recvElementIDs>();
  proxy.reduce<vt::collective::PlusOp>(cb, ElementInfo{index, col->getElmID()});
}

// ColT -> ColT, expected communication edge on receive side
TEST_F(TestLBDataComm, test_lb_data_comm_col_to_col_send) {
  auto range = vt::Index1D{dim1};
  auto proxy = vt::makeCollection<MyCol>("test_lb_stats_comm_col_to_col_send")
    .bounds(range)
    .bulkInsert()
    .wait();

  vt::runInEpochCollective("simulateColTColTSends", [&]{
    for (int i = 0; i < dim1; i++) {
      if (proxy(i).tryGetLocalPtr()) {
        proxy(i).invoke<simulateColTColTSends>();
      }
    }
  });

  vt::thePhase()->nextPhaseCollective();

  vt::runInEpochCollective("doReduce", [&]{
    for (int i = 0; i < dim1; i++) {
      if (proxy(i).tryGetLocalPtr()) {
        proxy(i).invoke<doReduce>();
      }
    }
  });

  vt::PhaseType phase = 0;
  auto lbdh = getLBDataForPhase(phase);
  auto& comm = lbdh.node_comm_;

  // Check that communication exists on the receive side as expected
  for (int i = 0; i < dim1; i++) {
    vt::Index1D idx{i};
    auto prev_i = i-1 >= 0 ? i-1 : dim1-1;
    vt::Index1D prev_idx{prev_i};
    if (proxy(i).tryGetLocalPtr()) {
      bool found = false;
      for (auto&& x : comm) {
        for (auto&& y : x.second) {
          auto key = y.first;
          auto vol = y.second;
          if (key.to_.id == idxToElmID(idx)) {
            EXPECT_TRUE(key.to_.isMigratable());
            EXPECT_TRUE(key.from_.isMigratable());
            EXPECT_EQ(key.from_.id, idxToElmID(prev_idx));
            EXPECT_EQ(vol.bytes, sizeof(MyMsg) * num_sends);
            EXPECT_EQ(vol.messages, num_sends);
            found = true;
          }
        }
      }
      EXPECT_TRUE(found);
    }
  }
}

// ColT -> ObjGroup, expected communication edge on send side
TEST_F(TestLBDataComm, test_lb_data_comm_col_to_objgroup_send) {
  auto range = vt::Index1D{dim1};
  auto proxy = vt::makeCollection<MyCol>("test_lb_stats_comm_col_to_objgroup_send")
    .bounds(range)
    .bulkInsert()
    .wait();

  auto obj_proxy = vt::theObjGroup()->makeCollective<MyObj>(
    "test_lb_stats_comm_col_to_objgroup_send"
  );

  vt::runInEpochCollective("simulateColTObjGroupSends", [&]{
    for (int i = 0; i < dim1; i++) {
      if (proxy(i).tryGetLocalPtr()) {
        proxy(i).invoke<simulateColTObjGroupSends>(obj_proxy);
      }
    }
  });

  vt::thePhase()->nextPhaseCollective();

  vt::runInEpochCollective("doReduce", [&]{
    for (int i = 0; i < dim1; i++) {
      if (proxy(i).tryGetLocalPtr()) {
        proxy(i).invoke<doReduce>();
      }
    }
  });

  vt::PhaseType phase = 0;
  auto lbdh = getLBDataForPhase(phase);
  auto& comm = lbdh.node_comm_;

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  auto op = obj_proxy.getProxy();

  // Check that communication exists on the send side as expected
  for (int i = 0; i < dim1; i++) {
    vt::Index1D idx{i};
    if (proxy(i).tryGetLocalPtr()) {
      bool found = false;
      auto idb = vt::elm::ElmIDBits::createObjGroup(op, next).id;
      for (auto&& x : comm) {
        for (auto&& y : x.second) {
          auto key = y.first;
          auto vol = y.second;
          if (key.from_.id == idxToElmID(idx) /*and key.to_.id == idb*/) {
            EXPECT_TRUE(key.from_.isMigratable());
            EXPECT_EQ(key.from_.id, idxToElmID(idx));
            EXPECT_FALSE(key.to_.isMigratable());
            EXPECT_EQ(key.to_.id, idb);
            EXPECT_EQ(vol.bytes, sizeof(MyObjMsg) * num_sends);
            EXPECT_EQ(vol.messages, num_sends);
            found = true;
          }
        }
      }
      EXPECT_TRUE(found);
    }
  }
}

// ObjGroup -> ColT, expected communication edge on receive side
TEST_F(TestLBDataComm, test_lb_data_comm_objgroup_to_col_send) {
  auto range = vt::Index1D{dim1};
  auto proxy = vt::makeCollection<MyCol>("test_lb_stats_comm_objgroup_to_col_send")
    .bounds(range)
    .bulkInsert()
    .wait();

  auto obj_proxy = vt::theObjGroup()->makeCollective<MyObj>(
    "test_lb_stats_comm_objgroup_to_col_send"
  );

  vt::runInEpochCollective("simulateObjGroupColTSends", [&]{
    auto node = theContext()->getNode();
    // @note: .invoke does not work here because it doesn't create the LBData
    // context!
    obj_proxy(node).send<ColProxyMsg, &MyObj::simulateObjGroupColTSends>(proxy);
  });

  vt::thePhase()->nextPhaseCollective();

  vt::runInEpochCollective("doReduce", [&]{
    for (int i = 0; i < dim1; i++) {
      if (proxy(i).tryGetLocalPtr()) {
        proxy(i).invoke<doReduce>();
      }
    }
  });

  vt::PhaseType phase = 0;
  auto lbdh = getLBDataForPhase(phase);
  auto& comm = lbdh.node_comm_;

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto prev = this_node - 1 >= 0 ? this_node - 1 : num_nodes-1;
  auto op = obj_proxy.getProxy();

  // Check that communication exists on the receive side as expected
  for (int i = 0; i < dim1; i++) {
    vt::Index1D idx{i};
    if (proxy(i).tryGetLocalPtr()) {
      bool found = false;
      for (auto&& x : comm) {
        for (auto&& y : x.second) {
          auto key = y.first;
          auto vol = y.second;
          if (key.to_.id == idxToElmID(idx)) {
            EXPECT_TRUE(key.to_.isMigratable());
            EXPECT_EQ(key.to_.id, idxToElmID(idx));
            EXPECT_FALSE(key.from_.isMigratable());
            EXPECT_EQ(key.from_.id, vt::elm::ElmIDBits::createObjGroup(op, prev).id);
            EXPECT_EQ(vol.bytes, sizeof(MyMsg) * num_sends);
            EXPECT_EQ(vol.messages, num_sends);
            found = true;
          }
        }
      }
      EXPECT_TRUE(found);
    }
  }
}

// ObjGroup -> ObjGroup, expected communication edge on send side
TEST_F(TestLBDataComm, test_lb_data_comm_objgroup_to_objgroup_send) {
  auto obj_proxy_a = vt::theObjGroup()->makeCollective<MyObj>(
    "test_lb_stats_comm_objgroup_to_objgroup_send"
  );
  auto obj_proxy_b = vt::theObjGroup()->makeCollective<MyObj>(
    "test_lb_stats_comm_objgroup_to_objgroup_send"
  );

  vt::runInEpochCollective("simulateObjGroupObjGroupSends", [&]{
    auto node = theContext()->getNode();
    // @note: .invoke does not work here because it doesn't create the LBData
    // context!
    obj_proxy_a(node).send<ProxyMsg, &MyObj::simulateObjGroupObjGroupSends>(
      obj_proxy_b
    );
  });

  vt::thePhase()->nextPhaseCollective();

  vt::PhaseType phase = 0;
  auto lbdh = getLBDataForPhase(phase);
  auto& comm = lbdh.node_comm_;

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  auto opa = obj_proxy_a.getProxy();
  auto opb = obj_proxy_b.getProxy();
  auto ida = vt::elm::ElmIDBits::createObjGroup(opa, this_node).id;
  auto idb = vt::elm::ElmIDBits::createObjGroup(opb, next).id;

  // Check that communication exists on the send side as expected
  bool found = false;
  for (auto&& x : comm) {
    for (auto&& y : x.second) {
      auto key = y.first;
      auto vol = y.second;
      if (key.from_.id == ida /*and key.to_.id == idb*/) {
        EXPECT_FALSE(key.to_.isMigratable());
        EXPECT_EQ(key.from_.id, ida);
        EXPECT_FALSE(key.from_.isMigratable());
        EXPECT_EQ(key.to_.id, idb);
        EXPECT_EQ(vol.bytes, sizeof(MyObjMsg) * num_sends);
        EXPECT_EQ(vol.messages, num_sends);
        found = true;
      }
    }
  }
  EXPECT_TRUE(found);
}

// Handler -> ColT, expected communication edge on receive side
TEST_F(TestLBDataComm, test_lb_data_comm_handler_to_col_send) {
  auto range = vt::Index1D{dim1};
  auto proxy = vt::makeCollection<MyCol>("test_lb_stats_comm_handler_to_col_send")
    .bounds(range)
    .bulkInsert()
    .wait();

  vt::runInEpochCollective("simulateHandlerColTSends", [&]{
    auto this_node = theContext()->getNode();
    auto num_nodes = theContext()->getNumNodes();
    auto next = (this_node + 1) % num_nodes;
    auto msg = makeMessage<ColProxyMsg>(proxy);
    theMsg()->sendMsg<simulateHandlerColTSends>(next, msg);
  });

  vt::thePhase()->nextPhaseCollective();

  vt::runInEpochCollective("doReduce", [&]{
    for (int i = 0; i < dim1; i++) {
      if (proxy(i).tryGetLocalPtr()) {
        proxy(i).invoke<doReduce>();
      }
    }
  });

  vt::PhaseType phase = 0;
  auto lbdh = getLBDataForPhase(phase);
  auto& comm = lbdh.node_comm_;

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto prev = this_node - 1 >= 0 ? this_node - 1 : num_nodes-1;

  // Check that communication exists on the receive side as expected
  for (int i = 0; i < dim1; i++) {
    vt::Index1D idx{i};
    if (proxy(i).tryGetLocalPtr()) {
      bool found = false;
      for (auto&& x : comm) {
        for (auto&& y : x.second) {
          auto key = y.first;
          auto vol = y.second;
          if (key.to_.id == idxToElmID(idx)) {
            EXPECT_TRUE(key.to_.isMigratable());
            EXPECT_EQ(key.to_.id, idxToElmID(idx));
            EXPECT_FALSE(key.from_.isMigratable());
            EXPECT_EQ(key.from_.id, vt::elm::ElmIDBits::createBareHandler(prev).id);
            EXPECT_EQ(vol.bytes, sizeof(MyMsg) * num_sends);
            EXPECT_EQ(vol.messages, num_sends);
            found = true;
          }
        }
      }
      EXPECT_TRUE(found);
    }
  }
}


// ColT -> Handler, expected communication edge on send side
TEST_F(TestLBDataComm, test_lb_data_comm_col_to_handler_send) {
  auto range = vt::Index1D{dim1};
  auto proxy = vt::makeCollection<MyCol>("test_lb_stats_comm_col_to_handler_send")
    .bounds(range)
    .bulkInsert()
    .wait();

  vt::runInEpochCollective("simulateColTHandlerSends", [&]{
    for (int i = 0; i < dim1; i++) {
      if (proxy(i).tryGetLocalPtr()) {
        proxy(i).invoke<simulateColTHandlerSends>();
      }
    }
  });

  vt::thePhase()->nextPhaseCollective();

  vt::runInEpochCollective("doReduce", [&]{
    for (int i = 0; i < dim1; i++) {
      if (proxy(i).tryGetLocalPtr()) {
        proxy(i).invoke<doReduce>();
      }
    }
  });

  vt::PhaseType phase = 0;
  auto lbdh = getLBDataForPhase(phase);
  auto& comm = lbdh.node_comm_;

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;

  // Check that communication exists on the send side as expected
  for (int i = 0; i < dim1; i++) {
    vt::Index1D idx{i};
    if (proxy(i).tryGetLocalPtr()) {
      bool found = false;
      for (auto&& x : comm) {
        for (auto&& y : x.second) {
          auto key = y.first;
          auto vol = y.second;
          fmt::print("from={}, to={}\n", key.from_, key.to_);
          if (key.from_.id == idxToElmID(idx)) {
            EXPECT_TRUE(key.from_.isMigratable());
            EXPECT_EQ(key.from_.id, idxToElmID(idx));
            EXPECT_FALSE(key.to_.isMigratable());
            EXPECT_EQ(key.to_.id, vt::elm::ElmIDBits::createBareHandler(next).id);
            EXPECT_EQ(vol.bytes, sizeof(MyObjMsg) * num_sends);
            EXPECT_EQ(vol.messages, num_sends);
            found = true;
          }
        }
      }
      EXPECT_TRUE(found);
    }
  }
}

// ObjGroup -> Handler, expected communication edge on send side
TEST_F(TestLBDataComm, test_lb_data_comm_objgroup_to_handler_send) {
  auto obj_proxy_a = vt::theObjGroup()->makeCollective<MyObj>(
    "test_lb_stats_comm_objgroup_to_handler_send"
  );

  vt::runInEpochCollective("simulateObjGroupHandlerSends", [&]{
    auto node = theContext()->getNode();
    // @note: .invoke does not work here because it doesn't create the LBData
    // context!
    obj_proxy_a(node).send<MyMsg, &MyObj::simulateObjGroupHandlerSends>();
  });


  vt::thePhase()->nextPhaseCollective();

  vt::PhaseType phase = 0;
  auto lbdh = getLBDataForPhase(phase);
  auto& comm = lbdh.node_comm_;

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  auto opa = obj_proxy_a.getProxy();
  auto ida = vt::elm::ElmIDBits::createObjGroup(opa, this_node).id;

  // Check that communication exists on the send side as expected
  bool found = false;
  for (auto&& x : comm) {
    for (auto&& y : x.second) {
      auto key = y.first;
      auto vol = y.second;
      if (key.from_.id == ida) {
        EXPECT_FALSE(key.to_.isMigratable());
        EXPECT_EQ(key.to_.id, vt::elm::ElmIDBits::createBareHandler(next).id);
        EXPECT_FALSE(key.from_.isMigratable());
        EXPECT_EQ(vol.bytes, sizeof(MyObjMsg) * num_sends);
        EXPECT_EQ(vol.messages, num_sends);
        found = true;
      }
    }
  }
  EXPECT_TRUE(found);
}

// Handler -> ObjGroup, expected communication edge on send side
TEST_F(TestLBDataComm, test_lb_data_comm_handler_to_objgroup_send) {
  auto obj_proxy_a = vt::theObjGroup()->makeCollective<MyObj>(
    "test_lb_stats_comm_handler_to_objgroup_send"
  );

  vt::runInEpochCollective("simulateHandlerObjGroupSends", [&]{
    auto this_node = theContext()->getNode();
    auto num_nodes = theContext()->getNumNodes();
    auto next = (this_node + 1) % num_nodes;
    auto msg = makeMessage<ProxyMsg>(obj_proxy_a);
    theMsg()->sendMsg<simulateHandlerObjGroupSends>(next, msg);
  });

  vt::thePhase()->nextPhaseCollective();

  vt::PhaseType phase = 0;
  auto lbdh = getLBDataForPhase(phase);
  auto& comm = lbdh.node_comm_;

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  auto opa = obj_proxy_a.getProxy();
  auto ida = vt::elm::ElmIDBits::createObjGroup(opa, next).id;

  // Check that communication exists on the send side as expected
  bool found = false;
  for (auto&& x : comm) {
    for (auto&& y : x.second) {
      auto key = y.first;
      auto vol = y.second;
      if (key.to_.id == ida) {
        EXPECT_FALSE(key.to_.isMigratable());
        EXPECT_EQ(key.from_.id, vt::elm::ElmIDBits::createBareHandler(this_node).id);
        EXPECT_FALSE(key.from_.isMigratable());
        EXPECT_EQ(vol.bytes, sizeof(MyObjMsg) * num_sends);
        EXPECT_EQ(vol.messages, num_sends);
        found = true;
      }
    }
  }
  EXPECT_TRUE(found);
}

// Handler -> Handler, expected communication edge on send side
TEST_F(TestLBDataComm, test_lb_data_comm_handler_to_handler_send) {
  vt::runInEpochCollective("simulateHandlerHandlerSends", [&]{
    auto this_node = theContext()->getNode();
    auto num_nodes = theContext()->getNumNodes();
    auto next = (this_node + 1) % num_nodes;
    auto msg = makeMessage<MyMsg>();
    theMsg()->sendMsg<simulateHandlerHandlerSends>(next, msg);
  });

  vt::thePhase()->nextPhaseCollective();

  vt::PhaseType phase = 0;
  auto lbdh = getLBDataForPhase(phase);
  auto& comm = lbdh.node_comm_;

  auto this_node = theContext()->getNode();
  auto num_nodes = theContext()->getNumNodes();
  auto next = (this_node + 1) % num_nodes;
  auto ida = vt::elm::ElmIDBits::createBareHandler(next).id;
  auto idb = vt::elm::ElmIDBits::createBareHandler(this_node).id;

  // Check that communication exists on the send side as expected
  bool found = false;
  for (auto&& x : comm) {
    for (auto&& y : x.second) {
      auto key = y.first;
      auto vol = y.second;
      if (key.to_.id == ida and key.from_.id == idb) {
        EXPECT_FALSE(key.to_.isMigratable());
        EXPECT_FALSE(key.from_.isMigratable());
        EXPECT_GE(vol.bytes, sizeof(MyObjMsg) * num_sends);
        EXPECT_GE(vol.messages, num_sends);
        found = true;
      }
    }
  }
  EXPECT_TRUE(found);
}

} /* end anon namespace */

}}}} // end namespace vt::tests::unit::comm

#endif
