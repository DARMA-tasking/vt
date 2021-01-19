/*
//@HEADER
// *****************************************************************************
//
//                               manager.static.h
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

#if !defined INCLUDED_VT_OBJGROUP_MANAGER_STATIC_H
#define INCLUDED_VT_OBJGROUP_MANAGER_STATIC_H

#include "vt/config.h"
#include "vt/objgroup/common.h"
#include "vt/messaging/active.h"

namespace vt { namespace objgroup {

template <typename MsgT>
void send(MsgSharedPtr<MsgT> msg, HandlerType han, NodeType dest_node) {
  auto const num_nodes = theContext()->getNumNodes();
  auto const this_node = theContext()->getNode();
  vtAssert(dest_node < num_nodes, "Invalid node (must be < num_nodes)");
  if (dest_node != this_node) {
    theMsg()->sendMsg<MsgT>(dest_node, han,msg, no_tag);
  } else {
  // Get the current epoch for the message
    auto const cur_epoch = theMsg()->setupEpochMsg(msg);
    // Schedule the work of dispatching the message handler for later
    scheduleMsg(msg.template toVirtual<ShortMessage>(),han,cur_epoch);
  }
}

template <typename MsgT>
void invoke(messaging::MsgPtrThief<MsgT> msg, HandlerType han, NodeType dest_node) {
  auto const this_node = theContext()->getNode();

  vtAssert(
    dest_node == this_node,
    fmt::format(
      "Attempting to invoke handler on node:{} instead of node:{}!", this_node,
      dest_node
    )
  );

  runnable::Runnable<MsgT>::run(han, nullptr, msg.msg_.get(), this_node);
}

template <typename MsgT>
void broadcast(MsgSharedPtr<MsgT> msg, HandlerType han) {
  theMsg()->broadcastMsg<MsgT>(han, msg, no_tag);
}

}} /* end namespace vt::objgroup */

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_STATIC_H*/
