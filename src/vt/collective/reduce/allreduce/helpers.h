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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_HELPERS_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_HELPERS_H

#include "data_handler.h"
#include "rabenseifner_msg.h"
#include "vt/messaging/message/shared_message.h"
#include "vt/utils/kokkos/exec_space.h"

#include <vector>

namespace vt::collective::reduce::allreduce {

template <typename Scalar, typename DataT>
struct ShouldUseView {
  static constexpr bool Value = false;
};

#if MAGISTRATE_KOKKOS_ENABLED
template <typename Scalar>
struct ShouldUseView<Scalar, Kokkos::View<Scalar*, Kokkos::HostSpace>> {
  static constexpr bool Value = true;
};

#endif // MAGISTRATE_KOKKOS_ENABLED

template <typename Scalar, typename DataT>
inline constexpr bool ShouldUseView_v = ShouldUseView<Scalar, DataT>::Value;

template <typename Scalar, typename DataT>
struct DataHelper {
  using DataHan = DataHandler<DataT>;

  template <typename... Args>
  static void assignFromMem(std::vector<Scalar>& dest, const Scalar* data, size_t size) {
    std::memcpy(dest.data(), data, size * sizeof(Scalar));
  }

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

template <typename Scalar, typename MemorySpace>
struct DataHelper<Scalar, Kokkos::View<Scalar*, MemorySpace>> {
  using DataT = Kokkos::View<Scalar*, MemorySpace>;
  using ExecSpace = typename utils::kokkos::AssociatedExecSpace<MemorySpace>::type;
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

    Kokkos::RangePolicy<ExecSpace> policy(0, msg->val_.extent(0));
    Kokkos::parallel_for(
      "Rabenseifner::copy", policy,
      KOKKOS_LAMBDA(const int i) { dest(start_idx + i) = msg->val_(i); }
    );
  }

  template <template <typename Arg> class Op>
  static void reduceMsg(
    DataT& dest, size_t start_idx, RabenseifnerMsg<Scalar, DataT>* msg) {

    Kokkos::RangePolicy<ExecSpace> policy(0, msg->val_.extent(0));
    Kokkos::parallel_for(
      "Rabenseifner::reduce", policy, KOKKOS_LAMBDA(const int i) {
        Op<Scalar>()(dest(start_idx + i), msg->val_(i));
      }
    );
  }

  template <template <typename Arg> class Op, typename... Args>
  static void reduce(
    DataT& dest, Args&&... val) {
    auto view_val = DataT{std::forward<Args>(val)...};

    Kokkos::RangePolicy<ExecSpace> policy(0, view_val.extent(0));
    Kokkos::parallel_for(
      "Rabenseifner::reduce", policy, KOKKOS_LAMBDA(const int i) {
        Op<Scalar>()(dest(i), view_val(i));
      }
    );
  }

  static bool empty(const DataT& payload) {
    return payload.extent(0) == 0;
  }
};

#endif // MAGISTRATE_KOKKOS_ENABLED

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_HELPERS_H*/
