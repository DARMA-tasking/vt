/*
//@HEADER
// *****************************************************************************
//
//                           remote_container.impl.h
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

#if !defined INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_IMPL_H
#define INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/interface/remote_container.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/pipe_manager.fwd.h"
#include "vt/context/context.h"
#include "vt/messaging/message.h"
#include "vt/pipe/signal/signal.h"

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

namespace vt { namespace pipe { namespace interface {

template <typename MsgT, typename TupleT>
template <typename... Args>
RemoteContainer<MsgT,TupleT>::RemoteContainer(
  PipeType const& in_pipe, Args... args
) : RemoteContainer<MsgT,TupleT>(in_pipe,std::make_tuple(args...))
{}

template <typename MsgT, typename TupleT>
template <typename... Args>
RemoteContainer<MsgT,TupleT>::RemoteContainer(
  PipeType const& in_pipe, std::tuple<Args...> tup
) : BaseContainer<MsgT>(in_pipe), trigger_list_(tup)
{}

template <typename MsgT, typename TupleT>
template <typename MsgU, typename CallbackT>
typename RemoteContainer<MsgT,TupleT>::template IsVoidType<MsgU>
RemoteContainer<MsgT,TupleT>::triggerDirect(CallbackT cb, MsgU*) {
  auto const& pid = BaseContainer<MsgT>::getPipe();
  constexpr auto multi_callback = std::tuple_size<decltype(trigger_list_)>();
  debug_print(
    pipe, node,
    "RemoteContainer: (void) invoke trigger: pipe={:x}, multi={}\n",
    pid, multi_callback
  );
  cb.trigger(nullptr,pid);
}

template <typename MsgT, typename TupleT>
template <typename MsgU, typename CallbackT>
typename RemoteContainer<MsgT,TupleT>::template IsNotVoidType<MsgU>
RemoteContainer<MsgT,TupleT>::triggerDirect(CallbackT cb, MsgU* data) {
  auto const& pid = BaseContainer<MsgT>::getPipe();
  auto const& multi_callback = std::tuple_size<decltype(trigger_list_)>() > 0;
  debug_print(
    pipe, node,
    "RemoteContainer: (typed) invoke trigger: pipe={:x}, multi={}, ptr={}\n",
    pid, multi_callback, print_ptr(data)
  );
  MsgT* cur_msg = data;

  /*
   *  Make a copy of the message for each callback trigger because the callback
   *  trigger may send/bcast messages, thus a copy must be made
   */
  if (multi_callback) {
    cur_msg = makeMessage<MsgT>(*data);
  }

  cb.trigger(cur_msg.get(),pid);
}

template <typename MsgT, typename TupleT>
template <typename MsgU>
void RemoteContainer<MsgT,TupleT>::trigger(MsgU* data) {
  auto const& pipe = BaseContainer<MsgT>::getPipe();
  bool const& is_send_back = isSendBack();
  debug_print(
    pipe, node,
    "RemoteContainer: pipe={:x}, send_back={}, size={}\n",
    pipe, is_send_back, std::tuple_size<decltype(trigger_list_)>()
  );
  if (is_send_back) {
    auto const& retpipe = this->getPipe();
    triggerSendBack<MsgT>(retpipe,data);
  } else {
    triggerDirect(data);
  }
}

template <typename MsgT, typename TupleT>
bool RemoteContainer<MsgT,TupleT>::isSendBack() const {
  auto const& pipe = this->getPipe();
  return PipeIDBuilder::isSendback(pipe);
}

template <typename MsgT, typename TupleT>
template <typename... Ts>
void RemoteContainer<MsgT,TupleT>::foreach(
  std::tuple<Ts...> const& t, MsgT* data
) {
  return foreach(t, std::index_sequence_for<Ts...>{}, data);
}

template <typename MsgT, typename TupleT>
template <typename... Ts, std::size_t... Idx>
void RemoteContainer<MsgT,TupleT>::foreach(
  std::tuple<Ts...> const& tup, std::index_sequence<Idx...>, MsgT* data
) {
  auto _={(triggerDirect(std::get<Idx>(tup),data),0)...};
}

template <typename MsgT, typename TupleT>
void RemoteContainer<MsgT,TupleT>::triggerDirect(MsgT* data) {
  return foreach(trigger_list_, data);
}

template <typename MsgT, typename TupleT>
template <typename SerializerT>
void RemoteContainer<MsgT,TupleT>::serialize(SerializerT& s) {
  BaseContainer<MsgT>::serializer(s);
  s | trigger_list_;
}

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_IMPL_H*/
