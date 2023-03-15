/*
//@HEADER
// *****************************************************************************
//
//                                manager.fwd.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_OBJGROUP_MANAGER_FWD_H
#define INCLUDED_VT_OBJGROUP_MANAGER_FWD_H

#include "vt/config.h"
#include "vt/messaging/message/smart_ptr.h"
#include "vt/messaging/pending_send.fwd.h"

namespace vt { namespace objgroup {

namespace holder {
struct HolderBase;
} /* end namespace holder */

struct ObjGroupManager;

namespace detail {
holder::HolderBase* getHolderBase(HandlerType handler);
} /* end namespace detail */

template <typename MsgT>
void dispatchObjGroup(
  MsgSharedPtr<MsgT> msg, HandlerType han, NodeType from_node, ActionType cont
);

std::unordered_map<ObjGroupProxyType, std::unique_ptr<holder::HolderBase>>& getObjs();
std::unordered_map<ObjGroupProxyType, std::vector<ActionType>>& getPending();

template <typename MsgT>
messaging::PendingSend send(MsgSharedPtr<MsgT> msg, HandlerType han, NodeType node);
template <typename ObjT, typename MsgT, auto f>
decltype(auto) invoke(messaging::MsgSharedPtr<MsgT> msg, HandlerType han, NodeType node);
template <typename MsgT>
messaging::PendingSend broadcast(MsgSharedPtr<MsgT> msg, HandlerType han);

}} /* end namespace vt::objgroup */

namespace vt {

extern objgroup::ObjGroupManager* theObjGroup();

} // end namespace vt

#include "vt/objgroup/manager.static.h"

#endif /*INCLUDED_VT_OBJGROUP_MANAGER_FWD_H*/
