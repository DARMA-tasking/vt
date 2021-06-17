/*
//@HEADER
// *****************************************************************************
//
//                           callback_proxy_send_tl.h
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

#if !defined INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H
#define INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/callback/callback_base_tl.h"
#include "vt/registry/auto/auto_registry_common.h"

namespace vt { namespace pipe { namespace callback {

struct CallbackProxySendTypeless : CallbackBaseTL<CallbackProxySendTypeless> {
  CallbackProxySendTypeless() = default;

  template <typename SerializerT>
  void serialize(SerializerT& s);

  bool operator==(CallbackProxySendTypeless const& other) const {
    return true;
  }

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);

  void triggerVoid(PipeType const& pipe) {
    vtAssert(0, "Must not be void");
  }
};

struct CallbackProxySendDirect : CallbackBaseTL<CallbackProxySendDirect> {
  using AutoHandlerType = auto_registry::AutoHandlerType;

  CallbackProxySendDirect() = default;
  CallbackProxySendDirect(HandlerType const in_han, AutoHandlerType in_vrt)
    : vrt_dispatch_han_(in_vrt), handler_(in_han)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s);

  bool operator==(CallbackProxySendDirect const& other) const {
    return
      other.handler_ == handler_ &&
      other.vrt_dispatch_han_ == vrt_dispatch_han_;
  }

public:
  template <typename MsgT>
  void trigger(MsgT* msg, PipeType const& pipe);

  void triggerVoid(PipeType const& pipe) {
    vtAssert(0, "Must not be void");
  }

private:
  AutoHandlerType vrt_dispatch_han_ = uninitialized_handler;
  HandlerType handler_ = uninitialized_handler;
};


}}} /* end namespace vt::pipe::callback */

#include "vt/pipe/callback/proxy_send/callback_proxy_send_tl.impl.h"

#endif /*INCLUDED_PIPE_CALLBACK_PROXY_SEND_CALLBACK_PROXY_SEND_TL_H*/
