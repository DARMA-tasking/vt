/*
//@HEADER
// *****************************************************************************
//
//                                  dispatch.h
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

#if !defined INCLUDED_VT_VRT_COLLECTION_DISPATCH_DISPATCH_H
#define INCLUDED_VT_VRT_COLLECTION_DISPATCH_DISPATCH_H

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
  virtual ~DispatchCollectionBase() {}

  virtual void
  broadcast(VirtualProxyType proxy, void* msg, HandlerType han) = 0;
  virtual void
  send(VirtualProxyType proxy, void* idx, void* msg, HandlerType han) = 0;

  template <typename=void>
  VirtualProxyType getDefaultProxy() const;

  template <typename=void>
  void setDefaultProxy(VirtualProxyType const& in_proxy);

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | default_proxy_;
  }

private:
  VirtualProxyType default_proxy_ = no_vrt_proxy;
};

template <typename ColT, typename MsgT>
struct DispatchCollection final : DispatchCollectionBase {
private:
  void broadcast(VirtualProxyType proxy, void* msg, HandlerType han) override;
  void send(
    VirtualProxyType proxy, void* idx, void* msg, HandlerType han
  ) override;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_DISPATCH_DISPATCH_H*/
