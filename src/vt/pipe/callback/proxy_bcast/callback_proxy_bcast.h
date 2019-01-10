/*
//@HEADER
// ************************************************************************
//
//                          callback_proxy_bcast.h
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

#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/vrt/collection/manager.h"

#include <functional>
#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename ColT, typename MsgT>
struct CallbackProxyBcast : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType  = typename signal::Signal<MsgT>;
  using SignalType      = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType  = typename SignalType::DataType;
  using ProxyType       = typename ColT::CollectionProxyType;
  using MessageType     = MsgT;

  CallbackProxyBcast() = default;
  CallbackProxyBcast(CallbackProxyBcast const&) = default;
  CallbackProxyBcast(CallbackProxyBcast&&) = default;

  CallbackProxyBcast(HandlerType const& in_handler, ProxyType const& in_proxy)
    : proxy_(in_proxy), handler_(in_handler)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | proxy_;
    s | handler_;
  }

private:
  void trigger_(SignalDataType* data) override {
    theCollection()->broadcastMsgUntypedHandler(
      proxy_,data,handler_,false,nullptr,true
    );
  }

private:
  ProxyType proxy_     = {};
  HandlerType handler_ = uninitialized_handler;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_PROXY_BCAST_H*/
