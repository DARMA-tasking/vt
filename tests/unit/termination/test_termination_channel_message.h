/*
//@HEADER
// *****************************************************************************
//
//                      test_termination_channel_message.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_UNIT_TERMINATION_TEST_TERMINATION_CHANNEL_MESSAGE_H
#define INCLUDED_UNIT_TERMINATION_TEST_TERMINATION_CHANNEL_MESSAGE_H

#include "data_message.h"
#include "vt/configs/types/types_type.h"
#include "vt/messaging/message/message.h"

namespace vt { namespace tests { namespace unit { namespace channel {

// basic message
struct BasicMsg : vt::Message {

  int           ttl_   = 0; // time to live for message routing
  vt::NodeType  src_   = vt::uninitialized_destination;
  vt::NodeType  dst_   = vt::uninitialized_destination;
  vt::EpochType epoch_ = vt::no_epoch;

  BasicMsg() = default;
  BasicMsg(
    vt::NodeType in_src, vt::NodeType in_dst,
    int in_ttl = 1, vt::EpochType in_epoch = vt::no_epoch
  ) : ttl_  (in_ttl - 1),
      src_  (in_src),
      dst_  (in_dst),
      epoch_(in_epoch)
  {}

  ~BasicMsg() = default;
};

// control messages
struct CtrlMsg : vt::Message {

  int           count_ = 0; // incoming/outgoing basic message count
  vt::NodeType  src_   = vt::uninitialized_destination;
  vt::NodeType  dst_   = vt::uninitialized_destination;
  vt::EpochType epoch_ = vt::no_epoch;

  CtrlMsg() = default;
  CtrlMsg(
    vt::NodeType in_src, vt::NodeType in_dst,
    int in_nb = 0, vt::EpochType in_epoch = vt::no_epoch
  ) : count_(in_nb),
      src_  (in_src),
      dst_  (in_dst),
      epoch_(in_epoch)
  {}

  ~CtrlMsg() = default;
};

}}}} // end namespace vt::tests::unit::channel

#endif /*INCLUDED_UNIT_TERMINATION_TEST_TERMINATION_CHANNEL_MESSAGE_H*/
