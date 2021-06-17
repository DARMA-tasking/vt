/*
//@HEADER
// *****************************************************************************
//
//                   test_termination_channel_counting.impl.h
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

#if !defined INCLUDED_TERMINATION_CHANNEL_COUNTING_IMPL_H
#define INCLUDED_TERMINATION_CHANNEL_COUNTING_IMPL_H

namespace vt { namespace tests { namespace unit { namespace channel {

template<typename Msg, vt::ActiveTypedFnType<Msg>* handler>
void sendMsg(vt::NodeType dst, int count, vt::EpochType ep) {
  vtAssert(dst != vt::uninitialized_destination, "Invalid destination");
  vtAssert(dst != node, "Invalid destination");

  auto msg = makeMessage<Msg>(node, dst, count, ep);
  if (ep != vt::no_epoch) {
    vt::envelopeSetEpoch(msg->env,ep);
  }
  vt::theMsg()->sendMsg<Msg,handler>(dst, msg);
}

// note: only for basic messages,
// but different handlers may be used.
template<vt::ActiveTypedFnType<BasicMsg>* handler>
void broadcast(int count, vt::EpochType ep) {
  auto msg = makeMessage<BasicMsg>(
    node, vt::uninitialized_destination, count, ep
  );
  if (ep != vt::no_epoch) {
    vt::envelopeSetEpoch(msg->env,ep);
  }
  vt::theMsg()->broadcastMsg<BasicMsg,handler>(msg);

  for (auto&& active : data[ep].count_) {
    auto const& dst = active.first;
    auto& nb = active.second;
    if (dst != node) {
      nb.out_++;
    }
  }
}

inline void routeBasic(vt::NodeType dst, int ttl, vt::EpochType ep) {
  sendMsg<BasicMsg,routedHandler>(dst, ttl, ep);
  // increment outgoing message counter
  data[ep].count_[dst].out_++;
}

inline void sendPing(vt::NodeType dst, int count, vt::EpochType ep) {
  sendMsg<CtrlMsg,pingHandler>(dst, count, ep);
}

inline void sendEcho(vt::NodeType dst, int count, vt::EpochType ep) {
  vt_debug_print(
    normal, term,
    "rank:{} echo::dst {}\n",
    node, dst
  );

  vtAssert(dst >= 0, "Invalid destination");
  vtAssert(dst != vt::uninitialized_destination, "Invalid destination");
  sendMsg<CtrlMsg, echoHandler>(dst, count, ep);
}


// on receipt of a basic message.
// note: msg->dst is uninitialized on broadcast,
// thus related assertion was removed.
inline void basicHandler(BasicMsg* msg) {
  auto const& src = msg->src_;
  // avoid self sending case
  if (node != src) {
    auto& nb = data[msg->epoch_].count_[src];
    nb.in_++;
  }
}

inline void routedHandler(BasicMsg* msg) {
  vtAssert(node != root, "Cannot route from root");
  basicHandler(msg);

  if (all > 2 && msg->ttl_ > 0) {
    // avoid implicit cast
    vt::NodeType const one = 1;
    int const nb_rounds = static_cast<int>(drand48()*5);

    for (int k = 0; k < nb_rounds; ++k) {
      // note: root and self-send are excluded
      auto dst = (node+one > all-one ? one: node+one);
      if (dst == node and dst == one) { dst++; }
      vtAssert(dst > one or node != one, "Invalid route destination");

      routeBasic(dst, msg->ttl_, msg->epoch_);
    }
  }
}

// on receipt of an echo message echo<m> from Pi
inline void echoHandler(CtrlMsg* msg) {
  vtAssert(node == msg->dst_, "Invalid destination");

  // shortcuts for readability
  auto const& src = msg->src_;
  auto const& ep = msg->epoch_;
  auto& degree = data[ep].degree_;
  auto& nb_ack = data[ep].count_[src].ack_;
  auto& nb_in  = data[ep].count_[src].in_;

  // update ack and decrease missing echoes counter
  nb_ack = msg->count_;
  degree--;

  vt_debug_print(
    normal, term,
    "rank {}: echoHandler: in={}, ack={}, degree={}\n",
    node,nb_in, nb_ack, degree
  );

  // last echo checks whether all subtrees are quiet
  if (degree == 0) {
    propagate(ep);
  }
  // all echoes arrived, everything quiet
  if (degree == 0) {
    // echo to parent activator node
    auto const& dst = data[ep].activator_;
    auto const& nb  = data[ep].count_[dst];
    if (dst != node) {
      sendEcho(dst, nb.in_, ep);
    }
  }
}

// on receipt of a control message test<m> from Pi
inline void pingHandler(CtrlMsg* msg) {
  vtAssert(node == msg->dst_, "Invalid destination");

  auto const& src = msg->src_;
  auto const& ep = msg->epoch_;
  auto const& degree = data[ep].degree_;
  auto const& nb = data[ep].count_[src];
  auto& activator = data[ep].activator_;

  vt_debug_print(
    normal, term,
    "{}: pingHandler: in={}, src={}, degree={}\n",
    node,nb.in_,src,degree
  );

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
inline void trigger(vt::EpochType ep) {
  vtAssert(node == root, "Only root may trigger termination check");

  for (auto&& active : data[ep].count_) {
    auto const& dst = active.first;

    if (dst != node) {
      vt_debug_print(
        normal, term,
        "rank:{} trigger dst {}\n",
        node, dst
      );

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
inline void propagate(vt::EpochType ep) {
  for (auto&& active : data[ep].count_) {
    auto const& dst = active.first;
    if (dst != node) {
      auto& nb = active.second;
      auto& degree = data[ep].degree_;
      // confirmation missing
      if (nb.out_ != nb.ack_) {
        vt_debug_print(
          normal, term,
          "rank {}: propagate: sendPing to {}, out={}, degree={}\n",
          node, dst, nb.out_, degree+1
        );
        // check subtree
        sendPing(dst,nb.out_,ep);
        // more echoes outstanding
        degree++;
      }
    }
  }
}

// check local termination, i.e if Cij^+ == Cij^-
inline bool hasEnded(vt::EpochType ep) {
  for (auto&& active : data[ep].count_) {
    auto const& dst = active.first;
    if (dst != node) {
      auto const& nb = active.second;
      if (nb.out_ != nb.ack_) {
        return false;
      }
    }
  }
  return true;
}

}}}} // end namespace vt::tests::unit::channel

#endif /*INCLUDED_TERMINATION_CHANNEL_COUNTING_IMPL_H*/
