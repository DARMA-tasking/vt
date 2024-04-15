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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/messaging/message/message.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"

#include <tuple>
#include <cstdint>

namespace vt::collective::reduce::allreduce {

constexpr bool debug = false;

template <typename DataT>
struct AllreduceRbnMsg
  : SerializeIfNeeded<vt::Message, AllreduceRbnMsg<DataT>, DataT> {
  using MessageParentType =
    SerializeIfNeeded<::vt::Message, AllreduceRbnMsg<DataT>, DataT>;

  AllreduceRbnMsg() = default;
  AllreduceRbnMsg(AllreduceRbnMsg const&) = default;
  AllreduceRbnMsg(AllreduceRbnMsg&&) = default;

  AllreduceRbnMsg(DataT&& in_val, int step = 0)
    : MessageParentType(),
      val_(std::forward<DataT>(in_val)),
      step_(step) { }
  AllreduceRbnMsg(DataT const& in_val, int step = 0)
    : MessageParentType(),
      val_(in_val),
      step_(step) { }

  template <typename SerializeT>
  void serialize(SerializeT& s) {
    MessageParentType::serialize(s);
    s | val_;
    s | step_;
  }

  DataT val_ = {};
  int32_t step_ = {};
};

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
struct Rabenseifner {
  void initialize(
    const DataT& data, vt::objgroup::proxy::Proxy<Rabenseifner> proxy,
    vt::objgroup::proxy::Proxy<ObjT> parentProxy, uint32_t num_nodes) {
    this_node_ = vt::theContext()->getNode();
    is_even_ = this_node_ % 2 == 0;
    val_ = data;
    proxy_ = proxy;
    num_steps_ = static_cast<int32_t>(log2(num_nodes));
    nprocs_pof2_ = 1 << num_steps_;
    nprocs_rem_ = num_nodes - nprocs_pof2_;
    is_part_of_adjustment_group_ = this_node_ < (2 * nprocs_rem_);
    if (is_part_of_adjustment_group_) {
      if (is_even_) {
        vrt_node_ = this_node_ / 2;
      } else {
        vrt_node_ = -1;
      }
    } else {
      vrt_node_ = this_node_ - nprocs_rem_;
    }

    r_index_.resize(num_steps_, 0);
    r_count_.resize(num_steps_, 0);
    s_index_.resize(num_steps_, 0);
    s_count_.resize(num_steps_, 0);

    w_size_ = data.size();

    int step = 0;
    size_t wsize = data.size();
    for (int mask = 1; mask < nprocs_pof2_; mask <<= 1) {
      auto vdest = vrt_node_ ^ mask;
      auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

      if (this_node_ < dest) {
        r_count_[step] = wsize / 2;
        s_count_[step] = wsize - r_count_[step];
        s_index_[step] = r_index_[step] + r_count_[step];
      } else {
        s_count_[step] = wsize / 2;
        r_count_[step] = wsize - s_count_[step];
        r_index_[step] = s_index_[step] + s_count_[step];
      }

      if (step + 1 < num_steps_) {
        r_index_[step + 1] = r_index_[step];
        s_index_[step + 1] = r_index_[step];
        wsize = r_count_[step];
        step++;
      }
    }

    steps_sent_.resize(num_steps_, false);
    steps_recv_.resize(num_steps_, false);

    if constexpr (debug) {
      fmt::print(
        "[{}] Initialize with size = {} num_steps {} \n", this_node_, w_size_,
        num_steps_);
    }
  }

  void partOne() {
    if (is_part_of_adjustment_group_) {
      auto const partner = is_even_ ? this_node_ + 1 : this_node_ - 1;

      if (is_even_) {
        proxy_[partner].template send<&Rabenseifner::partOneRightHalf>(
          DataT{val_.begin() + (val_.size() / 2), val_.end()});
      } else {
        proxy_[partner].template send<&Rabenseifner::partOneLeftHalf>(
          DataT{val_.begin(), val_.end() - (val_.size() / 2)});
      }
    }
  }

  void partOneRightHalf(AllreduceRbnMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[(val_.size() / 2) + i] += msg->val_[i];
    }

    // Send to left node
    proxy_[theContext()->getNode() - 1]
      .template send<&Rabenseifner::partOneFinalPart>(
        DataT{val_.begin() + (val_.size() / 2), val_.end()});
  }

  void partOneLeftHalf(AllreduceRbnMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[i] += msg->val_[i];
    }
  }

  void partOneFinalPart(AllreduceRbnMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[(val_.size() / 2) + i] = msg->val_[i];
    }

    partTwo();
  }

  void partTwo() {
    if (
      vrt_node_ == -1 or (step_ >= num_steps_) or
      (not std::all_of(
        steps_recv_.cbegin(), steps_recv_.cbegin() + step_,
        [](const auto val) { return val; }))) {
      return;
    }

    auto vdest = vrt_node_ ^ mask_;
    auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;
    if constexpr (debug) {
      fmt::print(
        "[{}] Part2 Step {}: Sending to Node {} starting with idx = {} and "
        "count "
        "{} \n",
        this_node_, step_, dest, s_index_[step_], s_count_[step_]);
    }
    proxy_[dest].template send<&Rabenseifner::partTwoHandler>(
      DataT{
        val_.begin() + (s_index_[step_]),
        val_.begin() + (s_index_[step_]) + s_count_[step_]},
      step_);

    mask_ <<= 1;
    num_send_++;
    steps_sent_[step_] = true;
    step_++;

    if (std::all_of(
          steps_recv_.cbegin(), steps_recv_.cbegin() + step_,
          [](const auto val) { return val; })) {
      partTwo();
    }
  }

  void partTwoHandler(AllreduceRbnMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[r_index_[msg->step_] + i] += msg->val_[i];
    }
    if constexpr (debug) {
      fmt::print(
        "[{}] Part2 Step {} mask_= {} nprocs_pof2_ = {}: "
        "idx = {} from {}\n",
        this_node_, msg->step_, mask_, nprocs_pof2_, r_index_[msg->step_],
        theContext()->getFromNodeCurrentTask());
    }
    steps_recv_[msg->step_] = true;
    num_recv_++;
    if (mask_ < nprocs_pof2_) {
      if (std::all_of(
            steps_recv_.cbegin(), steps_recv_.cbegin() + step_,
            [](const auto val) { return val; })) {
        partTwo();
      }
    } else {
      // step_ = num_steps_ - 1;
      // mask_ = nprocs_pof2_ >> 1;
      //  partThree();
    }
  }

  void partThree() {
    if (
      vrt_node_ == -1 or
      (not std::all_of(
        steps_recv_.cbegin() + step_ + 1, steps_recv_.cend(),
        [](const auto val) { return val; }))) {
      return;
    }

    if (not startedPartThree_) {
      step_ = num_steps_ - 1;
      mask_ = nprocs_pof2_ >> 1;
      num_send_ = 0;
      num_recv_ = 0;
      startedPartThree_ = true;
      std::fill(steps_sent_.begin(), steps_sent_.end(), false);
      std::fill(steps_recv_.begin(), steps_recv_.end(), false);
    }

    auto vdest = vrt_node_ ^ mask_;
    auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

    if constexpr (debug) {
      fmt::print(
        "[{}] Part3 Step {}: Sending to Node {} starting with idx = {} and "
        "count "
        "{} \n",
        this_node_, step_, dest, r_index_[step_], r_count_[step_]);
    }
    proxy_[dest].template send<&Rabenseifner::partThreeHandler>(
      DataT{
        val_.begin() + (r_index_[step_]),
        val_.begin() + (r_index_[step_]) + r_count_[step_]},
      step_);

    steps_sent_[step_] = true;
    num_send_++;
    mask_ >>= 1;
    step_--;
    if (
      step_ >= 0 and
      std::all_of(
        steps_recv_.cbegin() + step_ + 1, steps_recv_.cend(),
        [](const auto val) { return val; })) {
      partThree();
    }
  }

  void partThreeHandler(AllreduceRbnMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[s_index_[msg->step_] + i] = msg->val_[i];
    }

    if (not startedPartThree_) {
      step_ = num_steps_ - 1;
      mask_ = nprocs_pof2_ >> 1;
      num_send_ = 0;
      num_recv_ = 0;
      startedPartThree_ = true;
      std::fill(steps_sent_.begin(), steps_sent_.end(), false);
      std::fill(steps_recv_.begin(), steps_recv_.end(), false);
    }

    num_recv_++;
    if constexpr (debug) {
      fmt::print(
        "[{}] Part3 Step {}: Received idx = {} from {}\n", this_node_,
        msg->step_, s_index_[msg->step_],
        theContext()->getFromNodeCurrentTask());
    }

    steps_recv_[msg->step_] = true;

    if (
      mask_ > 0 and
      ((step_ == num_steps_ - 1) or
       std::all_of(
         steps_recv_.cbegin() + step_ + 1, steps_recv_.cend(),
         [](const auto val) { return val; }))) {
      partThree();
    }
  }

  void partFour() {
    if (is_part_of_adjustment_group_ and is_even_) {
      if constexpr (debug) {
        fmt::print(
          "[{}] Part4 : Sending to Node {}  \n", this_node_, this_node_ + 1);
      }
      proxy_[this_node_ + 1].template send<&Rabenseifner::partFourHandler>(
        val_, 0);
    }
  }

  void partFourHandler(AllreduceRbnMsg<DataT>* msg) { val_ = msg->val_; }

  NodeType this_node_ = {};
  bool is_even_ = false;
  vt::objgroup::proxy::Proxy<Rabenseifner> proxy_ = {};
  vt::objgroup::proxy::Proxy<ObjT> parentProxy_ = {};
  DataT val_ = {};
  NodeType vrt_node_ = {};
  bool is_part_of_adjustment_group_ = false;
  int32_t num_steps_ = {};
  int32_t nprocs_pof2_ = {};
  int32_t nprocs_rem_ = {};
  int32_t mask_ = 1;
  bool startedPartThree_ = false;

  size_t w_size_ = {};
  int32_t step_ = 0;
  int32_t num_send_ = 0;
  int32_t num_recv_ = 0;

  std::vector<bool> steps_recv_ = {};
  std::vector<bool> steps_sent_ = {};
  std::vector<int32_t> r_index_ = {};
  std::vector<int32_t> r_count_ = {};
  std::vector<int32_t> s_index_ = {};
  std::vector<int32_t> s_count_ = {};
};

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H*/
