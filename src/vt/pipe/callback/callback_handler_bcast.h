/*
//@HEADER
// *****************************************************************************
//
//                           callback_handler_bcast.h
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

#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_HANDLER_BCAST_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_HANDLER_BCAST_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/active.h"

namespace vt { namespace pipe { namespace callback {

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct CallbackBcast : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType = typename signal::Signal<MsgT>;
  using SignalType     = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType = typename SignalType::DataType;
  using MessageType    = MsgT;

  explicit CallbackBcast(bool const in_include_root = false)
    : include_root_(in_include_root)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | include_root_;
  }

private:
  void trigger_(SignalDataType* data) override {
    theMsg()->broadcastMsg<MsgT,f>(data);
    if (include_root_) {
      auto nmsg = makeMessage<SignalDataType*>(*data);
      f(nmsg.get());
    }
  }

private:
  bool include_root_ = false;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_HANDLER_BCAST_H*/
