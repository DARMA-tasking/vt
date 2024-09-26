/*
//@HEADER
// *****************************************************************************
//
//                              rabenseifner_msg.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_MSG_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_MSG_H

#include "vt/config.h"
#include "vt/messaging/active.h"
#include "vt/configs/debug/debug_print.h"
#include "vt/collective/reduce/operators/default_msg.h"

namespace vt::collective::reduce::allreduce {

template <typename Scalar, typename DataT>
struct RabenseifnerMsg : Message {
  using MessageParentType = vt::Message;
  vt_msg_serialize_required();

  RabenseifnerMsg() = default;
  RabenseifnerMsg(RabenseifnerMsg const&) = default;
  RabenseifnerMsg(RabenseifnerMsg&&) = default;
  ~RabenseifnerMsg() {
    if (owning_) {
      delete[] val_;
    }
  }

  RabenseifnerMsg(const Scalar* in_val, size_t size, size_t id, int step = 0)
    : MessageParentType(),
      val_(in_val),
      size_(size),
      id_(id),
      step_(step) { }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    MessageParentType::serialize(s);

    s | size_;

    if (s.isUnpacking()) {
      owning_ = true;
      val_ = new Scalar[size_];
    }

    checkpoint::dispatch::serializeArray(s, val_, size_);

    s | id_;
    s | step_;
  }

  const Scalar* val_ = {};
  size_t size_ = {};
  size_t id_ = {};
  int32_t step_ = {};
  bool owning_ = false;
};

#if MAGISTRATE_KOKKOS_ENABLED
template <typename Scalar, typename... Properties>
struct RabenseifnerMsg<Scalar, Kokkos::View<Scalar*, Properties...>> : Message {
  using ViewT = Kokkos::View<Scalar*, Properties...>;
  using MessageParentType = vt::Message;
  vt_msg_serialize_required();

  RabenseifnerMsg() = default;
  RabenseifnerMsg(RabenseifnerMsg const&) = default;
  RabenseifnerMsg(RabenseifnerMsg&&) = default;

  RabenseifnerMsg(const ViewT& in_val, size_t id, int step = 0)
    : MessageParentType(),
      val_(in_val),
      id_(id),
      step_(step) { }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    MessageParentType::serialize(s);

    s | val_;
    s | id_;
    s | step_;
  }

  ViewT val_ = {};
  size_t id_ = {};
  int32_t step_ = {};
};
#endif // MAGISTRATE_KOKKOS_ENABLED

} // namespace vt::collective::reduce::allreduce
#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_MSG_H*/
