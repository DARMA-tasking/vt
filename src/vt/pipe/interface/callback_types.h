/*
//@HEADER
// *****************************************************************************
//
//                               callback_types.h
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

#if !defined INCLUDED_PIPE_INTERFACE_CALLBACK_TYPES_H
#define INCLUDED_PIPE_INTERFACE_CALLBACK_TYPES_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/interface/callback_direct.h"
#include "vt/pipe/interface/callback_direct_multi.h"
#include "vt/pipe/callback/handler_send/callback_send.h"
#include "vt/pipe/callback/anon/callback_anon.h"
#include "vt/pipe/callback/handler_bcast/callback_bcast.h"
#include "vt/pipe/callback/proxy_bcast/callback_proxy_bcast.h"
#include "vt/pipe/callback/proxy_send/callback_proxy_send.h"
#include "vt/pipe/signal/signal.h"

namespace vt { namespace pipe { namespace interface {

template <typename T>
struct CallbackTypes {
  using V = signal::SigVoidType;
  using CallbackDirectSend      = CallbackDirect<T,callback::CallbackSend<T>>;
  using CallbackDirectBcast     = CallbackDirect<T,callback::CallbackBcast<T>>;
  using CallbackDirectVoidSend  = CallbackDirect<V,callback::CallbackSend<V>>;
  using CallbackDirectVoidBcast = CallbackDirect<V,callback::CallbackBcast<V>>;
  using CallbackAnon            = CallbackDirect<T,callback::CallbackAnon<T>>;
  using CallbackAnonVoid        = CallbackDirect<V,callback::CallbackAnon<V>>;
};

template <typename C, typename T>
struct CallbackVrtTypes {
  using CallbackProxyBcast = CallbackDirect<T,callback::CallbackProxyBcast<C,T>>;
  using CallbackProxySend  = CallbackDirect<T,callback::CallbackProxySend<C,T>>;
};

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_CALLBACK_TYPES_H*/
