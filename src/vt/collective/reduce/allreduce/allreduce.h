/*
//@HEADER
// *****************************************************************************
//
//                                   reduce.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_ALLREDUCE_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_ALLREDUCE_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/messaging/message/message.h"

#include <tuple>
#include <cstdint>

namespace vt::collective::reduce::alleduce {

template <typename DataT>
struct AllreduceMsg
  : SerializeIfNeeded<vt::Message, AllreduceMsg<DataT>, DataT> {
  using MessageParentType =
    SerializeIfNeeded<::vt::Message, AllreduceMsg<DataT>, DataT>;

  AllreduceMsg() = default;
  AllreduceMsg(AllreduceMsg const&) = default;
  AllreduceMsg(AllreduceMsg&&) = default;

  explicit AllreduceMsg(DataT&& in_val)
    : MessageParentType(),
      val_(std::forward<DataT>(in_val)) { }
  explicit AllreduceMsg(DataT const& in_val)
    : MessageParentType(),
      val_(in_val) { }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    MessageParentType::serialize(s);
    s | val_;
  }

  DataT val_ = {};
};

template <typename DataT>
struct Allreduce {
  void rightHalf(AllreduceMsg<DataT>* msg) {
    for (int i = 0; i < msg->vec_.size(); i++) {
      val_[(val_.size() / 2) + i] += msg->vec_[i];
    }
  }

  void rightHalfComplete(AllreduceMsg<DataT>* msg) {
    for (int i = 0; i < msg->vec_.size(); i++) {
      val_[(val_.size() / 2) + i] = msg->vec_[i];
    }
  }

  void leftHalf(AllreduceMsg<DataT>* msg) {
    for (int i = 0; i < msg->vec_.size(); i++) {
      val_[i] += msg->vec_[i];
    }
  }

  void leftHalfComplete(AllreduceMsg<DataT>* msg) {
    for (int i = 0; i < msg->vec_.size(); i++) {
      val_[i] = msg->vec_[i];
    }
  }

  void sendHandler(AllreduceMsg<DataT>* msg) {
    uint32_t start = is_even_ ? 0 : val_.size() / 2;
    uint32_t end = is_even_ ? val_.size() / 2 : val_.size();
    for (int i = 0; start < end; start++) {
      val_[start] += msg->vec_[i++];
    }
  }

  void reducedHan(AllreduceMsg<DataT>* msg) {
    for (int i = 0; i < msg->vec_.size(); i++) {
      val_[val_.size() / 2 + i] = msg->vec_[i];
    }
  }

  Allreduce() { is_even_ = theContext()->getNode() % 2 == 0; }

  bool is_even_ = false;
  DataT val_ = {};
};

} // namespace vt::collective::reduce::alleduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_H*/
