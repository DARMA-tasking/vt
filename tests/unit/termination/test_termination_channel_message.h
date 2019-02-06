/*
//@HEADER
// ************************************************************************
//
//                test_termination_channel_message.h
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

#include "data_message.h"
#include "vt/transport.h"

#if !defined INCLUDED_TERMINATION_CHANNEL_MESSAGES_H
#define INCLUDED_TERMINATION_CHANNEL_MESSAGES_H

namespace vt { namespace tests { namespace unit { namespace channel {

// basic message
struct BasicMsg : vt::Message {

  BasicMsg() = default;
  BasicMsg(vt::NodeType src, vt::NodeType dst, int ttl, vt::EpochType epoch)
    : src_(src),
      dst_(dst),
      ttl_(ttl-1),
      epoch_(epoch)
  {}
  BasicMsg(vt::NodeType src, vt::NodeType dst, int ttl) : BasicMsg(src,dst,ttl,vt::no_epoch) {}
  BasicMsg(vt::NodeType src, vt::NodeType dst) : BasicMsg(src,dst,1,vt::no_epoch) {}

  //
  int ttl_ = 0;
  vt::NodeType src_ = vt::uninitialized_destination;
  vt::NodeType dst_ = vt::uninitialized_destination;
  vt::EpochType epoch_ = vt::no_epoch;
};

// control messages
struct CtrlMsg : vt::Message {

  CtrlMsg() = default;
  CtrlMsg(vt::NodeType src, vt::NodeType dst, int nb, vt::EpochType epoch)
    : src_(src),
      dst_(dst),
      count_(nb),
      epoch_(epoch)
  {}
  CtrlMsg(vt::NodeType src, vt::NodeType dst, int nb) : CtrlMsg(src,dst,nb,vt::no_epoch) {}
  CtrlMsg(vt::NodeType src, vt::NodeType dst) : CtrlMsg(src,dst,0,vt::no_epoch) {}

  // incoming/outgoing basic message count
  int count_ = 0;
  vt::NodeType src_ = vt::uninitialized_destination;
  vt::NodeType dst_ = vt::uninitialized_destination;
  vt::EpochType epoch_ = vt::no_epoch;
};

}}}} // end namespace vt::tests::unit::channel

#endif /*INCLUDED_TERMINATION_CHANNEL_MESSAGES_H*/