/*
//@HEADER
// *****************************************************************************
//
//                              dispatch_handler.h
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

#if !defined INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_HANDLER_H
#define INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_HANDLER_H

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/pending_send.h"

#include <checkpoint/checkpoint.h>

namespace vt { namespace serialization { namespace auto_dispatch {

template <typename MsgT>
struct SenderHandler {
  static messaging::PendingSend sendMsg(
    NodeType const& node, MsgT* msg, HandlerType const& handler,
    TagType const& tag
  );
};

template <typename MsgT>
struct SenderSerializeHandler {
  static messaging::PendingSend sendMsg(
    NodeType const& node, MsgT* msg, HandlerType const& han, TagType const& tag
  );
  static messaging::PendingSend sendMsgParserdes(
    NodeType const& node, HandlerType const& han, MsgT* msg, TagType const& tag
  );
};

template <typename MsgT>
struct BroadcasterHandler {
  static messaging::PendingSend broadcastMsg(
    MsgT* msg, HandlerType const& handler, TagType const& tag
  );
};

template <typename MsgT>
struct BroadcasterSerializeHandler {
  static messaging::PendingSend broadcastMsg(
    MsgT* msg, HandlerType const& handler, TagType const& tag
  );
  static messaging::PendingSend broadcastMsgParserdes(
    MsgT* msg, HandlerType const& han, TagType const& tag
  );
};

template <typename MsgT, typename=void>
struct RequiredSerializationHandler {
  static messaging::PendingSend sendMsg(
    NodeType const& node, MsgT* msg, HandlerType const& handler,
    TagType const& tag = no_tag
  ) {
    return SenderHandler<MsgT>::sendMsg(node,msg,handler,tag);
  }
  static messaging::PendingSend broadcastMsg(
    MsgT* msg, HandlerType const& handler, TagType const& tag = no_tag
  ) {
    return BroadcasterHandler<MsgT>::broadcastMsg(msg,handler,tag);
  }
};

template <typename MsgT>
struct RequiredSerializationHandler<
  MsgT,
  typename std::enable_if_t<
    ::checkpoint::SerializableTraits<MsgT>::has_serialize_function
  >
> {
  static messaging::PendingSend sendMsg(
    NodeType const& node, MsgT* msg, HandlerType const& han,
    TagType const& tag = no_tag
  ) {
    return SenderSerializeHandler<MsgT>::sendMsg(node,msg,han,tag);
  }
  static messaging::PendingSend broadcastMsg(
    MsgT* msg, HandlerType const& han, TagType const& tag = no_tag
  ) {
    return BroadcasterSerializeHandler<MsgT>::broadcastMsg(msg,han,tag);
  }
};

}}} /* end namespace vt::serialization::auto_dispatch */

namespace vt {

template <typename MsgT>
using ActiveSendHandler =
  serialization::auto_dispatch::RequiredSerializationHandler<MsgT>;

} /* end namespace vt */

#include "vt/serialization/auto_dispatch/dispatch_handler.impl.h"

#endif /*INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_HANDLER_H*/
