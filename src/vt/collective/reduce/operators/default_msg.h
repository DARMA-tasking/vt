/*
//@HEADER
// ************************************************************************
//
//                          default_msg.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
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

#if !defined INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H
#define INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H

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
struct ReduceDataMsg : ReduceMsg, ReduceCombine<void> {
  using CallbackType = CallbackU;

  ReduceDataMsg() = default;
  explicit ReduceDataMsg(DataType&& in_val)
    : ReduceMsg(), ReduceCombine<void>(),
      val_(std::forward<DataType>(in_val))
  { }
  explicit ReduceDataMsg(DataType const& in_val)
    : ReduceMsg(), ReduceCombine<void>(), val_(in_val)
  { }

  DataType const& getConstVal() const { return val_; }
  DataType& getVal() { return val_; }
  DataType&& getMoveVal() { return std::move(val_); }
  CallbackType getCallback() { return cb_; }

  template <typename MsgT>
  void setCallback(Callback<MsgT> cb) { cb_ = CallbackType{cb}; }

  template <typename SerializerT>
  void invokeSerialize(SerializerT& s) {
    ReduceMsg::invokeSerialize(s);
    s | val_;
    s | cb_;
  }

protected:
  DataType val_    = {};
  CallbackType cb_ = {};
};

template <typename T>
struct ReduceTMsg : ReduceDataMsg<T> {
  using DataType = T;
  ReduceTMsg() = default;
  explicit ReduceTMsg(DataType&& in_val)
    : ReduceDataMsg<DataType>(std::forward<DataType>(in_val))
  { }
  explicit ReduceTMsg(DataType const& in_val)
    : ReduceDataMsg<DataType>(in_val)
  { }
};

template <typename T, std::size_t N>
struct ReduceArrMsg : ReduceDataMsg<std::array<T,N>> {
  using DataType = std::array<T,N>;
  ReduceArrMsg() = default;
  explicit ReduceArrMsg(DataType&& in_val)
    : ReduceDataMsg<DataType>(std::forward<DataType>(in_val))
  { }
  explicit ReduceArrMsg(DataType const& in_val)
    : ReduceDataMsg<DataType>(in_val)
  { }
};

template <typename T>
struct ReduceVecMsg : ReduceDataMsg<std::vector<T>> {
  using DataType = std::vector<T>;

  ReduceVecMsg() = default;
  explicit ReduceVecMsg(DataType&& in_val)
    : ReduceDataMsg<DataType>(std::forward<DataType>(in_val))
  { }
  explicit ReduceVecMsg(DataType const& in_val)
    : ReduceDataMsg<DataType>(in_val)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | ReduceDataMsg<DataType>::val_;
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

}} /* end namespace vt::collective */

#endif /*INCLUDED_COLLECTIVE_REDUCE_OPERATORS_DEFAULT_MSG_H*/
