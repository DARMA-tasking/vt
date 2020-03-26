/*
//@HEADER
// *****************************************************************************
//
//                            serialized_messenger.h
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

#if !defined INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_H
#define INCLUDED_SERIALIZATION_MESSAGING_SERIALIZED_MESSENGER_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/messaging/pending_send.h"
#include "vt/serialization/messaging/serialized_data_msg.h"

#include <tuple>
#include <type_traits>
#include <cstdlib>
#include <functional>

using namespace ::serialization::interface;

namespace vt { namespace serialization {

template <typename MsgT, typename BaseEagerMsgT>
using SerializedEagerMsg = SerialEagerPayloadMsg<MsgT, BaseEagerMsgT>;

template <typename MsgT, typename BaseEagerMsgT>
using ActionEagerSend = std::function<messaging::PendingSend(
  MsgSharedPtr<SerializedEagerMsg<MsgT,BaseEagerMsgT>> msg
)>;
using ActionNodeSendType = std::function<messaging::PendingSend(NodeType)>;
using ActionDataSend = std::function<messaging::PendingSend(ActionNodeSendType)>;

struct SerializedMessenger {
  template <typename UserMsgT>
  using SerialWrapperMsgType = SerializedDataMsg<UserMsgT>;

  template <typename UserMsgT>
  static void serialMsgHandlerBcast(
    SerialWrapperMsgType<UserMsgT>* sys_msg
  );

  template <typename UserMsgT>
  static void serialMsgHandler(
    SerialWrapperMsgType<UserMsgT>* sys_msg
  );

  template <typename UserMsgT, typename BaseEagerMsgT>
  static void payloadMsgHandler(
    SerialEagerPayloadMsg<UserMsgT, BaseEagerMsgT>* sys_msg
  );

  template <typename MsgT, typename BaseT = Message>
  static messaging::PendingSend sendSerialMsg(
    NodeType dest, MsgT* msg, HandlerType han,
    ActionEagerSend<MsgT, BaseT> eager = nullptr
  );

  template <typename MsgT, typename BaseT = Message>
  static messaging::PendingSend broadcastSerialMsg(
    MsgT* msg, HandlerType han
  );

  template <typename MsgT, typename BaseT = Message>
  static messaging::PendingSend sendSerialMsgSendImpl(
    MsgT* msg, HandlerType han,
    ActionEagerSend<MsgT, BaseT> eager, ActionDataSend sender
  );
};

}} /* end namespace vt::serialization */

namespace vt {

using SerializedMessenger = ::vt::serialization::SerializedMessenger;

} /* end namespace vt */

#include "vt/serialization/messaging/serialized_messenger.impl.h"

#endif /*INCLUDED_SERIALIZATION_MESSAGING/SERIALIZED_MESSENGER_H*/
