/*
//@HEADER
// ************************************************************************
//
//                test_termination_channel_counting.h
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

#include <unordered_map>

#include "test_termination_channel_message.h"

#if !defined INCLUDED_TERMINATION_CHANNEL_COUNTING_H
#define INCLUDED_TERMINATION_CHANNEL_COUNTING_H

#define DEBUG_CHANNEL_COUNTING 0

/*
 * channel counting termination detection.
 * all methods are required to be static,
 * so just embed them within a namespace.
 */
namespace vt { namespace tests { namespace unit { namespace channel {

// data per rank and epoch
struct Data {
  struct Counters {
    int in_  = 0; // incoming
    int out_ = 0; // outgoing
    int ack_ = 0; // acknowledged
  };

  Data();
  ~Data();

  int degree_ = 0;
  vt::NodeType activator_ = 0;
  std::unordered_map<vt::NodeType,Counters> count_;
};

// send any kind of message
template <typename Msg, vt::ActiveTypedFnType<Msg>* handler>
void sendMsg(vt::NodeType dst, int count, vt::EpochType ep);
// broadcast basic message
template <vt::ActiveTypedFnType<BasicMsg>* handler>
void broadcast(int count, vt::EpochType ep);

// route basic message, send control messages
void routeBasic(vt::NodeType dst, int ttl, vt::EpochType ep);
void sendPing(vt::NodeType dst, int count, vt::EpochType ep);
void sendEcho(vt::NodeType dst, int count, vt::EpochType ep);

// on receipt of a basic message.
void basicHandler(BasicMsg* msg);
void routedHandler(BasicMsg* msg);
void echoHandler(CtrlMsg* msg);
void pingHandler(CtrlMsg* msg);

// trigger, propagate and check
// termination of an epoch
void trigger(vt::EpochType ep);
void propagate(vt::EpochType ep);
bool hasEnded(vt::EpochType ep);

// ranks
extern vt::NodeType me;
extern vt::NodeType root;
extern vt::NodeType all;
// channel counters per epoch and per rank
extern std::unordered_map<vt::EpochType,Data> data;

}}}} // end namespace vt::tests::unit::channel

/*
 * cannot move implementation within a .cc file
 * due to unit test linking rules which will
 * create a test target for each .cc file.
 * by the way, it contains template methods impl.
 */
#include "test_termination_channel_counting.impl.h"

#endif /*INCLUDED_TERMINATION_CHANNEL_COUNTING_H*/