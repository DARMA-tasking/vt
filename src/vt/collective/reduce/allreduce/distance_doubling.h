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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_DISTANCE_DOUBLING_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_DISTANCE_DOUBLING_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/messaging/message/message.h"
#include "vt/objgroup/proxy/proxy_objgroup.h"
#include "vt/configs/error/config_assert.h"
#include "vt/messaging/message/smart_ptr.h"

#include <tuple>
#include <cstdint>

namespace vt::collective::reduce::allreduce {

constexpr bool isdebug = false;

template <typename DataT>
struct AllreduceDblMsg
  : SerializeIfNeeded<vt::Message, AllreduceDblMsg<DataT>, DataT> {
  using MessageParentType =
    SerializeIfNeeded<::vt::Message, AllreduceDblMsg<DataT>, DataT>;

  AllreduceDblMsg() = default;
  AllreduceDblMsg(AllreduceDblMsg const&) = default;
  AllreduceDblMsg(AllreduceDblMsg&&) = default;

  explicit AllreduceDblMsg(DataT&& in_val)
    : MessageParentType(),
      val_(std::forward<DataT>(in_val)) { }
  explicit AllreduceDblMsg(DataT const& in_val, int step = 0)
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

template <typename DataT, typename Op, typename ObjT, auto finalHandler>
struct DistanceDoubling {
  void initialize(
    const DataT& data, vt::objgroup::proxy::Proxy<DistanceDoubling> proxy,
    vt::objgroup::proxy::Proxy<ObjT> parentProxy,
    uint32_t num_nodes) {
    this_node_ = vt::theContext()->getNode();
    is_even_ = this_node_ % 2 == 0;
    val_ = data;
    proxy_ = proxy;
    parentProxy_ = parentProxy;
    num_steps_ = static_cast<int32_t>(log2(num_nodes));
    messages.resize(num_steps_, nullptr);

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

    w_size_ = data.size();

    expected_send_ = num_steps_;
    expected_recv_ = num_steps_;
    steps_sent_.resize(num_steps_, false);
    steps_recv_.resize(num_steps_, false);
  }

  void partOne() {
    if (not nprocs_rem_) {
      // we're running on power of 2 number of nodes, proceed to second step
      partTwo();
    } else if (is_part_of_adjustment_group_ and not is_even_) {
      proxy_[this_node_ - 1].template send<&DistanceDoubling::partOneHandler>(
        val_);
    }
  }

  void partOneHandler(AllreduceDblMsg<DataT>* msg) {
    Op(val_, msg->val_);
    // for (int i = 0; i < msg->val_.size(); i++) {
    //   val_[i] += msg->val_[i];
    // }

    partTwo();
  }

  bool isValid() { return (vrt_node_ != -1) and (step_ < num_steps_); }
  bool isReady() {
    return std::all_of(
      steps_recv_.cbegin(), steps_recv_.cbegin() + step_,
      [](const auto val) { return val; });
  }
  void partTwo() {
    if (not isValid() or not isReady()) {
      return;
    }

    auto vdest = vrt_node_ ^ mask_;
    auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;
    if constexpr (isdebug) {
      fmt::print(
        "[{}] Part2 Step {}: Sending to Node {} \n", this_node_, step_, dest);
    }
    if (step_) {
      for (int i = 0; i < val_.size(); ++i) {
        val_[i] += messages.at(step_ - 1)->val_[i];
      }
    }

    proxy_[dest].template send<&DistanceDoubling::partTwoHandler>(val_, step_);

    mask_ <<= 1;
    num_send_++;
    steps_sent_[step_] = true;
    step_++;

    if (isReady()) {
      partTwo();
    }
  }

  void partTwoHandler(AllreduceDblMsg<DataT>* msg) {
    messages.at(msg->step_) = promoteMsg(msg);

    if constexpr (isdebug) {
      std::string data(1024, 0x0);
      for (auto val : msg->val_) {
        data.append(fmt::format("{} ", val));
      }
      fmt::print(
        "[{}] Part2 Step {} mask_= {} nprocs_pof2_ = {}: Received data ({}) "
        "from {}\n",
        this_node_, msg->step_, mask_, nprocs_pof2_, data,
        theContext()->getFromNodeCurrentTask());
    }
    steps_recv_[msg->step_] = true;
    num_recv_++;
    if (mask_ < nprocs_pof2_) {
      if (isReady()) {
        partTwo();
      }
    }
  }

  void partThree() {
    if (is_part_of_adjustment_group_ and is_even_) {
      if constexpr (isdebug) {
        fmt::print(
          "[{}] Part4 : Sending to Node {}  \n", this_node_, this_node_ + 1);
      }
      proxy_[this_node_ + 1].template send<&DistanceDoubling::partThreeHandler>(
        val_);
    }
  }

  void partThreeHandler(AllreduceDblMsg<DataT>* msg) { val_ = msg->val_; }
  void finalPart() {
    if (vrt_node_ != -1) {
      for (int i = 0; i < val_.size(); ++i) {
        val_[i] += messages.at(step_ - 1)->val_[i];
      }
    }

    parentProxy_[this_node_] .template invoke<finalHandler>(val_);
  }

  NodeType this_node_ = {};
  bool is_even_ = false;
  vt::objgroup::proxy::Proxy<DistanceDoubling> proxy_ = {};
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
  int32_t expected_send_ = 0;
  int32_t num_recv_ = 0;
  int32_t expected_recv_ = 0;

  std::vector<bool> steps_recv_ = {};
  std::vector<bool> steps_sent_ = {};

  std::vector<MsgSharedPtr<AllreduceDblMsg<DataT>>> messages = {};
};

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H*/
