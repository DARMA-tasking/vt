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
#include "vt/objgroup/proxy/proxy_objgroup.h"

#include <tuple>
#include <cstdint>

namespace vt::collective::reduce::allreduce {

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
  explicit AllreduceMsg(DataT const& in_val, int step = 0)
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

template <typename DataT>
struct Allreduce {
  void initialize(
    const DataT& data, vt::objgroup::proxy::Proxy<Allreduce> proxy,
    uint32_t num_nodes) {
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

    // std::string str(1024, 0x0);
    // for (int i = 0; i < num_steps_; ++i) {
    //   str.append(fmt::format(
    //     "Step{}: send_idx = {} send_count = {} recieve_idx = {} recieve_count "
    //     "= {}\n",
    //     i, s_index_[i], s_count_[i], r_index_[i], r_count_[i]));
    // }
    // fmt::print(
    //   "[{}] Initialize with size = {} num_steps {} \n {}", this_node_, w_size_,
    //   num_steps_, str);
  }

  void partOneCollective() {
    if (is_part_of_adjustment_group_) {
      auto const partner = is_even_ ? this_node_ + 1 : this_node_ - 1;

      if (is_even_) {
        proxy_[partner].template send<&Allreduce::partOneRightHalf>(
          std::vector<int32_t>{val_.begin() + (val_.size() / 2), val_.end()});
        vrt_node_ = this_node_ / 2;
      } else {
        proxy_[partner].template send<&Allreduce::partOneLeftHalf>(
          std::vector<int32_t>{val_.begin(), val_.end() - (val_.size() / 2)});
        vrt_node_ = -1;
      }
    } else {
      vrt_node_ = this_node_ - nprocs_rem_;
    }

    if (nprocs_rem_ == 0) {
      partTwo();
    }
  }

  void partOneRightHalf(AllreduceMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[(val_.size() / 2) + i] += msg->val_[i];
    }

    // Send to left node
    proxy_[theContext()->getNode() - 1]
      .template send<&Allreduce::partOneFinalPart>(
        std::vector<int32_t>{val_.begin() + (val_.size() / 2), val_.end()});
  }

  void partOneLeftHalf(AllreduceMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[i] += msg->val_[i];
    }
  }

  void partOneFinalPart(AllreduceMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[(val_.size() / 2) + i] = msg->val_[i];
    }

    partTwo();
  }

  void partTwo() {
    auto vdest = vrt_node_ ^ mask_;
    auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

    // fmt::print(
    //   "[{}] Part2 Step {}: Sending to Node {} starting with idx = {} and count "
    //   "{} \n",
    //   this_node_, step_, dest, s_index_[step_], s_count_[step_]);
    proxy_[dest].template send<&Allreduce::partTwoHandler>(
      std::vector<int32_t>{
        val_.begin() + (s_index_[step_]),
        val_.begin() + (s_index_[step_]) + s_count_[step_]},
      step_);

    mask_ <<= 1;
    if (step_ + 1 < num_steps_) {
      step_++;
    }
  }

  void partTwoHandler(AllreduceMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[r_index_[msg->step_] + i] += msg->val_[i];
    }

    // std::string data(128, 0x0);
    // for (auto val : msg->val_) {
    //   data.append(fmt::format("{} ", val));
    // }
    // fmt::print(
    //   "[{}] Part2 Step {}: Received data ({}) idx = {} from {}\n", this_node_,
    //   msg->step_, data, r_index_[msg->step_],
    //   theContext()->getFromNodeCurrentTask());

    if (mask_ < nprocs_pof2_) {
      partTwo();
    } else {
      step_ = num_steps_ - 1;
      mask_ = nprocs_pof2_ >> 1;
      partThree();
    }
  }

  void partThree() {
    auto vdest = vrt_node_ ^ mask_;
    auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

    // std::string data(128, 0x0);
    // auto subV = std::vector<int32_t>{
    //   val_.begin() + (r_index_[step_]),
    //   val_.begin() + (r_index_[step_]) + r_count_[step_]};
    // for (auto val : subV) {
    //   data.append(fmt::format("{} ", val));
    // }

    // fmt::print(
    //   "[{}] Part3 Step {}: Sending to Node {} starting with idx = {} and count "
    //   "{} "
    //   "data={} \n",
    //   this_node_, step_, dest, r_index_[step_], r_count_[step_], data);

    proxy_[dest].template send<&Allreduce::partThreeHandler>(
      std::vector<int32_t>{
        val_.begin() + (r_index_[step_]),
        val_.begin() + (r_index_[step_]) + r_count_[step_]},
      step_);

    mask_ >>= 1;
    step_--;
  }

  void partThreeHandler(AllreduceMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[s_index_[msg->step_] + i] = msg->val_[i];
    }

    // std::string data(128, 0x0);
    // for (auto val : msg->val_) {
    //   data.append(fmt::format("{} ", val));
    // }
    // fmt::print(
    //   "[{}] Part3 Step {}: Received data ({}) idx = {} from {}\n", this_node_,
    //   msg->step_, data, s_index_[msg->step_],
    //   theContext()->getFromNodeCurrentTask());

    if (mask_ > 0) {
      partThree();
    }
  }

  NodeType this_node_ = {};
  bool is_even_ = false;
  vt::objgroup::proxy::Proxy<Allreduce> proxy_ = {};
  DataT val_ = {};
  NodeType vrt_node_ = {};
  bool is_part_of_adjustment_group_ = false;
  int32_t num_steps_ = {};
  int32_t nprocs_pof2_ = {};
  int32_t nprocs_rem_ = {};
  int32_t mask_ = 1;

  size_t w_size_ = {};
  int32_t step_ = 0;
  std::vector<int32_t> r_index_ = {};
  std::vector<int32_t> r_count_ = {};
  std::vector<int32_t> s_index_ = {};
  std::vector<int32_t> s_count_ = {};
};

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_H*/
