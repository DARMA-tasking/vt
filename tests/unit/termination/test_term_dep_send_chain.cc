/*
//@HEADER
// *****************************************************************************
//
//                         test_term_dep_send_chain.cc
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

#include <gtest/gtest.h>

#include "test_parallel_harness.h"
#include "data_message.h"
#include "test_helpers.h"

#include "vt/vrt/collection/manager.h"
#include "vt/messaging/collection_chain_set.h"

namespace vt { namespace tests { namespace unit {

using namespace vt;
using namespace vt::tests::unit;

struct TestTermDepSendChain :
    TestParallelHarnessParam<std::tuple<bool, int, bool>> { };

struct TestMsg1 : vt::Message {
  TestMsg1() = default;
  TestMsg1(int a, int b, int c) : a_(a), b_(b), c_(c) { }

  int a_ = 0, b_ = 0, c_ = 0;
};

// Create some value that is calculable
double calcVal(int i, vt::Index2D idx) {
  return (i + 1)*idx.x() + (i + 1)*idx.y();
}

struct MyObjGroup;

struct MyCol : vt::Collection<MyCol,vt::Index2D> {
  MyCol() = default;
  MyCol(NodeType num, int k) : max_x(static_cast<int>(num)), max_y(k) {
    idx_ = getIndex();
  }

  struct OpMsg : vt::CollectionMessage<MyCol> {
    OpMsg() = default;
    OpMsg(double a, double b) : a_(a), b_(b) { }
    double a_ = 0.0, b_ = 0.0;
  };

  struct OpIdxMsg : vt::CollectionMessage<MyCol> {
    OpIdxMsg(vt::Index2D idx) : idx_(idx) { }
    vt::Index2D idx_;
  };

  struct OpVMsg : vt::CollectionMessage<MyCol> {
    using MessageParentType = vt::CollectionMessage<MyCol>;
    vt_msg_serialize_required(); // by stl container

    OpVMsg() = default;
    OpVMsg(std::vector<double> vec) : vec_(vec) { }
    OpVMsg(std::vector<double> vec, int step) : vec_(vec), step_(step) { }

    template <typename SerializerT>
    void serialize(SerializerT& s) {
      MessageParentType::serialize(s);
      s | vec_ | step_;
    }

    std::vector<double> vec_ = {};
    int step_ = 0;
  };

  struct ProxyMsg : vt::CollectionMessage<MyCol> {
    ProxyMsg() = default;
    ProxyMsg(vt::Callback<OpIdxMsg> cb) : cb_(cb) { }
    vt::Callback<OpIdxMsg> cb_ = {};
  };

  struct FinalMsg : vt::CollectionMessage<MyCol> {
    FinalMsg() = default;
    FinalMsg(int num) : num_(num) { }
    int num_ = 0;
  };

  static constexpr int const num_steps = 7;

  void checkExpectedStep(int expected) {
    EXPECT_EQ(step_ / num_steps, iter_);
    EXPECT_EQ(step_ % num_steps, expected);
  }

  void checkIncExpectedStep(int expected) {
    checkExpectedStep(expected);
    step_++;
    if (expected == num_steps - 1) {
      iter_++;
    }
  }

  void op1(OpMsg* msg) {
    migrating_ = false;
    checkIncExpectedStep(0);
    EXPECT_EQ(msg->a_, calcVal(1, idx_));
    EXPECT_EQ(msg->b_, calcVal(2, idx_));
    // fmt::print(
    //   "op1: idx={}, iter={}, a={}, b={}\n", idx, iter, msg->a, msg->b
    // );
  }

  void op2(OpMsg* msg) {
    checkIncExpectedStep(1);
    EXPECT_EQ(msg->a_, calcVal(3, idx_));
    EXPECT_EQ(msg->b_, calcVal(4, idx_));
    // fmt::print(
    //   "op2: idx={}, iter={}, a={}, b={}\n", idx, iter, msg->a, msg->b
    // );
  }

  void op3(OpVMsg* msg) {
    checkIncExpectedStep(2);
    for (auto i = 0; i < 10; i++) {
      EXPECT_EQ(msg->vec_[i], idx_.x()*i + idx_.y());
    }
    // fmt::print("op3: idx={}, iter={}, size={}\n", idx, iter, msg->vec.size());
  }

  void op4(ProxyMsg* msg) {
    checkExpectedStep(3);
    // fmt::print("op4: idx={}, iter={}\n", idx, iter);
    msg->cb_.send(idx_);
  }
  void op4Impl(OpMsg* msg) {
    checkIncExpectedStep(3);
    EXPECT_EQ(msg->a_, calcVal(5, idx_));
    EXPECT_EQ(msg->b_, calcVal(6, idx_));
    // fmt::print(
    //   "op4Impl: idx={}, iter={}, a={}, b={}\n", idx, iter, msg->a, msg->b
    // );
  }

  void op5(OpMsg* msg) {
    checkIncExpectedStep(4);
    EXPECT_EQ(msg->a_, calcVal(7, idx_));
    EXPECT_EQ(msg->b_, calcVal(8, idx_));
    // fmt::print(
    //   "op5: idx={}, iter={}, a={}, b={}\n", idx, iter, msg->a, msg->b
    // );
  }

  void op6(OpMsg* msg) {
    // Do not increment step until all op6Impl msgs are received to check
    // correctness
    checkExpectedStep(5);
    EXPECT_EQ(msg->a_, calcVal(9, idx_));
    EXPECT_EQ(msg->b_, calcVal(10, idx_));
    auto proxy = this->getCollectionProxy();
    auto xp1 = idx_.x() + 1 < max_x ? idx_.x() + 1 : 0;
    auto yp1 = idx_.y() + 1 < max_y ? idx_.y() + 1 : 0;
    auto xm1 = idx_.x() - 1 >= 0 ? idx_.x() - 1 : max_x - 1;
    auto ym1 = idx_.y() - 1 >= 0 ? idx_.y() - 1 : max_y - 1;
    std::vector<double> v = { 1.0, 2.0, 3.0 };
    proxy(xp1, idx_.y()).template send<OpVMsg, &MyCol::op6Impl>(v);
    proxy(xm1, idx_.y()).template send<OpVMsg, &MyCol::op6Impl>(v);
    proxy(idx_.x(), yp1).template send<OpVMsg, &MyCol::op6Impl>(v);
    proxy(idx_.x(), ym1).template send<OpVMsg, &MyCol::op6Impl>(v);
    // fmt::print(
    //   "op6: idx={}, iter={}, a={}, b={}\n", idx, iter, msg->a, msg->b
    // );
    started_op6_ = true;
    processOp6Impl();
  }
  void op6Impl(OpVMsg* msg) {
    if (not started_op6_) {
      op6_msgs_.push(vt::promoteMsg(msg));
      return;
    }

    checkExpectedStep(5);
    op6_counter_++;
    // fmt::print(
    //   "op6Impl: idx={}, iter={}, counter={}, step={}\n",
    //   idx, iter, op6_counter_, step
    // );
    if (op6_counter_ == 4) {
      // Only increment once all these are received
      checkIncExpectedStep(5);
      op6_counter_ = 0;
      started_op6_ = false;
    }

    processOp6Impl();
  }
  void processOp6Impl() {
    if (op6_msgs_.size() > 0) {
      auto t = op6_msgs_.top();
      op6_msgs_.pop();
      op6Impl(t.get());
    }
  }

  void op7(OpMsg* msg) {
    checkIncExpectedStep(6);
    EXPECT_EQ(msg->a_, calcVal(11, idx_));
    EXPECT_EQ(msg->b_, calcVal(12, idx_));
    // fmt::print(
    //   "op7: idx={}, iter={}, a={}, b={}\n", idx, iter, msg->a, msg->b
    // );
  }

  void doMigrate(OpMsg* msg) {
    checkExpectedStep(0);
    EXPECT_EQ(msg->a_, calcVal(13, idx_));
    EXPECT_EQ(msg->b_, calcVal(14, idx_));

    fmt::print(
      "doMigrate: idx={}, iter={}, a={}, b={}\n", idx_, iter_, msg->a_, msg->b_
    );

    migrating_ = true;
    auto node = vt::theContext()->getNode();
    auto num = vt::theContext()->getNumNodes();
    auto next = node + 1 < num ? node + 1 : 0;
    this->migrate(next);
  }

  void finalCheck(FinalMsg* msg) {
    auto num_iter = msg->num_;
    EXPECT_EQ(step_ / num_steps, num_iter);
    EXPECT_EQ(iter_, num_iter);
    final_check_ = true;
  }

  virtual ~MyCol() {
    EXPECT_TRUE(final_check_ or migrating_);
    // fmt::print(
    //   "destructor idx={}, final={}, mig={}\n",
    //   getIndex(), final_check, migrating
    // );
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::Collection<MyCol,vt::Index2D>::serialize(s);
    s | iter_ | step_ | idx_ | final_check_ | max_x | max_y | started_op6_;
    s | migrating_;
    s | op6_counter_;

    // Skip the stack op6_msgs_ because migration only occurs after all op-steps
    // are complete. Thus, the stack must be empty.
    if (not s.isUnpacking()) {
      vtAssert(op6_msgs_.size() == 0, "Stack should be empty during serialize");
    }
    s.skip(op6_msgs_);
  }

private:
  int iter_ = 0;
  int step_ = 0;
  vt::Index2D idx_;
  bool final_check_ = false;
  int max_x = 0, max_y = 0;
  bool started_op6_ = false;
  int op6_counter_ = 0;
  bool migrating_ = false;
  std::stack<MsgSharedPtr<OpVMsg>> op6_msgs_;
};

struct MyObjGroup {
  using OpMsg    = MyCol::OpMsg;
  using OpVMsg   = MyCol::OpVMsg;
  using OpIdxMsg = MyCol::OpIdxMsg;
  using ProxyMsg = MyCol::ProxyMsg;
  using FinalMsg = MyCol::FinalMsg;

  MyObjGroup() = default;

  void makeVT() {
    frontend_proxy_ = vt::theObjGroup()->makeCollective(this);
  }

  void makeColl(NodeType num_nodes, int k) {
    auto range = vt::Index2D(static_cast<int>(num_nodes),k);
    backend_proxy_ = vt::theCollection()->constructCollective<MyCol>(
      range, [=](vt::Index2D idx) {
        return std::make_unique<MyCol>(num_nodes, k);
      }
    );

    chains_ = std::make_unique<vt::messaging::CollectionChainSet<vt::Index2D>>(
      backend_proxy_
    );
  }

  void startUpdate() {
    epoch_ = vt::theTerm()->makeEpochCollective();
    vt::theMsg()->pushEpoch(epoch_);
    started_ = true;
  }

  void op1() {
    chains_->nextStep("op1", [=](vt::Index2D idx) {
      auto a = calcVal(1, idx);
      auto b = calcVal(2, idx);
      return backend_proxy_(idx).template send<OpMsg, &MyCol::op1>(a,b);
    });
  }

  void op2() {
    chains_->nextStep("op2", [=](vt::Index2D idx) {
      auto a = calcVal(3,idx);
      auto b = calcVal(4,idx);
      return backend_proxy_(idx).template send<OpMsg, &MyCol::op2>(a,b);
    });
  }

  void op3() {
    chains_->nextStep("op3", [=](vt::Index2D idx) {
      std::vector<double> v;
      for (auto i = 0; i < 10; i++) {
        v.push_back(idx.x()*i + idx.y());
      }
      return backend_proxy_(idx).template send<OpVMsg, &MyCol::op3>(v);
    });
  }

  void op4() {
    chains_->nextStep("op4", [=](vt::Index2D idx) {
      auto node = vt::theContext()->getNode();
      auto num = vt::theContext()->getNumNodes();
      auto next = node + 1 < num ? node + 1 : 0;
      auto proxy = frontend_proxy_(next);
      auto c = vt::theCB()->makeSend<MyObjGroup,OpIdxMsg,&MyObjGroup::op4Impl>(proxy);
      return backend_proxy_(idx).template send<ProxyMsg, &MyCol::op4>(c);
    });
  }
  void op4Impl(OpIdxMsg* msg) {
    //  Respond to collection element for op4
    auto idx = msg->idx_;
    auto a = calcVal(5,idx);
    auto b = calcVal(6,idx);
    backend_proxy_(idx).template send<OpMsg, &MyCol::op4Impl>(a,b);
  }

  void op5() {
    chains_->nextStep("op5", [=](vt::Index2D idx) {
      auto a = calcVal(7,idx);
      auto b = calcVal(8,idx);
      return backend_proxy_(idx).template send<OpMsg, &MyCol::op5>(a,b);
    });
  }

  void op6() {
    chains_->nextStepCollective("op6", [=](vt::Index2D idx) {
      auto a = calcVal(9,idx);
      auto b = calcVal(10,idx);
      return backend_proxy_(idx).template send<OpMsg, &MyCol::op6>(a,b);
    });
  }

  void op7() {
    chains_->nextStep("op7", [=](vt::Index2D idx) {
      auto a = calcVal(11,idx);
      auto b = calcVal(12,idx);
      return backend_proxy_(idx).template send<OpMsg, &MyCol::op7>(a,b);
    });
  }

  void doMigrate() {
    chains_->nextStep("doMigrate", [=](vt::Index2D idx) {
      auto a = calcVal(13,idx);
      auto b = calcVal(14,idx);
      return backend_proxy_(idx).template send<OpMsg, &MyCol::doMigrate>(a,b);
    });
  }

  void finishUpdate() {
    chains_->phaseDone();
    vt::theMsg()->popEpoch(epoch_);
    vt::theTerm()->finishedEpoch(epoch_);

    vt::runSchedulerThrough(epoch_);

    started_ = false;
  }

  void finalCheck(int i) {
    chains_->nextStep("finalCheck", [=](vt::Index2D idx) {
      return backend_proxy_(idx).template send<FinalMsg, &MyCol::finalCheck>(i);
    });
  }

  void startup() {
    // Produce on global epoch so on a single node it does not terminate early
    vt::theTerm()->produce(vt::term::any_epoch_sentinel);
  }

  void shutdown() {
    // Consume on global epoch to match the startup produce
    vt::theTerm()->consume(vt::term::any_epoch_sentinel);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | started_
      | epoch_
      | backend_proxy_
      | frontend_proxy_
      | chains_;
  }

private:
  // Has an update been started
  bool started_ = false;
  // The current epoch for a given update
  vt::EpochType epoch_ = vt::no_epoch;
  // The backend collection proxy for managing the over decomposed workers
  vt::CollectionProxy<MyCol, vt::Index2D> backend_proxy_ = {};
  // The proxy for this objgroup
  vt::objgroup::proxy::Proxy<MyObjGroup> frontend_proxy_ = {};
  // The current collection chains that are being managed here
  std::unique_ptr<vt::messaging::CollectionChainSet<vt::Index2D>> chains_ = nullptr;
};

TEST_P(TestTermDepSendChain, test_term_dep_send_chain) {
  SET_MAX_NUM_NODES_CONSTRAINT(CMAKE_DETECTED_MAX_NUM_NODES);

  auto const& this_node = theContext()->getNode();
  auto const& num_nodes = theContext()->getNumNodes();
  auto const iter = 50;
  auto const& tup = GetParam();
  auto const use_ds = std::get<0>(tup);
  auto const k = std::get<1>(tup);
  auto const migrate = std::get<2>(tup);

  vt::theConfig()->vt_term_rooted_use_wave = !use_ds;
  vt::theConfig()->vt_term_rooted_use_ds = use_ds;

  auto local = std::make_unique<MyObjGroup>();
  local->startup();
  local->makeVT();
  local->makeColl(num_nodes,k);

  // Must have barrier here so op4Impl does not bounce early (invalid proxy)!
  vt::theCollective()->barrier();

  for (int t = 0; t < iter; t++) {
    if (this_node == 0 && t % 5 == 0) {
      fmt::print("start iter={}, k={}, max_iter={}\n", t, k, iter);
    }

    local->startUpdate();
    local->op1();
    local->op2();
    local->op3();
    local->op4();
    local->op5();
    local->op6();
    local->op7();

    if (migrate && t > 0 && t % 5 == 0) {
      local->doMigrate();
    }

    local->finishUpdate();
  }

  local->finalCheck(iter);
  local->shutdown();
}


struct PrintParam {
  template <typename ParamType>
  std::string operator()(testing::TestParamInfo<ParamType> const& val) const {
    auto const ds = std::get<0>(val.param);
    auto const k = std::get<1>(val.param);
    auto const migrate = std::get<2>(val.param);
    return fmt::format("DS{}K{}MIGRATE{}", ds, k, migrate);
  }
};

struct MergeCol : vt::Collection<MergeCol,vt::Index2D> {
  MergeCol() = default;
  MergeCol(NodeType num, double off) : offset_( off ) {
    idx_ = getIndex();
  }

  struct DataMsg : vt::CollectionMessage<MergeCol> {
    DataMsg() = default;
    explicit DataMsg(double x) : x_(x) { }
    double x_ = 0.0;
  };

  struct GhostMsg : vt::CollectionMessage<MergeCol > {
    GhostMsg() = default;
    explicit GhostMsg(vt::CollectionProxy<MergeCol, vt::Index2D> proxy)
      : proxy_(proxy)
    {}
    vt::CollectionProxy<MergeCol, vt::Index2D> proxy_;
  };

  void initData(DataMsg* msg) {
    EXPECT_EQ(msg->x_, calcVal(1, idx_));
    data_ = msg->x_ + offset_;
  }

  void ghost(GhostMsg* msg) {
    msg->proxy_(getIndex()).template send<DataMsg, &MergeCol::interact>(data_);
  }

  void interact(DataMsg* msg ) {
    data_ *= msg->x_;
  }

  void check(DataMsg *msg) {
    EXPECT_EQ(msg->x_, data_);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    vt::Collection<MergeCol,vt::Index2D>::serialize(s);
    s | idx_ | offset_ | data_;
  }

private:

  vt::Index2D idx_;
  double offset_ = 0;
  double data_ = 0.0;
};

struct MergeObjGroup
{
  MergeObjGroup() = default;

  void startup() {
    // Produce on global epoch so on a single node it does not terminate early
    vt::theTerm()->produce(vt::term::any_epoch_sentinel);
  }

  void shutdown() {
    // Consume on global epoch to match the startup produce
    vt::theTerm()->consume(vt::term::any_epoch_sentinel);
  }

  void makeVT() {
    frontend_proxy_ = vt::theObjGroup()->makeCollective(this);
  }

  void makeColl(NodeType num_nodes, int k, double offset) {
    auto const node = theContext()->getNode();
    auto range = vt::Index2D(static_cast<int>(num_nodes),k);
    backend_proxy_ = vt::theCollection()->constructCollective<MergeCol>(
      range, [=](vt::Index2D idx) {
        return std::make_unique<MergeCol>(num_nodes, offset);
      }
    );

    chains_ = std::make_unique<vt::messaging::CollectionChainSet<vt::Index2D>>();

    for (int i = 0; i < k; ++i) {
      chains_->addIndex(vt::Index2D(static_cast<int>(node), i));
    }
  }

  void startUpdate() {
    epoch_ = vt::theTerm()->makeEpochCollective();
    vt::theMsg()->pushEpoch(epoch_);
  }

  void initData() {
    chains_->nextStep("initData", [=](vt::Index2D idx) {
      auto x = calcVal(1,idx);
      return backend_proxy_(idx).template send<MergeCol::DataMsg, &MergeCol::initData>(x);
    });
  }

  void interact( MergeObjGroup &other ) {
    auto other_proxy = other.backend_proxy_;
    vt::messaging::CollectionChainSet<vt::Index2D>::mergeStepCollective( "interact",
                                                                        *chains_,
                                                                        *other.chains_,
                                                           [=]( vt::Index2D idx) {
      return backend_proxy_(idx).template send<MergeCol::GhostMsg, &MergeCol::ghost>(other_proxy);
    });
  }

  void check( double offset, double other_offset, bool is_left ) {
    chains_->nextStep("initData", [=](vt::Index2D idx) {
      auto x = calcVal(1,idx) + offset;
      if ( !is_left )
        x *= calcVal(1,idx) + other_offset;
      return backend_proxy_(idx).template send<MergeCol::DataMsg, &MergeCol::check>(x);
    });
  }

  void finishUpdate() {
    chains_->phaseDone();
    vt::theMsg()->popEpoch(epoch_);
    vt::theTerm()->finishedEpoch(epoch_);

    vt::runSchedulerThrough(epoch_);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | epoch_
      | backend_proxy_
      | frontend_proxy_
      | chains_;
  }

  private:

  // The current epoch for a given update
  vt::EpochType epoch_ = vt::no_epoch;
  // The backend collection proxy for managing the over decomposed workers
  vt::CollectionProxy<MergeCol, vt::Index2D> backend_proxy_ = {};
  // The proxy for this objgroup
  vt::objgroup::proxy::Proxy<MergeObjGroup> frontend_proxy_ = {};
  // The current collection chains that are being managed here
  std::unique_ptr<vt::messaging::CollectionChainSet<vt::Index2D>> chains_ = nullptr;
};

TEST_P(TestTermDepSendChain, test_term_dep_send_chain_merge) {

  SET_MAX_NUM_NODES_CONSTRAINT(CMAKE_DETECTED_MAX_NUM_NODES);

  auto const& num_nodes = theContext()->getNumNodes();
  auto const iter = 50;
  auto const& tup = GetParam();
  auto const use_ds = std::get<0>(tup);
  auto const k = std::get<1>(tup);

  vt::theConfig()->vt_term_rooted_use_wave = !use_ds;
  vt::theConfig()->vt_term_rooted_use_ds = use_ds;

  auto obj_a = std::make_unique<MergeObjGroup>();
  obj_a->startup();
  obj_a->makeVT();
  obj_a->makeColl(num_nodes,k, 0.0);

  auto obj_b = std::make_unique<MergeObjGroup>();
  obj_b->startup();
  obj_b->makeVT();
  obj_b->makeColl(num_nodes,k, 1000.0);

  // Must have barrier here so op4Impl does not bounce early (invalid proxy)!
  vt::theCollective()->barrier();

  for (int t = 0; t < iter; t++) {
    obj_a->startUpdate();
    obj_a->initData();

    obj_b->startUpdate();
    obj_b->initData();

    obj_a->interact( *obj_b );

    obj_a->check(0.0, 1000.0, true);
    obj_b->check(0.0, 1000.0, false);

    obj_b->finishUpdate();
    obj_a->finishUpdate();
  }

  obj_a->shutdown();
  obj_b->shutdown();
}

// Test Wave-epoch with a narrower set of parameters since large k is very slow
INSTANTIATE_TEST_SUITE_P(
  DepSendChainInputExplodeWave, TestTermDepSendChain,
  ::testing::Combine(
    ::testing::Values(false),
    ::testing::Values(1, 2),
    ::testing::Bool()
  ),
  PrintParam()
);

// Test DS-epoch with a broader set of parameters since it is quick
INSTANTIATE_TEST_SUITE_P(
  DepSendChainInputExplodeDS, TestTermDepSendChain,
  ::testing::Combine(
    ::testing::Values(true),
    ::testing::Range(1, 8),
    ::testing::Bool()
  ),
  PrintParam()
);

}}} // end namespace vt::tests::unit
