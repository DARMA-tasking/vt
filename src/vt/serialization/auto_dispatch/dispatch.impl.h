/*
//@HEADER
// *****************************************************************************
//
//                               dispatch.impl.h
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

#if !defined INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_IMPL_H
#define INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_IMPL_H

#include "vt/config.h"
#include "vt/serialization/auto_dispatch/dispatch.h"
#include "vt/serialization/serialize_interface.h"
#include "vt/serialization/messaging/serialized_messenger.h"
#include "vt/messaging/active.h"

#include <cassert>

namespace vt { namespace serialization { namespace auto_dispatch {

/*
 * Regular message send/broadcast pass-though (acts as delegate)
 */
template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ messaging::PendingSend Sender<MsgT,f>::sendMsg(
  NodeType const& node, MsgT* msg, TagType const& tag
) {
  return theMsg()->sendMsg<MsgT,f>(node,msg,tag);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ messaging::PendingSend Broadcaster<MsgT,f>::broadcastMsg(
  MsgT* msg, TagType const& tag
) {
  return theMsg()->broadcastMsg<MsgT,f>(msg,tag);
}

/*
 * Serialization message send/broadcast detected based on the is_serializable
 * type traits
 */
template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ messaging::PendingSend SenderSerialize<MsgT,f>::sendMsgParserdes(
  NodeType const& node, MsgT* msg, TagType const& tag
) {
  vtAssert(tag == no_tag, "Tagged messages serialized not implemented");
  return SerializedMessenger::sendParserdesMsg<MsgT,f>(node,msg);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ messaging::PendingSend SenderSerialize<MsgT,f>::sendMsg(
  NodeType const& node, MsgT* msg, TagType const& tag
) {
  vtAssert(tag == no_tag, "Tagged messages serialized not implemented");
  return SerializedMessenger::sendSerialMsg<MsgT,f>(node,msg);
}


template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ messaging::PendingSend BroadcasterSerialize<MsgT,f>::broadcastMsgParserdes(
  MsgT* msg, TagType const& tag
) {
  vtAssert(tag == no_tag, "Tagged messages serialized not implemented");
  return SerializedMessenger::broadcastParserdesMsg<MsgT,f>(msg);
}


template <typename MsgT, ActiveTypedFnType<MsgT>* f>
/*static*/ messaging::PendingSend BroadcasterSerialize<MsgT,f>::broadcastMsg(
  MsgT* msg, TagType const& tag
) {
  vtAssert(tag == no_tag, "Tagged messages serialized not implemented");
  return SerializedMessenger::broadcastSerialMsg<MsgT,f>(msg);
}

}}} /* end namespace vt::serialization::auto_dispatch */

#endif /*INCLUDED_SERIALIZATION_AUTO_DISPATCH_DISPATCH_IMPL_H*/
