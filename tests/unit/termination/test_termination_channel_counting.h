/*
//@HEADER
// *****************************************************************************
//
//                     test_termination_channel_counting.h
//                           DARMA Toolkit v. 1.0.0
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

#include <unordered_map>

#include "test_termination_channel_message.h"

#if !defined INCLUDED_TERMINATION_CHANNEL_COUNTING_H
#define INCLUDED_TERMINATION_CHANNEL_COUNTING_H

/*
 * channel counting termination detection.
 * all methods are required to be static,
 * so just embed them within a namespace.
 */
namespace vt { namespace tests { namespace unit { namespace channel {

// ranks
extern vt::NodeType node;
extern vt::NodeType root;
extern vt::NodeType all;

// data per rank and epoch
struct Data {
  struct Counters {
    int in_  = 0; // incoming
    int out_ = 0; // outgoing
    int ack_ = 0; // acknowledged
  };

  Data()
    : degree_(0),
      activator_(0)
  {
    for (vt::NodeType dst = 0; dst < all; ++dst) {
      count_[dst] = {0, 0, 0};
    }
  }

  ~Data() { count_.clear(); }

  int degree_ = 0;
  vt::NodeType activator_ = 0;
  std::unordered_map<vt::NodeType,Counters> count_;
};

// channel counters per epoch and per rank
extern std::unordered_map<vt::EpochType,Data> data;

// send any kind of message
template <typename Msg, vt::ActiveTypedFnType<Msg>* handler>
void sendMsg(vt::NodeType dst, int count, vt::EpochType ep);
// broadcast basic message
template <vt::ActiveTypedFnType<BasicMsg>* handler>
void broadcast(int count, vt::EpochType ep);

// route basic message, send control messages
inline void routeBasic(vt::NodeType dst, int ttl, vt::EpochType ep);
inline void sendPing(vt::NodeType dst, int count, vt::EpochType ep);
inline void sendEcho(vt::NodeType dst, int count, vt::EpochType ep);

// on receipt of a basic message.
inline void basicHandler(BasicMsg* msg);
inline void routedHandler(BasicMsg* msg);
inline void echoHandler(CtrlMsg* msg);
inline void pingHandler(CtrlMsg* msg);

// trigger, propagate and check
// termination of an epoch
inline void trigger(vt::EpochType ep);
inline void propagate(vt::EpochType ep);
inline bool hasEnded(vt::EpochType ep);

}}}} // end namespace vt::tests::unit::channel

/*
 * cannot move implementation within a .cc file
 * due to unit test linking rules which will
 * create a test target for each .cc file.
 * by the way, it contains template methods impl.
 */
#include "test_termination_channel_counting.impl.h"

#endif /*INCLUDED_TERMINATION_CHANNEL_COUNTING_H*/
