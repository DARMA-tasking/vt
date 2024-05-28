/*
//@HEADER
// *****************************************************************************
//
//                                 param_msg.h
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

#if !defined INCLUDED_VT_MESSAGING_PARAM_MSG_H
#define INCLUDED_VT_MESSAGING_PARAM_MSG_H

#include "vt/messaging/message/message_serialize.h"

namespace vt {

struct MsgProps {

  MsgProps() = default;

  MsgProps&& asLocationMsg(bool set = true) {
    as_location_msg_ = set;
    return std::move(*this);
  }

  MsgProps&& asTerminationMsg(bool set = true) {
    as_termination_msg_ = set;
    return std::move(*this);
  }

  MsgProps&& asCollectionMsg(bool set = true) {
    as_collection_msg_ = set;
    return std::move(*this);
  }

  MsgProps&& asSerializationMsg(bool set = true) {
    as_serial_msg_ = set;
    return std::move(*this);
  }

  MsgProps&& withEpoch(EpochType in_ep) {
    ep_ = in_ep;
    return std::move(*this);
  }

  MsgProps&& withPriority([[maybe_unused]] PriorityType in_priority) {
#if vt_check_enabled(priorities)
    priority_ = in_priority;
#endif
    return std::move(*this);
  }

  MsgProps&& withPriorityLevel(
    [[maybe_unused]] PriorityLevelType in_priority_level
  ) {
#if vt_check_enabled(priorities)
    priority_level_ = in_priority_level;
#endif
    return std::move(*this);
  }

  template <typename MsgPtrT>
  void apply(MsgPtrT msg);

private:
  bool as_location_msg_ = false;
  bool as_termination_msg_ = false;
  bool as_serial_msg_ = false;
  bool as_collection_msg_ = false;
  EpochType ep_ = no_epoch;
#if vt_check_enabled(priorities)
  PriorityType priority_ = no_priority;
  PriorityLevelType priority_level_ = no_priority_level;
#endif
};

} /* end namespace vt */

namespace vt::messaging::detail {

template <typename enabled_, typename... Params>
struct GetTraits;

template <>
struct GetTraits<std::enable_if_t<std::is_same_v<void, void>>> {
  using TupleType = std::tuple<>;
};

template <typename Param, typename... Params>
struct GetTraits<
  std::enable_if_t<std::is_same_v<MsgProps, Param>>, Param, Params...
>  {
  using TupleType = std::tuple<Params...>;
};

template <typename Param, typename... Params>
struct GetTraits<
  std::enable_if_t<not std::is_same_v<MsgProps, Param>>, Param, Params...
>  {
  using TupleType = std::tuple<Param, Params...>;
};

template <typename Tuple>
struct GetTraitsTuple;

template <typename... Params>
struct GetTraitsTuple<std::tuple<Params...>> {
  using TupleType = typename GetTraits<void, Params...>::TupleType;
};

} /* end namespace vt::messaging::detail */

namespace vt::messaging {

template <typename Tuple, typename enabled = void>
struct ParamMsg;

template <typename Tuple>
struct ParamMsg<
  Tuple, std::enable_if_t<is_byte_copyable_t<Tuple>::value>
> : vt::Message
{
  using TupleType = typename detail::GetTraitsTuple<Tuple>::TupleType;

  ParamMsg() = default;

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

template <typename Tuple>
struct ParamMsg<
  Tuple, std::enable_if_t<not is_byte_copyable_t<Tuple>::value>
> : vt::Message
{
  using MessageParentType = vt::Message;
  vt_msg_serialize_if_needed_by_parent_or_type1(Tuple); // by tup

  using TupleType = typename detail::GetTraitsTuple<Tuple>::TupleType;

  ParamMsg() = default;

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

  std::unique_ptr<TupleType> params;

  TupleType& getTuple() { return *params.get(); }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
    s | params;
  }
};

} /* end namespace vt::messaging */

#endif /*INCLUDED_VT_MESSAGING_PARAM_MSG_H*/
