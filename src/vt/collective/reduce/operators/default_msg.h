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

namespace vt { namespace collective { namespace reduce { namespace operators {

template <typename>
struct ReduceCombine;

template <typename DataType>
struct ReduceDataMsg : SerializeIfNeeded<
  ReduceMsg,
  ReduceDataMsg<DataType>,
  DataType
>, ReduceCombine<void> {
  using MessageParentType = SerializeIfNeeded<
    ReduceMsg,
    ReduceDataMsg<DataType>,
    DataType
  >;

  using CallbackType = CallbackU;

  ReduceDataMsg() = default;
  explicit ReduceDataMsg(DataType&& in_val)
    : MessageParentType(), ReduceCombine<void>(),
      val_(std::forward<DataType>(in_val))
  { }
  explicit ReduceDataMsg(DataType const& in_val)
    : MessageParentType(), ReduceCombine<void>(), val_(in_val)
  { }

  DataType const& getConstVal() const { return val_; }
  DataType& getVal() { return val_; }
  DataType&& getMoveVal() { return std::move(val_); }
  CallbackType getCallback() { return cb_; }

  template <typename MsgT>
  void setCallback(Callback<MsgT> cb) { cb_ = CallbackType{cb}; }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    MessageParentType::serialize(s);
    s | val_;
    s | cb_;
  }

protected:
  DataType val_    = {};
  CallbackType cb_ = {};
};

template <typename T>
struct ReduceTMsg : SerializeIfNeeded<
  ReduceDataMsg<T>,
  ReduceTMsg<T>
> {
  using MessageParentType = SerializeIfNeeded<
    ReduceDataMsg<T>,
    ReduceTMsg<T>
  >;

  ReduceTMsg() = default;
  explicit ReduceTMsg(T&& in_val)
    : MessageParentType(std::forward<T>(in_val))
  { }
  explicit ReduceTMsg(T const& in_val)
    : MessageParentType(in_val)
  { }

  template <typename SerializeT>
  inline void serialize(SerializeT& s) {
    MessageParentType::serialize(s);
  }
};

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
