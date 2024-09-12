/*
//@HEADER
// *****************************************************************************
//
//                                  helpers.h
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

#include "vt/configs/debug/debug_printconst.h"
#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_HELPERS_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_HELPERS_H

#include "data_handler.h"
#include "rabenseifner_msg.h"
#include "vt/messaging/message/shared_message.h"

#include <vector>
#include <type_traits>

namespace vt {
template <typename T>
using remove_cvref = std::remove_cv_t<std::remove_reference_t<T>>;
}

namespace vt::collective::reduce::allreduce {

template <typename T>
struct function_traits;  // General template declaration.

// Specialization for function pointers.
template <typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> {
    using return_type = Ret;
    static constexpr std::size_t arity = sizeof...(Args);
    using args_tuple = std::tuple<Args...>;

    template <std::size_t N>
    using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

template <typename Ret, typename ObjT, typename... Args>
struct function_traits<Ret(ObjT::*)(Args...)> {
    using return_type = Ret;
    static constexpr std::size_t arity = sizeof...(Args);
    using args_tuple = std::tuple<Args...>;

    template <std::size_t N>
    using arg_type = typename std::tuple_element<N, std::tuple<Args...>>::type;
};

// Primary template
template <typename Scalar, typename DataT>
struct ShouldUseView {
  static constexpr bool Value = false;
};

#if MAGISTRATE_KOKKOS_ENABLED
// Partial specialization for Kokkos::View
template <typename Scalar>
struct ShouldUseView<Scalar, Kokkos::View<Scalar*, Kokkos::HostSpace>> {
  static constexpr bool Value = true;
};
#endif // MAGISTRATE_KOKKOS_ENABLED

// Helper alias for cleaner usage
template <typename Scalar, typename DataT>
inline constexpr bool ShouldUseView_v = ShouldUseView<Scalar, DataT>::Value;

template <typename Scalar, typename DataT>
struct DataHelper {
  using DataHan = DataHandler<DataT>;

  template <typename... Args>
  static void assign(std::vector<Scalar>& dest, Args&&... data) {
    dest = DataHan::toVec(std::forward<Args>(data)...);
  }

  static auto createMessage(
    const std::vector<Scalar>& payload, size_t begin, size_t count, size_t id,
    int32_t step = 0) {
    return vt::makeMessage<RabenseifnerMsg<Scalar, DataT>>(
      payload.data() + begin, count, id, step);
  }

  static void copy(
    std::vector<Scalar>& dest, size_t start_idx, RabenseifnerMsg<Scalar, DataT>* msg) {
    for (uint32_t i = 0; i < msg->size_; i++) {
      dest[start_idx + i] = msg->val_[i];
    }
  }

  template <template <typename Arg> class Op>
  static void reduceMsg(
    std::vector<Scalar>& dest, size_t start_idx, RabenseifnerMsg<Scalar, DataT>* msg) {
    for (uint32_t i = 0; i < msg->size_; i++) {
      Op<Scalar>()(dest[start_idx + i], msg->val_[i]);
    }
  }

  template <template <typename Arg> class Op, typename... Args>
  static void reduce(
    std::vector<Scalar>& dest, Args &&... data) {
    auto& vector_val = DataHan::toVec(std::forward<Args>(data)...);
    Op<std::vector<Scalar>>()(dest, vector_val);
  }

  static bool empty(const std::vector<Scalar>& payload) {
    return payload.empty();
  }
};

#if MAGISTRATE_KOKKOS_ENABLED

template <typename Scalar>
struct DataHelper<Scalar, Kokkos::View<Scalar*, Kokkos::HostSpace>> {
  using DataT = Kokkos::View<Scalar*, Kokkos::HostSpace>;
  using DataType = DataHandler<DataT>;

  template <typename... Args>
  static void assign(DataT& dest, Args&&... data) {
    dest = {std::forward<Args>(data)...};
  }

  static auto createMessage(
    const DataT& payload, size_t begin, size_t count, size_t id,
    int32_t step = 0) {
    return vt::makeMessage<RabenseifnerMsg<Scalar, DataT>>(
      Kokkos::subview(payload, std::make_pair(begin, begin + count)), id, step
    );
  }

  static void
  copy(DataT& dest, size_t start_idx, RabenseifnerMsg<Scalar, DataT>* msg) {
    Kokkos::parallel_for(
      "Rabenseifner::copy", msg->val_.extent(0),
      KOKKOS_LAMBDA(const int i) { dest(start_idx + i) = msg->val_(i); }
    );
  }

  template <template <typename Arg> class Op>
  static void reduceMsg(
    DataT& dest, size_t start_idx, RabenseifnerMsg<Scalar, DataT>* msg) {
    Kokkos::parallel_for(
      "Rabenseifner::reduce", msg->val_.extent(0), KOKKOS_LAMBDA(const int i) {
        Op<Scalar>()(dest(start_idx + i), msg->val_(i));
      }
    );
  }

  template <template <typename Arg> class Op, typename... Args>
  static void reduce(
    DataT& dest, Args&&... val) {
    auto view_val = DataT{std::forward<Args>(val)...};
    Kokkos::parallel_for(
      "Rabenseifner::reduce", view_val.extent(0), KOKKOS_LAMBDA(const int i) {
        Op<Scalar>()(dest(i), view_val(i));
      }
    );
  }

  static bool empty(const DataT& payload) {
    return payload.extent(0) == 0;
  }
};

#endif // MAGISTRATE_KOKKOS_ENABLED

struct StateBase {
  virtual ~StateBase() = default;
  size_t size_ = {};

  uint32_t local_col_wait_count_ = 0;
  bool finished_adjustment_part_ = false;

  int32_t mask_ = 1;
  int32_t step_ = 0;
  bool initialized_ = false;
  bool completed_ = false;

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

template <typename Scalar, typename DataT>
struct State : StateBase {
  ~State() override {
    left_adjust_message_ = nullptr;
    right_adjust_message_ = nullptr;

    for(auto& msg : scatter_messages_){
      msg = nullptr;
    }

    for(auto& msg : gather_messages_){
      msg = nullptr;
    }
  }

  std::vector<Scalar> val_ = {};

  MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>> left_adjust_message_ = nullptr;
  MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>> right_adjust_message_ = nullptr;
  std::vector<MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>>> scatter_messages_ = {};
  std::vector<MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>>> gather_messages_ = {};

  vt::pipe::callback::cbunion::CallbackTyped<DataT> final_handler_ = {};
};

#if MAGISTRATE_KOKKOS_ENABLED
template <typename Scalar>
struct State<Scalar, Kokkos::View<Scalar*, Kokkos::HostSpace>> : StateBase {
  using DataT = Kokkos::View<Scalar*, Kokkos::HostSpace>;

  Kokkos::View<Scalar*, Kokkos::HostSpace> val_ = {};

  MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>> left_adjust_message_ = nullptr;
  MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>> right_adjust_message_ = nullptr;
  std::vector<MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>>> scatter_messages_ = {};
  std::vector<MsgSharedPtr<RabenseifnerMsg<Scalar, DataT>>> gather_messages_ = {};

  vt::pipe::callback::cbunion::CallbackTyped<DataT> final_handler_ = {};
};
#endif //MAGISTRATE_KOKKOS_ENABLED

} // namespace vt::collective::reduce::allreduce
#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_HELPERS_H*/
