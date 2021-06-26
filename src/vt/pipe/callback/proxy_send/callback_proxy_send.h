/*
//@HEADER
// *****************************************************************************
//
//                            callback_proxy_send.h
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

#if !defined INCLUDED_VT_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_H
#define INCLUDED_VT_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/vrt/collection/active/active_funcs.h"
#include "vt/vrt/proxy/collection_elm_proxy.h"
#include "vt/vrt/proxy/collection_proxy.h"

#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename ColT, typename MsgT>
struct CallbackProxySend : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType   = typename signal::Signal<MsgT>;
  using SignalType       = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType   = typename SignalType::DataType;
  using IndexedProxyType = typename ColT::ProxyType;
  using ProxyType        = typename ColT::CollectionProxyType;
  using IndexType        = typename ColT::IndexType;
  using MessageType      = MsgT;

  CallbackProxySend(
    HandlerType const in_handler, IndexedProxyType const& in_proxy
  ) : proxy_(in_proxy.getCollectionProxy()),
      idx_(in_proxy.getElementProxy().getIndex()), handler_(in_handler)
  { }

  CallbackProxySend(
    HandlerType const in_handler, ProxyType const& in_proxy,
    IndexType const& in_idx
  ) : proxy_(in_proxy), idx_(in_idx), handler_(in_handler)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  using CallbackBase<signal::Signal<MsgT>>::trigger_;
  void trigger_(SignalDataType* data) override;

private:
  ProxyType proxy_     = {};
  IndexType idx_       = {};
  HandlerType handler_ = uninitialized_handler;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_VT_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_H*/
