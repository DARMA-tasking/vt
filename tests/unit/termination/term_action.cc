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

#define DEBUG_TERM_ACTION 0

namespace vt { namespace tests { namespace unit {

// specify the order of 'addAction' w.r.t 'finishedEpoch'
enum class ORDER : int { before, after, misc };

struct MyParam {
  ORDER order = ORDER::before;
  bool useDS = false;
};

struct TestTermAction : vt::tests::unit::TestParallelHarnessParam<MyParam> {

  struct Metadata; // forward-declaration for 'data'

  static vt::NodeType me;
  static vt::NodeType root;
  static vt::NodeType all;
  static std::unordered_map<vt::EpochType,Metadata> data;

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


  template<typename Msg, vt::ActiveTypedFnType<Msg>* handler>
  static void sendMsg(vt::NodeType dst, int count, vt::EpochType ep){
    vtAssertExpr(dst not_eq me);
    auto msg = makeSharedMessage<Msg>(me,dst,count,ep);
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
  static void sendBasic(vt::NodeType dst, vt::EpochType ep){
    sendMsg<BasicMsg,basicHandler>(dst,1,ep);
    // increment outgoing message counter
    data[ep].count_[dst].out_++;
  }

  static void routeBasic(vt::NodeType dst, int ttl, vt::EpochType ep){
    sendMsg<BasicMsg,routedHandler>(dst,ttl,ep);
    // increment outgoing message counter
    data[ep].count_[dst].out_++;
  }

  static void sendPing(vt::NodeType dst, int count, vt::EpochType ep){
    sendMsg<CtrlMsg,pingHandler>(dst,count,ep);
  }

  static void sendEcho(vt::NodeType dst, int count, vt::EpochType ep){
    sendMsg<CtrlMsg,echoHandler>(dst,count,ep);
  }

  // trigger termination detection by root
  static void trigger(vt::EpochType ep){
    vtAssert(me == root, "Only root may trigger termination check");
    for(auto& active: data[ep].count_){
      auto const& dst = active.first;

      if(dst not_eq me){
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
          #if DEBUG_TERM_ACTION
            fmt::print(
              "{}: propagate: sendPing to {}, out={}, degree={}\n",
              me,i,nb.out_,degree+1
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
      int const nb_rounds = static_cast<int>(drand48()*5);
      for(int k=0; k < nb_rounds; ++k){
        // note: root and self-send are excluded
        int dst = (me+1 > all-1 ? 1: me+1);
        if (dst == me && dst == 1) dst++;
        vtAssertExpr(dst > 1 || me != 1);

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

    #if DEBUG_TERM_ACTION
      fmt::print(
        "{}: echoHandler: in={}, ack={}, degree={}\n",
        me,msg->count_,nb_ack, data[ep].degree_
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

    #if DEBUG_TERM_ACTION
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
    vt::tests::unit::TestParallelHarnessParam<MyParam>::SetUp();

    // set ranks
    root = 0;
    me  = vt::theContext()->getNode();
    all = vt::theContext()->getNumNodes();
    vtAssert(all > 1, "There should be at least two nodes");
  }



  // assign action to be processed
  // at the end of the epoch;
  // trigger termination detection;
  // and finalize epoch.
  void finalize(vt::EpochType ep){

    auto const& param = GetParam();

    if(ep not_eq vt::no_epoch){
      switch(param.order){
        case ORDER::after :{
          trigger(ep);
          vt::theTerm()->finishedEpoch(ep);
          vt::theTerm()->addActionEpoch(ep,[&]{ EXPECT_TRUE(hasFinished(ep)); });
          break;
        }
        case ORDER::misc :{
          trigger(ep);
          vt::theTerm()->addActionEpoch(ep,[&]{ EXPECT_TRUE(hasFinished(ep)); });
          vt::theTerm()->finishedEpoch(ep);
          break;
        }
        default :{
          trigger(ep);
          vt::theTerm()->addActionEpoch(ep,[&]{ EXPECT_TRUE(hasFinished(ep)); });
          vt::theTerm()->finishedEpoch(ep);
          break;
        }
      }
    } else {
      trigger(vt::no_epoch);
      vt::theTerm()->addAction([]{ EXPECT_TRUE(hasFinished(vt::no_epoch)); });
    }
  }
};

/*static*/ vt::NodeType TestTermAction::me;
/*static*/ vt::NodeType TestTermAction::root;
/*static*/ vt::NodeType TestTermAction::all;
/*static*/ std::unordered_map<vt::EpochType,TestTermAction::Metadata> TestTermAction::data;

TEST_P(TestTermAction, test_term_detect_broadcast)
{
  if(me == root){
    // start computation
    broadcast<basicHandler>(1,vt::no_epoch);
    // trigger termination detection
    trigger(vt::no_epoch);

    // check status
    vt::theTerm()->addAction([]{
      EXPECT_TRUE(hasFinished(vt::no_epoch));
    });
  }
}

TEST_P(TestTermAction, test_term_detect_routed)
{
  // there should be at least 3 nodes for this case
  if(all > 2 && me == root){

    std::random_device device;
    std::mt19937 engine(device());

    std::uniform_int_distribution<int> dist_ttl(1,10);
    std::uniform_int_distribution<int> dist_dest(1,all-1);
    std::uniform_int_distribution<int> dist_round(1,10);

    int const rounds = dist_round(engine);
    // start computation
    for(int k=0; k < rounds; ++k){
      int dst = me + dist_dest(engine);
      int ttl = dist_ttl(engine);
      routeBasic(dst,ttl,vt::no_epoch);
    }

    // trigger termination detection
    trigger(vt::no_epoch);

    // check status
    vt::theTerm()->addAction([]{
      EXPECT_TRUE(hasFinished(vt::no_epoch));
    });
  }
}


TEST_P(TestTermAction, test_term_detect_collect_epoch)
{
  int const nb_ep = 4;
  vt::EpochType ep[nb_ep];

  // create epoch in a collective way
  for(int i=0; i < nb_ep; ++i){
    ep[i] = vt::theTerm()->makeEpochCollective();
  }

  if(me == root){
    std::random_device device;
    std::mt19937 engine(device());

    std::uniform_int_distribution<int> dist_ttl(1,10);
    std::uniform_int_distribution<int> dist_dest(1,all-1);
    std::uniform_int_distribution<int> dist_round(1,10);

    // process for each epoch
    for(int i=0; i < nb_ep; ++i){
      int rounds = dist_round(engine);
      // start computation
      if(all > 2){
        for(int k=0; k < rounds; ++k){
          int dst = me + dist_dest(engine);
          int ttl = dist_ttl(engine);
          routeBasic(dst,ttl,ep[i]);
        }
      } else {
        broadcast<basicHandler>(1,ep[i]);
      }

      // trigger termination detection
      trigger(ep[i]);
    }
  }
  //
  for(int i=0; i < nb_ep; ++i){
    vt::theTerm()->finishedEpoch(ep[i]);
  }

  if(me == root){
    // check that all messages related to a
    // given epoch have been delivered
    // nb: only consider root counters.
    for(int i=0; i < nb_ep; ++i){
      vt::theTerm()->addActionEpoch(ep[i],[&]{
        EXPECT_TRUE(hasFinished(ep[i]));
      });
    }
  }
}


TEST_P(TestTermAction, test_term_detect_rooted_epoch)
{
  if(me == root){

    int const nb_ep = 5;
    vt::EpochType ep[nb_ep];

    auto const& param = GetParam();

    // create rooted epoch sequence
    ep[0] = vt::theTerm()->makeEpochRooted(param.useDS);
    vtAssertExpr(root == epoch::EpochManip::node(ep[0]));
    for(int i=1; i < nb_ep; ++i){
      ep[i] = epoch::EpochManip::next(ep[i-1]);
    }
    //
    std::random_device device;
    std::mt19937 engine(device());
    std::uniform_int_distribution<int> dist_ttl(1,10);
    std::uniform_int_distribution<int> dist_dest(1,all-1);
    std::uniform_int_distribution<int> dist_round(1,10);

    for(int i=0; i < nb_ep; ++i){

      #if DEBUG_TERM_ACTION
        fmt::print(
          "node:{}, i:{}, epoch: {}, is_rooted ? {}\n",
          me, i, ep[i], epoch::EpochManip::isRooted(ep[i])
        );
      #endif
      vtAssertExpr(epoch::EpochManip::isRooted(ep[i]));

      // process computation for current epoch
      int rounds = dist_round(engine);
      if(all > 2){
        for(int k=0; k < rounds; ++k){
          int dst = me + dist_dest(engine);
          int ttl = dist_ttl(engine);
          routeBasic(dst,ttl,ep[i]);
        }
      } else {
        broadcast<basicHandler>(1,ep[i]);
      }
      //
      finalize(ep[i]);
    }
  }
}


INSTANTIATE_TEST_CASE_P(
  InstantiationName, TestTermAction,
  ::testing::Values(
    MyParam{ORDER::before, false}, MyParam{ORDER::before, true},
    MyParam{ORDER::after, false}, MyParam{ORDER::after, true},
    MyParam{ORDER::misc, false}, MyParam{ORDER::misc, true}
  )
);

}}} // end namespace vt::tests::unit
