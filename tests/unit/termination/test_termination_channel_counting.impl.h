/*
//@HEADER
// ************************************************************************
//
//              test_termination_channel_counting.impl.h
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

#if !defined INCLUDED_TERMINATION_CHANNEL_COUNTING_IMPL_H
#define INCLUDED_TERMINATION_CHANNEL_COUNTING_IMPL_H

namespace vt { namespace tests { namespace unit { namespace channel {

Data::Data() : degree_(0), activator_(0) {
  for (vt::NodeType dst = 0; dst < all; ++dst) {
    count_[dst] = {0,0,0};
  }
}

Data::~Data() { count_.clear(); }

template<typename Msg, vt::ActiveTypedFnType<Msg>* handler>
void sendMsg(vt::NodeType dst, int count, vt::EpochType ep) {
  vtAssertExpr(dst not_eq vt::uninitialized_destination);
  vtAssertExpr(dst not_eq node);
  auto msg = makeSharedMessage<Msg>(node,dst,count,ep);
  vtAssertExpr(msg->src_ == node);

  if (ep not_eq vt::no_epoch) {
    vt::envelopeSetEpoch(msg->env,ep);
  }
  vt::theMsg()->sendMsg<Msg,handler>(dst,msg);
}

// note: only for basic messages,
// but different handlers may be used.
template<vt::ActiveTypedFnType<BasicMsg>* handler>
void broadcast(int count, vt::EpochType ep) {
  auto msg = makeSharedMessage<BasicMsg>(node,vt::uninitialized_destination,count,ep);
  if (ep not_eq vt::no_epoch) {
    vt::envelopeSetEpoch(msg->env,ep);
  }
  vt::theMsg()->broadcastMsg<BasicMsg,handler>(msg);

  for (auto&& active : data[ep].count_) {
    auto const& dst = active.first;
    auto& nb = active.second;
    if (dst not_eq node) {
      nb.out_++;
    }
  }
}

void routeBasic(vt::NodeType dst, int ttl, vt::EpochType ep) {
  sendMsg<BasicMsg,routedHandler>(dst,ttl,ep);
  // increment outgoing message counter
  data[ep].count_[dst].out_++;
}

void sendPing(vt::NodeType dst, int count, vt::EpochType ep) {
  sendMsg<CtrlMsg,pingHandler>(dst,count,ep);
}

void sendEcho(vt::NodeType dst, int count, vt::EpochType ep) {
  vtAssertExpr(dst >= 0);
#if DEBUG_CHANNEL_COUNTING
  fmt::print("rank:{} echo::dst {}",node,dst);
#endif
  vtAssertExpr(dst not_eq vt::uninitialized_destination);
  sendMsg<CtrlMsg,echoHandler>(dst,count,ep);
}


// on receipt of a basic message.
// note: msg->dst is uninitialized on broadcast,
// thus related assertion was removed.
void basicHandler(BasicMsg* msg) {
  auto const& src = msg->src_;
  // avoid self sending case
  if (node not_eq src) {
    auto& nb = data[msg->epoch_].count_[src];
    nb.in_++;
  }
}

void routedHandler(BasicMsg* msg) {
  vtAssertExpr(node not_eq root);
  basicHandler(msg);

  if (all > 2 && msg->ttl_ > 0) {
    // avoid implicit cast
    vt::NodeType const one = 1;
    int const nb_rounds = static_cast<int>(drand48()*5);

    for (int k = 0; k < nb_rounds; ++k) {
      // note: root and self-send are excluded
      auto dst = (node+one > all-one ? one: node+one);
      if (dst == node && dst == one) dst++;
      vtAssertExpr(dst > one || node not_eq one);

      routeBasic(dst,msg->ttl_,msg->epoch_);
    }
  }
}

// on receipt of an echo message echo<m> from Pi
void echoHandler(CtrlMsg* msg) {
  vtAssertExpr(node == msg->dst_);
  // shortcuts for readability
  auto const& src = msg->src_;
  auto const& ep = msg->epoch_;
  auto& degree = data[ep].degree_;
  auto& nb_ack = data[ep].count_[src].ack_;

  // update ack and decrease missing echoes counter
  nb_ack = msg->count_;
  degree--;

#if DEBUG_CHANNEL_COUNTING
  auto& nb_in = data[ep].count_[src].in_;
    fmt::print(
      "rank {}: echoHandler: in={}, ack={}, degree={}\n",
      node,nb_in,nb_ack, degree
    );
#endif

  // last echo checks whether all subtrees are quiet
  if (degree == 0) {
    propagate(ep);
  }
  // all echoes arrived, everything quiet
  if (degree == 0) {
    // echo to parent activator node
    auto const& dst = data[ep].activator_;
    auto const& nb = data[ep].count_[dst];
    if (dst not_eq node) {
      sendEcho(dst,nb.in_,ep);
    }
  }
}

// on receipt of a control message test<m> from Pi
void pingHandler(CtrlMsg* msg) {
  vtAssertExpr(node == msg->dst_);
  auto const& src = msg->src_;
  auto const& ep = msg->epoch_;
  auto const& degree = data[ep].degree_;
  auto const& nb = data[ep].count_[src];
  auto& activator = data[ep].activator_;

#if DEBUG_CHANNEL_COUNTING
  fmt::print(
      "{}: pingHandler: in={}, src={}, degree={}\n",
      node,nb.in_,src,degree
    );
#endif

  // if already engaged or subtree is quiet
  if (degree > 0 or hasEnded(ep)) {
    sendEcho(src,nb.in_,ep);
  }
  else {
    activator = src;
    propagate(ep);
  }
}


// trigger termination detection by root
void trigger(vt::EpochType ep) {
  vtAssert(node == root, "Only root may trigger termination check");

  for (auto&& active : data[ep].count_) {
    auto const& dst = active.first;

    if (dst not_eq node) {
#if DEBUG_CHANNEL_COUNTING
      fmt::print("rank:{} trigger dst {}\n", node, dst);
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
void propagate(vt::EpochType ep) {
  for (auto&& active : data[ep].count_) {
    auto const& dst = active.first;
    if (dst not_eq node) {
      auto& nb = active.second;
      auto& degree = data[ep].degree_;
      // confirmation missing
      if (nb.out_ not_eq nb.ack_) {
#if DEBUG_CHANNEL_COUNTING
        fmt::print(
            "rank {}: propagate: sendPing to {}, out={}, degree={}\n",
            node,dst,nb.out_,degree+1
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

// check local termination, i.e if Cij^+ == Cij^-
bool hasEnded(vt::EpochType ep) {
  for (auto&& active : data[ep].count_) {
    auto const& dst = active.first;
    if (dst not_eq node) {
      auto const& nb = active.second;
      if (nb.out_ not_eq nb.ack_) {
        return false;
      }
    }
  }
  return true;
}

}}}} // end namespace vt::tests::unit::channel

#endif /*INCLUDED_TERMINATION_CHANNEL_COUNTING_IMPL_H*/