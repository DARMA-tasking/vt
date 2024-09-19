/*
//@HEADER
// *****************************************************************************
//
//                               param_col_msg.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_VRT_COLLECTION_MESSAGES_PARAM_COL_MSG_H
#define INCLUDED_VT_VRT_COLLECTION_MESSAGES_PARAM_COL_MSG_H

#include "vt/vrt/collection/messages/user.h"
#include "vt/messaging/message/message_serialize.h"
#include "vt/messaging/param_msg.h"

namespace vt { namespace vrt { namespace collection {

template <typename Tuple, typename ColT, typename enabled = void>
struct ParamColMsg;

template <typename Tuple, typename ColT>
struct ParamColMsg<
  Tuple,
  ColT,
  std::enable_if_t<messaging::is_byte_copyable_t<Tuple>::value>
> : CollectionMessage<ColT, vt::Message> {

  using TupleType = typename messaging::detail::GetTraitsTuple<Tuple>::TupleType;

  ParamColMsg() = default;

  void setParams() { }

  template <typename Param, typename... Params>
  void setParams(Param&& p, Params&&... in_params) {
    if constexpr (std::is_same_v<std::decay_t<Param>, MsgProps>) {
      params = TupleType{std::forward<Params>(in_params)...};
      p.apply(this);
    } else {
      params = TupleType{std::forward<Param>(p), std::forward<Params>(in_params)...};
    }
  }

  TupleType params;
  TupleType& getTuple() { return params; }
};

template <typename Tuple, typename ColT>
struct ParamColMsg<
  Tuple,
  ColT,
  std::enable_if_t<not messaging::is_byte_copyable_t<Tuple>::value>
> : CollectionMessage<ColT, vt::Message> {
  using MessageParentType = CollectionMessage<ColT, vt::Message>;
  vt_msg_serialize_if_needed_by_parent_or_type1(Tuple); // by params

  using TupleType = typename messaging::detail::GetTraitsTuple<Tuple>::TupleType;

  ParamColMsg() = default;

  void setParams() {
    params = std::make_unique<TupleType>();
  }

  template <typename Param, typename... Params>
  void setParams(Param&& p, Params&&... in_params) {
    if constexpr (std::is_same_v<std::decay_t<Param>, MsgProps>) {
      params = std::make_unique<TupleType>(std::forward<Params>(in_params)...);
      p.apply(this);
    } else {
      params = std::make_unique<TupleType>(
        std::forward<Param>(p), std::forward<Params>(in_params)...
      );
    }
  }

  std::unique_ptr<Tuple> params;

  TupleType& getTuple() { return *params.get(); }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | params;
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_MESSAGES_PARAM_COL_MSG_H*/
