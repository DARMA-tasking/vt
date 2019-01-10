/*
//@HEADER
// ************************************************************************
//
//                          dispatch.impl.h
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

#if !defined INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_IMPL_H
#define INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_IMPL_H

#include "vt/config.h"
#include "vt/vrt/collection/dispatch/dispatch.h"
#include "vt/vrt/proxy/collection_proxy.h"
#include "vt/vrt/collection/traits/coll_msg.h"
#include "vt/vrt/collection/manager.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename MsgT>
void DispatchCollection<ColT,MsgT>::broadcast(
  VirtualProxyType proxy, void* msg, HandlerType han, bool member,
  ActionType action
) {
  using IdxT = typename ColT::IndexType;
  auto const msg_typed = reinterpret_cast<MsgT*>(msg);
  CollectionProxy<ColT,IdxT> typed_proxy{proxy};
  theCollection()->broadcastMsgWithHan<MsgT,ColT>(
    typed_proxy,msg_typed,han,member,action,true
  );
}

template <typename ColT, typename MsgT>
void DispatchCollection<ColT,MsgT>::send(
  VirtualProxyType proxy, void* idx, void* msg, HandlerType han, bool member,
  ActionType action
) {
  using IdxT = typename ColT::IndexType;
  auto const msg_typed = reinterpret_cast<MsgT*>(msg);
  auto const idx_typed = reinterpret_cast<IdxT*>(idx);
  auto const& idx_typed_ref = *idx_typed;
  VrtElmProxy<ColT,IdxT> typed_proxy{proxy,idx_typed_ref};
  theCollection()->sendMsgWithHan<MsgT,ColT>(
    typed_proxy,msg_typed,han,member,action
  );
}

template <typename always_void_>
VirtualProxyType DispatchCollectionBase::getDefaultProxy() const {
  vtAssert(default_proxy_ != no_vrt_proxy, "Must be valid proxy");
  return default_proxy_;
}

template <typename always_void_>
void DispatchCollectionBase::setDefaultProxy(VirtualProxyType const& proxy) {
  default_proxy_ = proxy;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_IMPL_H*/
