/*
//@HEADER
// ************************************************************************
//
//                          dispatch.h
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

#if !defined INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_H
#define INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_H

#include "vt/config.h"
#include "vt/vrt/collection/traits/coll_msg.h"

#include <type_traits>

namespace vt { namespace vrt { namespace collection {

struct DispatchCollectionBase {
  template <typename T, typename U=void>
  using IsColMsgType = std::enable_if_t<ColMsgTraits<T>::is_coll_msg>;
  template <typename T, typename U=void>
  using IsNotColMsgType = std::enable_if_t<!ColMsgTraits<T>::is_coll_msg>;

  DispatchCollectionBase() = default;

public:
  virtual void broadcast(
    VirtualProxyType proxy, void* msg, HandlerType han, bool member,
    ActionType action
  ) = 0;
  virtual void send(
    VirtualProxyType proxy, void* idx, void* msg, HandlerType han, bool member,
    ActionType action
  ) = 0;

  template <typename=void>
  VirtualProxyType getDefaultProxy() const;

  template <typename=void>
  void setDefaultProxy(VirtualProxyType const& in_proxy);

private:
  VirtualProxyType default_proxy_ = no_vrt_proxy;
};

template <typename ColT, typename MsgT>
struct DispatchCollection final : DispatchCollectionBase {
private:
  void broadcast(
    VirtualProxyType proxy, void* msg, HandlerType han, bool member,
    ActionType action
  ) override;
  void send(
    VirtualProxyType proxy, void* idx, void* msg, HandlerType han, bool member,
    ActionType action
  ) override;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DISPATCH_DISPATCH_H*/
