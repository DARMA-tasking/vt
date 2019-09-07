/*
//@HEADER
// *****************************************************************************
//
//                            remote_container_msg.h
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

#if !defined INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_H
#define INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/interface/base_container.h"

#include <tuple>
#include <utility>
#include <type_traits>

namespace vt { namespace pipe { namespace interface {

template <typename MsgT, typename TupleT>
struct RemoteContainerMsg : BaseContainer<MsgT> {
  using VoidSigType   = signal::SigVoidType;
  template <typename T, typename U=void>
  using IsVoidType    = std::enable_if_t<std::is_same<T,VoidSigType>::value,U>;
  template <typename T, typename U=void>
  using IsNotVoidType = std::enable_if_t<!std::is_same<T,VoidSigType>::value,U>;

  template <typename... Args>
  explicit RemoteContainerMsg(PipeType const& in_pipe, Args... args);

  template <typename... Args>
  RemoteContainerMsg(PipeType const& in_pipe, std::tuple<Args...> tup);

private:
  template <typename MsgU, typename CallbackT>
  IsNotVoidType<MsgU> triggerDirect(CallbackT cb, MsgU* data);

  template <typename MsgU, typename CallbackT>
  IsVoidType<MsgU> triggerDirect(CallbackT cb, MsgU* data);

  void triggerDirect(MsgT* data);

  template <typename... Ts>
  void foreach(std::tuple<Ts...> const& t, MsgT* data);

  template <typename... Ts, std::size_t... Idx>
  void foreach(
    std::tuple<Ts...> const& tup, std::index_sequence<Idx...>, MsgT* data
  );

  bool isSendBack() const;

public:
  template <typename MsgU>
  void trigger(MsgU* data);

  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  TupleT trigger_list_;
};

}}} /* end namespace vt::pipe::interface */

#include "vt/pipe/interface/remote_container_msg.impl.h"

#endif /*INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_H*/
