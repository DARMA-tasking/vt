/*
//@HEADER
// *****************************************************************************
//
//                           pipe_manager_construct.h
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

#if !defined INCLUDED_PIPE_PIPE_MANAGER_CONSTRUCT_H
#define INCLUDED_PIPE_PIPE_MANAGER_CONSTRUCT_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/state/pipe_state.h"
#include "vt/pipe/interface/remote_container_msg.h"
#include "vt/pipe/interface/send_container.h"
#include "vt/pipe/interface/callback_direct.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/context/context.h"
#include "vt/registry/auto/auto_registry.h"

#include <cassert>
#include <tuple>
#include <type_traits>
#include <array>

namespace vt { namespace pipe {

template <std::size_t N, typename ValueT>
struct RepeatNImpl {
  using ResultType = decltype(
    std::tuple_cat(
      std::declval<std::tuple<ValueT>>(),
      std::declval<typename RepeatNImpl<N-1,ValueT>::ResultType>()
  ));
};

template <typename ValueT>
struct RepeatNImpl<0,ValueT> {
  using ResultType = std::tuple<>;
};

template <
  std::size_t cur, typename T, typename ConsT, typename U, std::size_t N,
  ActiveTypedFnType<U>*... f
>
struct ConstructCallbacksImpl;

template <
  std::size_t cur, typename T, typename ConsT, typename U, std::size_t N,
  ActiveTypedFnType<U>* fcur, ActiveTypedFnType<U>*... f
>
struct ConstructCallbacksImpl<cur,T,ConsT,U,N,fcur,f...> {
  using ResultType = decltype(
    std::tuple_cat(
      std::declval<std::tuple<T>>(),
      std::declval<
        typename ConstructCallbacksImpl<cur+1,T,ConsT,U,N,f...>::ResultType
      >()
  ));
  static ResultType make(ConsT const& ct);
};

template <typename T, typename Tuple, size_t... I>
static T&& build(HandlerType han, Tuple t, std::index_sequence<I...>) {
  return std::move(T{
    han,
    std::forward<typename std::tuple_element<I,Tuple>::type>(
      std::get<I>(t)
    )...
  });
}

template <std::size_t size, typename T, typename... Args>
static T&& build(HandlerType han, std::tuple<Args...> t) {
  return std::move(build<T>(han, t, std::make_index_sequence<size>{}));
}

template <
  std::size_t cur, typename T, typename ConsT, typename U, std::size_t N,
  ActiveTypedFnType<U>* fcur, ActiveTypedFnType<U>*... f
>
/*static*/ typename ConstructCallbacksImpl<cur,T,ConsT,U,N,fcur,f...>::ResultType
ConstructCallbacksImpl<cur,T,ConsT,U,N,fcur,f...>::make(ConsT const& ct)  {
  static constexpr auto size = std::tuple_size<
    typename std::tuple_element<cur,ConsT>::type
  >::value;
  ::fmt::print("val={}\n",std::get<0>(std::get<cur>(ct)));
  return std::tuple_cat(
    std::make_tuple(
      build<size,T>(
        auto_registry::makeAutoHandler<U,fcur>(nullptr), std::get<cur>(ct)
      )
    ),
    ConstructCallbacksImpl<cur+1,T,ConsT,U,N,f...>::make(ct)
  );
}

template <typename T, typename ConsT, typename U, std::size_t N>
struct ConstructCallbacksImpl<N,T,ConsT,U,N> {
  using ResultType = std::tuple<>;
  static ResultType make(ConsT const& ct);
};

template <typename T, typename ConsT, typename U, std::size_t N>
/*static*/ typename ConstructCallbacksImpl<N,T,ConsT,U,N>::ResultType
ConstructCallbacksImpl<N,T,ConsT,U,N>::make(ConsT const& ct) {
  return {};
}

template <typename T, typename ConsT, typename U, ActiveTypedFnType<U>*... f>
struct ConstructCallbacks {
  using Parent = ConstructCallbacksImpl<0,T,ConsT,U,sizeof...(f),f...>;
  using ResultType = typename Parent::ResultType;
  static ResultType make(ConsT const& ct);
};

template <typename T, typename ConsT, typename U, ActiveTypedFnType<U>*... f>
/*static*/ typename ConstructCallbacks<T,ConsT,U,f...>::ResultType
ConstructCallbacks<T,ConsT,U,f...>::make(ConsT const& c) {
  return ConstructCallbacks<T,ConsT,U,f...>::Parent::make(c);
}

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_MANAGER_CONSTRUCT_H*/
