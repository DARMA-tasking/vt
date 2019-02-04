/*
//@HEADER
// ************************************************************************
//
//                          test_term.cc
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

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <random>
#include <unordered_map>

#include "test_parallel_harness.h"
#include "data_message.h"
#include "vt/transport.h"

#define DEBUG_TERM_ACTION 2

namespace vt { namespace tests { namespace unit {

// specify the order of 'addAction' w.r.t 'finishedEpoch'
enum class ORDER : int { before, after, misc };

struct TestTermBase : vt::tests::unit::TestParallelHarnessParam<std::tuple<ORDER,bool,int>> {

protected:
  struct Metadata; // forward-declaration for 'data'
  static vt::NodeType me;
  static vt::NodeType root;
  static vt::NodeType all;
  static std::unordered_map<vt::EpochType,Metadata> data;

  // test parameters
  struct {
    ORDER order = ORDER::before;
    bool useDS = false;
    int depth = 1;
  } param_;

  // basic message
  struct BasicMsg : vt::Message {

    BasicMsg() = default;
    BasicMsg(vt::NodeType src, vt::NodeType dst, int ttl, vt::EpochType epoch) :
      src_(src),
      dst_(dst),
      ttl_(ttl-1),
      epoch_(epoch) {}
    BasicMsg(vt::NodeType src, vt::NodeType dst, int ttl) : BasicMsg(src,dst,ttl,vt::no_epoch){}
    BasicMsg(vt::NodeType src, vt::NodeType dst) : BasicMsg(src,dst,1,vt::no_epoch){}

    //
    int ttl_ = 0;
    vt::NodeType src_ = vt::uninitialized_destination;
    vt::NodeType dst_ = vt::uninitialized_destination;
    vt::EpochType epoch_ = vt::no_epoch;
  };

  // control messages for channel counting
  struct CtrlMsg : vt::Message {

    CtrlMsg() = default;
    CtrlMsg(vt::NodeType src, vt::NodeType dst, int nb, vt::EpochType epoch) :
      src_(src),
      dst_(dst),
      count_(nb),
      epoch_(epoch) {}
    CtrlMsg(vt::NodeType src, vt::NodeType dst, int nb) : CtrlMsg(src,dst,nb,vt::no_epoch){}
    CtrlMsg(vt::NodeType src, vt::NodeType dst) : CtrlMsg(src,dst,0,vt::no_epoch){}

    // nb of outgoing basic messages for 'pingHandler'
    // nb of incoming basic messages for 'echoHandler'
    int count_ = 0;
    vt::NodeType src_ = vt::uninitialized_destination;
    vt::NodeType dst_ = vt::uninitialized_destination;
    vt::EpochType epoch_ = vt::no_epoch;    
  };

  // message counters per rank and per channel.
  // note: should be aggregated since we compare
  // different counters of an unique rank.
  // (thus an unique iterator)
  struct MsgCount {
    int in_ = 0;  // incoming
    int out_ = 0; // outgoing
    int ack_ = 0; // acknowledged
  };

  // meta-data per rank and per epoch
  // for channel counting algorithm.
  struct Metadata {

    int degree_ = 0;
    vt::NodeType activator_ = 0;
    std::unordered_map<vt::NodeType,MsgCount> count_;

    Metadata() :
      degree_(0), activator_(0) {
      for (vt::NodeType dst=0; dst < all; ++dst){
        count_[dst] = {0,0,0};
      }
    }

    ~Metadata(){ count_.clear(); };
  };


  // -------------------------------------
  // Handlers and methods involved in the
  // channel counting detection algorithm
  // -------------------------------------

  template<typename Msg, vt::ActiveTypedFnType<Msg>* handler>
  static void sendMsg(vt::NodeType dst, int count, vt::EpochType ep){
    vtAssertExpr(dst not_eq vt::uninitialized_destination);
    vtAssertExpr(dst not_eq me);
    auto msg = makeSharedMessage<Msg>(me,dst,count,ep);
    vtAssertExpr(msg->src_ == me);

    if(ep not_eq vt::no_epoch){
      vt::envelopeSetEpoch(msg->env,ep);
    }
    vt::theMsg()->sendMsg<Msg,handler>(dst,msg);
  }

  // note: only for basic messages,
  // but different handlers may be used.
  template<vt::ActiveTypedFnType<BasicMsg>* handler>
  static void broadcast(int count, vt::EpochType ep){
    auto msg = makeSharedMessage<BasicMsg>(me,vt::uninitialized_destination,count,ep);
    if(ep not_eq vt::no_epoch){
      vt::envelopeSetEpoch(msg->env,ep);
    }
    vt::theMsg()->broadcastMsg<BasicMsg,handler>(msg);

    for(auto& active: data[ep].count_){
      auto const& dst = active.first;
      auto& nb = active.second;
      if(dst not_eq me){
        nb.out_++;
      }
    }
  }

  // shortcuts for message sending
  static void routeBasic(vt::NodeType dst, int ttl, vt::EpochType ep){
    sendMsg<BasicMsg,routedHandler>(dst,ttl,ep);
    // increment outgoing message counter
    data[ep].count_[dst].out_++;
  }

  static void sendPing(vt::NodeType dst, int count, vt::EpochType ep){
    sendMsg<CtrlMsg,pingHandler>(dst,count,ep);
  }

  static void sendEcho(vt::NodeType dst, int count, vt::EpochType ep){
    vtAssertExpr(dst >= 0);
    #if DEBUG_TERM_ACTION == 1
      fmt::print("rank:{} echo::dst {}",me,dst);
    #endif
    vtAssertExpr(dst not_eq vt::uninitialized_destination);
    sendMsg<CtrlMsg,echoHandler>(dst,count,ep);
  }

  // trigger termination detection by root
  static void trigger(vt::EpochType ep){
    vtAssert(me == root, "Only root may trigger termination check");

    for(auto& active: data[ep].count_){
      auto const& dst = active.first;

      if(dst not_eq me){
        #if DEBUG_TERM_ACTION
          fmt::print("rank:{} trigger dst {}\n", me, dst);
        #endif
        auto& nb = active.second;
        auto& degree = data[ep].degree_;
        // avoid potential negative degree.
        // it may occurs when an echo is
        // directly sent to the root
        sendPing(dst,nb.out_,ep);
        degree++;
      }
    }
  }

  // propagate check on current subtree
  static void propagate(vt::EpochType ep){
    for(auto& active: data[ep].count_){
      auto const& dst = active.first;
      if(dst not_eq me){
        auto& nb = active.second;
        auto& degree = data[ep].degree_;
        // confirmation missing
        if(nb.out_ not_eq nb.ack_){
          #if DEBUG_TERM_ACTION == 1
            fmt::print(
              "rank {}: propagate: sendPing to {}, out={}, degree={}\n",
              me,dst,nb.out_,degree+1
            );
          #endif
          // check subtree
          sendPing(dst,nb.out_,ep);
          // more echoes outstanding
          degree++;
        }
      }
    }
  }

  // check local termination
  // check whether Cij^+ == Cij^-
  static bool hasFinished(vt::EpochType ep){
    for(auto& active: data[ep].count_){
      auto const& dst = active.first;
      if(dst not_eq me){
        auto const& nb = active.second;
        if(nb.out_ not_eq nb.ack_){
          return false;
        }
      }
    }
    return true;
  }

  // on receipt of a basic message.
  // note: msg->dst is uninitialized on broadcast,
  // thus related assertion was removed.
  static void basicHandler(BasicMsg* msg){
    auto const& src = msg->src_;
    // avoid self sending case
    if(me not_eq src){
      auto& nb = data[msg->epoch_].count_[src];
      nb.in_++;
    }
  }

  static void routedHandler(BasicMsg* msg){
    vtAssertExpr(me not_eq root);
    basicHandler(msg);

    if (all > 2 && msg->ttl_ > 0){
      // avoid implicit cast
      vt::NodeType const one = 1;
      int const nb_rounds = static_cast<int>(drand48()*5);

      for(int k=0; k < nb_rounds; ++k){
        // note: root and self-send are excluded
        auto dst = (me+one > all-one ? one: me+one);
        if (dst == me && dst == one) dst++;
        vtAssertExpr(dst > one || me not_eq one);

        routeBasic(dst,msg->ttl_,msg->epoch_);
      }
    }
  }

  // on receipt of an echo message echo<m> from Pi
  static void echoHandler(CtrlMsg* msg){
    vtAssertExpr(me == msg->dst_);
    // shortcuts for readability
    auto const& src = msg->src_;
    auto const& ep = msg->epoch_;
    auto& degree = data[ep].degree_;
    auto& nb_ack = data[ep].count_[src].ack_;

    // update ack and decrease missing echoes counter
    nb_ack = msg->count_;
    degree--;

    #if DEBUG_TERM_ACTION == 1
      auto& nb_in = data[ep].count_[src].in_;
      fmt::print(
        "rank {}: echoHandler: in={}, ack={}, degree={}\n",
        me,nb_in,nb_ack, degree
      );
    #endif

    // last echo checks whether all subtrees are quiet
    if(degree == 0){
      propagate(ep);
    }
    // all echoes arrived, everything quiet
    if(degree == 0){
      // echo to parent activator node
      auto const& dst = data[ep].activator_;
      auto const& nb = data[ep].count_[dst];
      if(dst not_eq me){
        sendEcho(dst,nb.in_,ep);
      }
    }
  }

  // on receipt of a control message test<m> from Pi
  static void pingHandler(CtrlMsg* msg){
    vtAssertExpr(me == msg->dst_);
    auto const& src = msg->src_;
    auto const& ep = msg->epoch_;
    auto const& degree = data[ep].degree_;
    auto const& nb = data[ep].count_[src];
    auto& activator = data[ep].activator_;

    #if DEBUG_TERM_ACTION == 1
      fmt::print(
        "{}: pingHandler: in={}, src={}, degree={}\n",
        me,nb.in_,src,degree
      );
    #endif

    // if already engaged or subtree is quiet
    if(degree > 0 or hasFinished(ep)){
      sendEcho(src,nb.in_,ep);
    }
    else {
      activator = src;
      propagate(ep);
    }
  }

  virtual void SetUp(){
    vt::tests::unit::TestParallelHarnessParam<std::tuple<ORDER,bool,int>>::SetUp();

    // set ranks
    root = 0;
    me  = vt::theContext()->getNode();
    all = vt::theContext()->getNumNodes();
    vtAssert(all > 1, "There should be at least two nodes");

    // set 'addAction" ordering parameter
    param_.order = std::get<0>(GetParam());
  }

  // ---------------------------------------------
  // Methods used by parameterized test cases:
  // 1. epoch initialization [rooted,collect]
  // 2. fictive distributed computation
  // 3. epoch finalization including termination
  //   detection and status checking
  // ---------------------------------------------

  // create a sequence of rooted epochs by root node
  std::vector<vt::EpochType> initRootedEpochSequence(int nb){

    vtAssertExpr(nb > 0);
    vtAssertExpr(me == root);

    std::vector<vt::EpochType> seq(nb);

    // create rooted epoch sequence
    seq[0] = vt::theTerm()->makeEpochRooted(param_.useDS);
    vtAssertExpr(root == epoch::EpochManip::node(seq[0]));
    vtAssertExpr(epoch::EpochManip::isRooted(seq[0]));

    for(int i=1; i < nb; ++i){
      seq[i] = epoch::EpochManip::next(seq[i-1]);
      vtAssertExpr(epoch::EpochManip::isRooted(seq[i]));
    }
    return seq;
  }

  // create a collective epoch sequence
  std::vector<vt::EpochType> initCollectEpochSequence(int nb){
    vtAssertExpr(nb > 0);

    std::vector<vt::EpochType> seq(nb);

    for(auto& ep : seq)
      ep = vt::theTerm()->makeEpochCollective();

    return seq;
  }

  // perform a fictive distributed computation for test purposes
  void distributedComputation(vt::EpochType const& ep){

    std::random_device device;
    std::mt19937 engine(device());
    std::uniform_int_distribution<int> dist_ttl(1,10);
    std::uniform_int_distribution<int> dist_dest(1,all-1);
    std::uniform_int_distribution<int> dist_round(1,10);

    #if DEBUG_TERM_ACTION
    if(ep not_eq vt::no_epoch){
      fmt::print(
        "rank:{}, epoch:{:x}, is_rooted ? {}\n",
        me, ep, epoch::EpochManip::isRooted(ep)
      );
    }
    #endif

    // process computation for current epoch
    int rounds = dist_round(engine);
    if(all > 2){
      for(int k=0; k < rounds; ++k){
        int dst = me + dist_dest(engine);
        int ttl = dist_ttl(engine);
        routeBasic(dst,ttl,ep);
      }
    } else {
      broadcast<basicHandler>(1,ep);
    }
  }

#if DEBUG_TERM_ACTION
  void debug_log(std::string const step, vt::EpochType const& ep){
    switch (param_.order){
      case ORDER::after:
        fmt::print(
          "rank:{}: epoch:{:x}, ORDER::after, {} action, is_rooted ? {}\n",
          me, ep, step, epoch::EpochManip::isRooted(ep)
        );
        break;
      case ORDER::misc:
        fmt::print(
          "rank:{}: epoch:{:x}, ORDER::misc, {} action, is_rooted ? {}\n",
          me, ep, step, epoch::EpochManip::isRooted(ep)
        );
        break;
      default:
        fmt::print(
          "rank:{}: epoch:{:x}, ORDER::before, {} action, is_rooted ? {}\n",
          me, ep, step, epoch::EpochManip::isRooted(ep)
        );
        break;
    }
    fflush(stdout);
  }
#endif

  // assign action to be processed at the end
  // of the epoch; trigger termination detection;
  // and finalize epoch.
  void finalize(vt::EpochType ep){

    auto finish = [&](vt::EpochType& ep){
      if(ep not_eq vt::no_epoch){
        vt::theTerm()->finishedEpoch(ep);
      }
    };

    // 1. no epoch case
    if(ep == vt::no_epoch){
      if(me == root){
        trigger(vt::no_epoch);
        #if DEBUG_TERM_ACTION
          debug_log("entering no_epoch", ep);
          vt::theTerm()->addAction([&]{
            debug_log("within no_epoch", ep);
            EXPECT_TRUE(hasFinished(vt::no_epoch));
            debug_log("leaving no_epoch", ep);
          });
        #else
        vt::theTerm()->addAction([&]{ EXPECT_TRUE(hasFinished(vt::no_epoch)); });
        #endif
      }
    } else if(epoch::EpochManip::isRooted(ep)){
      if(me == root){

        switch(param_.order){
          case ORDER::after :{
            trigger(ep);
            finish(ep);
            #if DEBUG_TERM_ACTION
              debug_log("entering", ep);
              vt::theTerm()->addActionEpoch(ep,[&]{
                debug_log("within", ep);
                EXPECT_TRUE(hasFinished(ep));
                debug_log("leaving", ep);
              });
            #else
              vt::theTerm()->addActionEpoch(ep,[&]{ EXPECT_TRUE(hasFinished(ep)); });
            #endif
            break;
          }
          case ORDER::misc :{
            trigger(ep);
            #if DEBUG_TERM_ACTION
              debug_log("entering", ep);
              vt::theTerm()->addActionEpoch(ep,[&]{
                debug_log("within", ep);
                EXPECT_TRUE(hasFinished(ep));
                debug_log("leaving",ep);
              });
            #else
              vt::theTerm()->addActionEpoch(ep,[&]{ EXPECT_TRUE(hasFinished(ep)); });
            #endif
            finish(ep);
            break;
          }
          default :{
            #if DEBUG_TERM_ACTION
              vt::theTerm()->addActionEpoch(ep,[&]{
                debug_log("within", ep);
                EXPECT_TRUE(hasFinished(ep));
                debug_log("leaving",ep);
              });
            #else
              vt::theTerm()->addActionEpoch(ep,[&]{ EXPECT_TRUE(hasFinished(ep)); });
            #endif
            trigger(ep);
            #if DEBUG_TERM_ACTION
              debug_log("entering", ep);
            #endif
            finish(ep);
            break;
          }
        }
      }
    } else /* it is a collective epoch */ {
      finish(ep);
      // check status
      if(me == root){
        vt::theTerm()->addActionEpoch(ep,[&]{ EXPECT_TRUE(hasFinished(ep)); });
      }
    }

    #if DEBUG_TERM_ACTION
      fmt::print("rank:{}, epoch:{:x} completed\n",me,ep);
    #endif
  }
};

/*static*/ vt::NodeType TestTermBase::me;
/*static*/ vt::NodeType TestTermBase::root;
/*static*/ vt::NodeType TestTermBase::all;
/*static*/ std::unordered_map<vt::EpochType,TestTermBase::Metadata> TestTermBase::data;


// -------------------------------------
// DERIVED FIXTURES
// note: use separate fixtures to reduce
//       the number of instantiated tests
// -------------------------------------

// Rooted epochs case [useDS]
struct TestTermRooted : TestTermBase {

  virtual void SetUp(){
    TestTermBase::SetUp();
    // retrieve useDS parameter value
    param_.useDS = std::get<1>(GetParam());
  }
};

// Nested collective epochs case [depth]
struct TestTermNestedCollect : TestTermBase {

  virtual void SetUp(){
    TestTermBase::SetUp();
    // set nesting depth value
    param_.depth = std::get<2>(GetParam());
  }

  void nestedCollectEpoch(int depth){

    vtAssertExpr(depth > 0);

    // explicitly set 'child' epoch param
    auto ep = vt::theTerm()->newEpochCollective(true);
    vt::theTerm()->getWindow(ep)->addEpoch(ep);

    // all ranks should have the same depth
    vt::theCollective()->barrier();
    if (depth > 1)
      nestedCollectEpoch(depth - 1);

    if (me == root){
      distributedComputation(ep);
      trigger(ep);
    }

    finalize(ep);
  }
};

// Nested rooted epochs case [useDS,depth]
struct TestTermNestedRooted : TestTermRooted {

  virtual void SetUp(){
    TestTermRooted::SetUp();
    // set nesting depth value
    param_.depth = std::get<2>(GetParam());
  }

  void nestedRootedEpoch(int depth){
    vtAssertExpr(depth > 0);

    auto ep = vt::no_epoch;

    if(me == root){
      // explicitly set 'child' epoch param
      ep = vt::theTerm()->newEpochRooted(param_.useDS,true);
      vt::theTerm()->getWindow(ep)->addEpoch(ep);

      vtAssertExpr(root == epoch::EpochManip::node(ep));
      vtAssertExpr(epoch::EpochManip::isRooted(ep));
    }

    // all ranks should have the same depth
    vt::theCollective()->barrier();
    if(depth > 1)
      nestedRootedEpoch(depth-1);

    if(me == root){
      distributedComputation(ep);
      trigger(ep);
      finalize(ep);
    }
  }
};

// -----------
// TEST CASES
// -----------

// simple broadcast
TEST_P(TestTermBase, test_term_detect_broadcast)
{
  if(me == root){
    // start computation
    broadcast<basicHandler>(1,vt::no_epoch);
    // trigger detection and check status
    finalize(vt::no_epoch);
  }
}

// routed messages
TEST_P(TestTermBase, test_term_detect_routed)
{
  // there should be at least 3 nodes for this case
  if(all > 2 && me == root){
    //start computation
    distributedComputation(vt::no_epoch);
    // trigger detection and check status
    finalize(vt::no_epoch);
  }
}

// collective epochs
TEST_P(TestTermBase, test_term_detect_collect_epoch)
{
  auto sequence = initCollectEpochSequence(5);

  for(auto const& ep: sequence){
    if(me == root){
      distributedComputation(ep);
      trigger(ep);
    }
    // finalize collective epoch
    finalize(ep);
  }
}

// rooted epochs
TEST_P(TestTermRooted, test_term_detect_rooted_epoch)
{
  if(me == root){
    auto sequence = initRootedEpochSequence(5);

    for(auto const& ep: sequence){
      distributedComputation(ep);
      finalize(ep);
    }
  }
}

// nested collective epochs
TEST_P(TestTermNestedCollect, test_term_detect_nested_collect_epoch)
{
  nestedCollectEpoch(param_.depth);
}

// nested rooted epochs
TEST_P(TestTermNestedRooted, test_term_detect_nested_rooted_epoch)
{
  nestedRootedEpoch(param_.depth);
}

// -------------------------
// testcases instantiation
// -------------------------

INSTANTIATE_TEST_CASE_P(
  InstantiationName, TestTermBase,
  ::testing::Combine(
    ::testing::Values(ORDER::before, ORDER::after, ORDER::misc),
    ::testing::Values(false),
    ::testing::Values(1)
  )
);

INSTANTIATE_TEST_CASE_P(
  InstantiationName, TestTermNestedCollect,
  ::testing::Combine(
    ::testing::Values(ORDER::before, ORDER::after, ORDER::misc),
    ::testing::Values(false),
    ::testing::Range(2,10,2)
  )
);

INSTANTIATE_TEST_CASE_P(
  InstantiationName, TestTermRooted,
  ::testing::Combine(
    ::testing::Values(ORDER::before, ORDER::after, ORDER::misc),
    ::testing::Bool(),
    ::testing::Values(1)
  )
);

INSTANTIATE_TEST_CASE_P(
  InstantiationName, TestTermNestedRooted,
  ::testing::Combine(
    ::testing::Values(ORDER::before, ORDER::after, ORDER::misc),
    ::testing::Bool(),
    ::testing::Range(2,10,2)
  )
);

}}} // end namespace vt::tests::unit