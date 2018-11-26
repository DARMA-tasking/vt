/*
//@HEADER
// ************************************************************************
//
//                          pipe.h
//                                VT
//              Copyright (C) 2017 NTESS, LLC
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

#if !defined INCLUDED_PIPE_PIPE_H
#define INCLUDED_PIPE_PIPE_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"

#include <functional>
#include <cassert>
#include <unordered_map>
#include <vector>

namespace vt { namespace pipe {




// template <typename MsgT, ActiveTypedFnType<MsgT>*... Args>
// ActiveTypedFnType<MsgT>*
// HandlerHolder<MsgT, Args...>::data[sizeof...(Args)] = { Args... };

// template <
//   typename MsgT, size_t N, template <std::size_t> class F,
//   ActiveTypedFnType<MsgT>*... Args
// >
// struct GenerateArrayImpl {
//   using ResultType =
//     typename GenerateArrayImpl<MsgT, N-1, F, F<N>::value, Args...>::ResultType;
// };

// template <
//   typename MsgT, template <std::size_t> class F,
//   ActiveTypedFnType<MsgT>*... Args
// >
// struct GenerateArrayImpl<MsgT, 0, F, Args...> {
//   using ResultType = HandlerHolder<MsgT, F<0>::value, Args...>;
// };

// template <typename MsgT, size_t N, template <std::size_t> class F>
// struct GenerateArray {
//   using ResultType = typename GenerateArrayImpl<MsgT, N-1, F>::ResultType;
// };

// template <typename MsgT, ActiveTypedFnType<MsgT>*... fpack>
// struct HandlerPack {
//   template <std::size_t elm>
//   HandlerType getHandlerElm() {
//     auto const& handler =
//       auto_registry::makeAutoHandler<MsgT,std::get<elm>(tpack)>(nullptr);
//   };
// };


using CallbackIDType = uint64_t;

// template <typename T, size_t N>
// struct GenerateTypePackImpl {
//   using ResultType = decltype(
//     std::tuple_cat(
//       std::declval<std::tuple<T>>(),
//       std::declval<typename GenerateTypePackImpl<T,N-1>::ResultType>()
//   ));
// };

// template <typename T>
// struct GenerateTypePackImpl<T,0> {
//   using ResultType = std::tuple<>;
// };

// template <typename T, size_t N>
// struct GenerateTypePack {
//   using ResultType = typename GenerateTypePackImpl<T,N>::ResultType;
// };



// template <typename T, ActiveTypedFnType<T>*... Args>
// struct HandlerRegHolder {
//   static constexpr HandlerType handlers[sizeof...(Args)] = {
//     auto_registry::makeAutoHandlerIdx<T,Args>()...
//   };
// };


// template <typename T, size_t N, ActiveTypedFnType<T>*... f>
// struct HandlePackRegImpl {
//   using ResultType = decltype(
//     std::tuple_cat(
//       std::declval<std::tuple<T>>(),
//       std::declval<typename HandlePackRegImpl<T,N-1>::ResultType>()
//   ));
// };


// template <std::size_t cur, typename T, std::size_t N, HandlerType handlers[N]>
// struct ConstructCallbacksImpl {
//   using ResultType = decltype(
//     std::tuple_cat(
//       std::declval<std::tuple<T>>(),
//       std::declval<typename GenerateTypePackImpl<T,N-1>::ResultType>()
//   ));
//   static constexpr ResultType tuple = std::tuple_cat(
//     T{0,handlers[cur]},
//     GenerateTypePackImpl<T,N-1>::tuple
//   );
// };

// template <typename T, std::size_t N, HandlerType handlers[N]>
// struct ConstructCallbacksImpl<0,T,N,handlers> {
//   using ResultType = std::tuple<>;
//   static constexpr ResultType tuple = ResultType{};
// };

// template <typename T, std::size_t N, HandlerType handlers[N]>
// struct ConstructCallbacks {
//   using Parent = ConstructCallbacksImpl<N-1,T,N,handlers[N]>;
//   using ResultType = typename Parent::ResultType;
//   static constexpr ResultType tuple = Parent::tuple;
// };



}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_PIPE_H*/
