/*
//@HEADER
// *****************************************************************************
//
//                                   state.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_H

#include "vt/collective/reduce/allreduce/rabenseifner_msg.h"
#include "vt/collective/reduce/allreduce/recursive_doubling_msg.h"

namespace vt::collective::reduce::allreduce {

struct StateBase {
  virtual ~StateBase() = default;
  size_t size_ = {};

  uint32_t local_col_wait_count_ = 0;
  bool finished_adjustment_part_ = false;

  int32_t mask_ = 1;
  int32_t step_ = 0;
  bool initialized_ = false;
  bool completed_ = false;
  bool active_ = false;
};

struct RabensiferBase : StateBase {
  // Scatter
  int32_t scatter_mask_ = 1;
  int32_t scatter_step_ = 0;
  int32_t scatter_num_recv_ = 0;
  std::vector<bool> scatter_steps_recv_ = {};
  std::vector<bool> scatter_steps_reduced_ = {};

  bool finished_scatter_part_ = false;

  // Gather
  int32_t gather_step_ = 0;
  int32_t gather_mask_ = 1;
  int32_t gather_num_recv_ = 0;
  std::vector<bool> gather_steps_recv_ = {};
  std::vector<bool> gather_steps_reduced_ = {};

  std::vector<uint32_t> r_index_ = {};
  std::vector<uint32_t> r_count_ = {};
  std::vector<uint32_t> s_index_ = {};
  std::vector<uint32_t> s_count_ = {};
};

template <typename DataT>
struct RecursiveDoublingState : StateBase {
  DataT val_ = {};
  bool value_assigned_ = false;
  MsgSharedPtr<RecursiveDoublingMsg<DataT>> adjust_message_ = nullptr;

  std::vector<bool> steps_recv_ = {};
  std::vector<bool> steps_reduced_ = {};
  std::vector<MsgSharedPtr<RecursiveDoublingMsg<DataT>>> messages_ = {};
  vt::pipe::callback::cbunion::CallbackTyped<DataT> final_handler_ = {};
};

template <typename Scalar, typename DataT>
struct RabenseifnerState : RabensiferBase {
  ~RabenseifnerState() override {
    left_adjust_message_ = nullptr;
    right_adjust_message_ = nullptr;

    for (auto& msg : scatter_messages_) {
      msg = nullptr;
    }

    for (auto& msg : gather_messages_) {
      msg = nullptr;
    }
  }

  std::vector<Scalar> val_ = {};

  MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>> left_adjust_message_ = nullptr;
  MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>> right_adjust_message_ = nullptr;
  std::vector<MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>>> scatter_messages_ =
    {};
  std::vector<MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>>> gather_messages_ =
    {};

  vt::pipe::callback::cbunion::CallbackTyped<DataT> final_handler_ = {};
};

#if MAGISTRATE_KOKKOS_ENABLED
template <typename Scalar>
struct RabenseifnerState<Scalar, Kokkos::View<Scalar*, Kokkos::HostSpace>>
  : RabensiferBase {
  using DataT = Kokkos::View<Scalar*, Kokkos::HostSpace>;

  ~RabenseifnerState() override {
    left_adjust_message_ = nullptr;
    right_adjust_message_ = nullptr;

    for (auto& msg : scatter_messages_) {
      msg = nullptr;
    }

    for (auto& msg : gather_messages_) {
      msg = nullptr;
    }
  }

  Kokkos::View<Scalar*, Kokkos::HostSpace> val_ = {};

  MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>> left_adjust_message_ = nullptr;
  MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>> right_adjust_message_ = nullptr;
  std::vector<MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>>> scatter_messages_ =
    {};
  std::vector<MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>>> gather_messages_ =
    {};

  vt::pipe::callback::cbunion::CallbackTyped<DataT> final_handler_ = {};
};
#endif //MAGISTRATE_KOKKOS_ENABLED

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_STATE_H*/
