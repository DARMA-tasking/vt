/*
//@HEADER
// *****************************************************************************
//
//                           callback_direct_multi.h
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

#if !defined INCLUDED_VT_PIPE_INTERFACE_CALLBACK_DIRECT_MULTI_H
#define INCLUDED_VT_PIPE_INTERFACE_CALLBACK_DIRECT_MULTI_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/interface/base_container.h"
#include "vt/pipe/interface/remote_container.h"

#include <tuple>
#include <utility>
#include <type_traits>

namespace vt { namespace pipe { namespace interface {

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
static struct CallbackDirectSendMultiTagType {} CallbackDirectSendMultiTag { };
#pragma GCC diagnostic pop

template <typename MsgT, typename TupleT>
struct CallbackDirectSendMulti : RemoteContainer<MsgT,TupleT> {

  template <typename... Args>
  explicit CallbackDirectSendMulti(PipeType const& in_pipe, Args... args)
    : RemoteContainer<MsgT,TupleT>(in_pipe,std::make_tuple(args...))
  { }

  template <typename... Args>
  CallbackDirectSendMulti(
    CallbackDirectSendMultiTagType, PipeType const& in_pipe,
    std::tuple<Args...> tup
  ) : RemoteContainer<MsgT,TupleT>(in_pipe,tup)
  { }

  void send(MsgT* m) {
    ::fmt::print("callback send\n");
    RemoteContainer<MsgT,TupleT>::trigger(m);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    RemoteContainer<MsgT,TupleT>::serialize(s);
  }
};

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_VT_PIPE_INTERFACE_CALLBACK_DIRECT_MULTI_H*/
