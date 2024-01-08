/*
//@HEADER
// *****************************************************************************
//
//                                default_msg.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H

#include "vt/config.h"
#include "vt/collective/reduce/operators/default_op.h"
#include "vt/collective/reduce/reduce_msg.h"
#include "vt/pipe/pipe_callback_only.h"

#include <array>
#include <vector>
#include <variant>

namespace vt { namespace collective { namespace reduce { namespace operators {

template <typename>
struct ReduceCombine;

template <typename T, typename enabled = void>
struct GetCallbackType;

template <typename T>
struct GetCallbackType<T> {
  using CallbackType = Callback<T>;
  using MsgT = T;
};

template <typename... Args>
struct GetCallbackType<std::tuple<Args...>> {
  using CallbackType = Callback<Args...>;
};

template <typename T>
struct GetCallbackType<Callback<T>> {
  using MsgT = T;
};

template <typename DataType>
struct ReduceDataMsg : SerializeIfNeeded<
  ReduceMsg,
  ReduceDataMsg<DataType>,
  DataType
>, ReduceCombine<void> {

  using DataT = DataType;
  using CallbackParamType = typename GetCallbackType<DataType>::CallbackType;
  using CallbackMsgType = Callback<ReduceDataMsg<DataType>>;

  using MessageParentType = SerializeIfNeeded<
    ReduceMsg,
    ReduceDataMsg<DataType>,
    DataType
  >;

  ReduceDataMsg() = default;
  ReduceDataMsg(ReduceDataMsg const&) = default;
  ReduceDataMsg(ReduceDataMsg&&) = default;

  explicit ReduceDataMsg(DataType&& in_val)
    : MessageParentType(), ReduceCombine<void>(),
      val_(std::forward<DataType>(in_val))
  { }
  explicit ReduceDataMsg(DataType const& in_val)
    : MessageParentType(), ReduceCombine<void>(), val_(in_val)
  { }

  DataType& getTuple() { return val_; }
  DataType const& getConstVal() const { return val_; }
  DataType& getVal() { return val_; }
  DataType&& getMoveVal() { return std::move(val_); }

  bool isMsgCallback() const { return cb_.index() == 0; }
  bool isParamCallback() const { return cb_.index() == 1; }
  CallbackMsgType getMsgCallback() { return std::get<0>(cb_); }
  CallbackParamType getParamCallback() { return std::get<1>(cb_); }
  bool hasValidCallback() {
    if (isMsgCallback()) {
      return getMsgCallback().valid();
    } else {
      return getParamCallback().valid();
    }
  }

  template <typename CallbackT>
  void setCallback(CallbackT cb) {
    if constexpr (std::is_same_v<CallbackT, CallbackParamType>) {
      cb_ = cb;
    } else if (
      std::is_convertible_v<
        typename GetCallbackType<CallbackT>::MsgT*, ReduceDataMsg<DataType>*
      >
    ) {
      auto cb_ptr = reinterpret_cast<Callback<ReduceDataMsg<DataType>>*>(&cb);
      cb_ = *cb_ptr;
    } else {
      static_assert(
        std::is_same_v<CallbackT, CallbackParamType> or
        std::is_convertible_v<
          typename GetCallbackType<CallbackT>::MsgT*, ReduceDataMsg<DataType>*
        >,
        "Must be a convertible message callback or parameterized callback"
      );
    }
  }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    MessageParentType::serialize(s);
    s | val_;
    int index = cb_.index();
    s | index;
    if (s.isUnpacking()) {
      if (index == 0) {
        CallbackMsgType cb;
        s | cb;
        cb_ = cb;
      } else {
        CallbackParamType cb;
        s | cb;
        cb_ = cb;
      }
    } else {
      if (index == 0) {
        s | std::get<0>(cb_);
      } else {
        s | std::get<1>(cb_);
      }
    }
  }

protected:
  DataType val_    = {};
  std::variant<CallbackMsgType, CallbackParamType> cb_;
};

template <typename T>
using ReduceTMsg = ReduceDataMsg<T>;

template <typename T, std::size_t N>
struct ReduceArrMsg : SerializeIfNeeded<
  ReduceDataMsg<std::array<T, N>>,
  ReduceArrMsg<T, N>
> {
  using MessageParentType = SerializeIfNeeded<
    ReduceDataMsg<std::array<T, N>>,
    ReduceArrMsg<T, N>
  >;
  using DataType = std::array<T, N>;

  ReduceArrMsg() = default;
  explicit ReduceArrMsg(DataType&& in_val)
    : MessageParentType(std::forward<DataType>(in_val))
  { }
  explicit ReduceArrMsg(DataType const& in_val)
    : MessageParentType(in_val)
  { }

  template <typename SerializeT>
  inline void serialize(SerializeT& s) {
    MessageParentType::serialize(s);
  }
};

template <typename T>
struct ReduceVecMsg : SerializeRequired<
  ReduceDataMsg<std::vector<T>>,
  ReduceVecMsg<T>
> {
  using MessageParentType = SerializeRequired<
    ReduceDataMsg<std::vector<T>>,
    ReduceVecMsg<T>
  >;
  using DataType = std::vector<T>;

  ReduceVecMsg() = default;
  explicit ReduceVecMsg(DataType&& in_val)
    : MessageParentType(std::forward<DataType>(in_val))
  { }
  explicit ReduceVecMsg(DataType const& in_val)
    : MessageParentType(in_val)
  { }

  template <typename SerializerT>
  inline void serialize(SerializerT& s) {
    MessageParentType::serialize(s);
  }
};

}}}} /* end namespace vt::collective::reduce::operators */

namespace vt { namespace collective {

template <typename T>
using ReduceVecMsg = reduce::operators::ReduceVecMsg<T>;

template <typename T, std::size_t N>
using ReduceArrMsg = reduce::operators::ReduceArrMsg<T,N>;

template <typename T>
using ReduceTMsg = reduce::operators::ReduceTMsg<T>;

using ReduceNoneMsg = reduce::operators::ReduceTMsg<NoneType>;

}} /* end namespace vt::collective */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H*/
