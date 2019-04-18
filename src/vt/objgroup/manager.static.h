/*
//@HEADER
// ************************************************************************
//
//                         manager.static.h
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
    theMsg()->sendMsgAuto<MsgT>(dest_node,han,msg.get(),no_tag);
  } else {
    // Schedule the work of dispatching the message handler for later
    scheduleMsg(msg.template toVirtual<ShortMessage>(),han);
  }
}

template <typename MsgT>
void broadcast(MsgSharedPtr<MsgT> msg, HandlerType han) {
  theMsg()->broadcastMsgAuto<MsgT>(han,msg.get(),no_tag);
  // Schedule delivery on this node for the objgroup
  scheduleMsg(msg.template toVirtual<ShortMessage>(),han);
}

}} /* end namespace vt::objgroup */

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_STATIC_H*/
