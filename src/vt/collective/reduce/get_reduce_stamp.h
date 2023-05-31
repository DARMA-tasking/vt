/*
//@HEADER
// *****************************************************************************
//
//                              get_reduce_stamp.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_GET_REDUCE_STAMP_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_GET_REDUCE_STAMP_H

#include "vt/config.h"
#include "vt/collective/reduce/reduce_scope.h"
#include "vt/messaging/message.h"

#include <tuple>
#include <utility>
#include <type_traits>

namespace vt { namespace collective { namespace reduce {

template <typename enable = void, typename... Args>
struct GetReduceStamp : std::false_type {
  template <typename MsgT>
  static auto getStampMsg(Args&&... args) {
    return
      std::make_tuple(
	collective::reduce::ReduceStamp{},
	vt::makeMessage<MsgT>(std::tuple{std::forward<Args>(args)...})
      );
  }
};

template <>
struct GetReduceStamp<
  std::enable_if_t<std::is_same_v<void, void>>
> : std::false_type {
  template <typename MsgT>
  static auto getStampMsg() {
    return std::make_tuple(
      collective::reduce::ReduceStamp{},
      vt::makeMessage<MsgT>(std::tuple<>{})
    );
  }
};

template <typename... Args>
struct GetReduceStamp<
  std::enable_if_t<
    std::is_same_v<
      std::decay_t<std::tuple_element_t<sizeof...(Args) - 1, std::tuple<Args...>>>,
      collective::reduce::ReduceStamp
    >
  >,
  Args...
> : std::true_type {
  template <typename... Params, std::size_t... Is>
  static constexpr auto getMsgHelper(
    std::tuple<Params...> tp, std::index_sequence<Is...>
  ) {
    return std::tuple{std::get<Is>(tp)...};
  }

  template <typename MsgT>
  static auto getStampMsg(Args&&... args) {
    auto tp = std::make_tuple(std::forward<Args>(args)...);
    return
      std::make_tuple(
        std::get<sizeof...(Args) - 1>(tp),
	vt::makeMessage<MsgT>(
          getMsgHelper(
            std::move(tp), std::make_index_sequence<sizeof...(Args) - 1>{}
          )
	)
      );
  }
};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_GET_REDUCE_STAMP_H*/
