/*
//@HEADER
// *****************************************************************************
//
//                            send_container.impl.h
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

#if !defined INCLUDED_VT_PIPE_INTERFACE_SEND_CONTAINER_IMPL_H
#define INCLUDED_VT_PIPE_INTERFACE_SEND_CONTAINER_IMPL_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/interface/send_container.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/pipe_manager.fwd.h"

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

namespace vt { namespace pipe { namespace interface {

template <typename DataT, typename... Args>
SendContainer<DataT,Args...>::SendContainer(
  PipeType const& in_pipe, Args&&... args
) : BaseContainer<DataT>(in_pipe),
    trigger_list_(std::make_tuple(std::forward<Args...>(args...)))
{}

template <typename DataT, typename... Args>
template <typename CallbackT>
void SendContainer<DataT,Args...>::triggerDirect(CallbackT cb, DataT data) {
  cb.trigger(data);
}

template <typename DataT, typename... Args>
void SendContainer<DataT,Args...>::trigger(DataT data) {
  bool const& is_send_back = isSendBack();
  if (is_send_back) {
    auto const& pipe = this->getPipe();
    triggerSendBack<DataT>(pipe,data);
  } else {
    triggerDirect(data);
  }
}

template <typename DataT, typename... Args>
bool SendContainer<DataT,Args...>::isSendBack() const {
  auto const& pipe = this->getPipe();
  return PipeIDBuilder::isSendback(pipe);
}

template <typename DataT, typename... Args>
template <typename... Ts>
void SendContainer<DataT,Args...>::foreach(
  std::tuple<Ts...> const& t, DataT data
) {
  return foreach(t, std::index_sequence_for<Ts...>{}, data);
}

template <typename DataT, typename... Args>
template <typename... Ts, std::size_t... Idx>
void SendContainer<DataT,Args...>::foreach(
  std::tuple<Ts...> const& tup, std::index_sequence<Idx...>, DataT data
) {
  auto _={(triggerDirect(std::get<Idx>(tup),data),0)...};
}

template <typename DataT, typename... Args>
void SendContainer<DataT,Args...>::triggerDirect(DataT data) {
  return foreach(trigger_list_, data);
}

template <typename DataT, typename... Args>
template <typename SerializerT>
void SendContainer<DataT,Args...>::serialize(SerializerT& s) {
  BaseContainer<DataT>::serializer(s);
  s | trigger_list_;
}

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_VT_PIPE_INTERFACE_SEND_CONTAINER_IMPL_H*/
