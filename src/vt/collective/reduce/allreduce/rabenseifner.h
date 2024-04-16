/*
//@HEADER
// *****************************************************************************
//
//                                rabenseifner.h
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
  template <typename... Args>
  Rabenseifner(
    vt::objgroup::proxy::Proxy<ObjT> parentProxy, NodeType num_nodes,
    Args&&... args)
    : parent_proxy_(parentProxy),
      val_(std::forward<Args>(args)...),
      num_nodes_(num_nodes),
      this_node_(vt::theContext()->getNode()),
      is_even_(this_node_ % 2 == 0),
      num_steps_(static_cast<int32_t>(log2(num_nodes_))),
      nprocs_pof2_(1 << num_steps_),
      nprocs_rem_(num_nodes_ - nprocs_pof2_),
      gather_step_(num_steps_ - 1),
      gather_mask_(nprocs_pof2_ >> 1),
      finished_adjustment_part_(nprocs_rem_ == 0) {
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

    scatter_messages_.resize(num_steps_, nullptr);
    scatter_steps_recv_.resize(num_steps_, false);
    scatter_steps_reduced_.resize(num_steps_, false);

    gather_messages_.resize(num_steps_, nullptr);
    gather_steps_recv_.resize(num_steps_, false);
    gather_steps_reduced_.resize(num_steps_, false);

    r_index_.resize(num_steps_, 0);
    r_count_.resize(num_steps_, 0);
    s_index_.resize(num_steps_, 0);
    s_count_.resize(num_steps_, 0);

    int step = 0;
    size_t wsize = val_.size();
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

    scatter_steps_recv_.resize(num_steps_, false);
  }

  void allreduce() {
    if (nprocs_rem_) {
      adjustForPowerOfTwo();
    } else {
      scatterReduceIter();
    }
  }

  void adjustForPowerOfTwo() {
    if (is_part_of_adjustment_group_) {
      auto const partner = is_even_ ? this_node_ + 1 : this_node_ - 1;

      if (is_even_) {
        proxy_[partner]
          .template send<&Rabenseifner::adjustForPowerOfTwoRightHalf>(
            DataT{val_.begin() + (val_.size() / 2), val_.end()});
      } else {
        proxy_[partner]
          .template send<&Rabenseifner::adjustForPowerOfTwoLeftHalf>(
            DataT{val_.begin(), val_.end() - (val_.size() / 2)});
      }
    }
  }

  void adjustForPowerOfTwoRightHalf(AllreduceRbnMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[(val_.size() / 2) + i] += msg->val_[i];
    }

    // Send to left node
    proxy_[theContext()->getNode() - 1]
      .template send<&Rabenseifner::adjustForPowerOfTwoFinalPart>(
        DataT{val_.begin() + (val_.size() / 2), val_.end()});
  }

  void adjustForPowerOfTwoLeftHalf(AllreduceRbnMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[i] += msg->val_[i];
    }
  }

  void adjustForPowerOfTwoFinalPart(AllreduceRbnMsg<DataT>* msg) {
    for (int i = 0; i < msg->val_.size(); i++) {
      val_[(val_.size() / 2) + i] = msg->val_[i];
    }

    finished_adjustment_part_ = true;

    scatterReduceIter();
  }

  void printValues() {
    if constexpr (debug) {
      std::string printer(1024, 0x0);
      for (auto val : val_) {
        printer.append(fmt::format("{} ", val));
      }
      fmt::print("[{}] Values = {} \n", this_node_, printer);
    }
  }

  bool scatterAllMessagesReceived() {
    return std::all_of(
      scatter_steps_recv_.cbegin(),
      scatter_steps_recv_.cbegin() + scatter_step_,
      [](const auto val) { return val; });
  }

  bool scatterIsDone() {
    return scatter_step_ == num_steps_ and scatter_num_recv_ == num_steps_;
  }

  bool scatterIsReady() {
    return (is_part_of_adjustment_group_ and finished_adjustment_part_) and
      scatter_step_ == 0 or
      scatterAllMessagesReceived();
  }

  void scatterTryReduce(int32_t step) {
    if (
      (step < scatter_step_) and not scatter_steps_reduced_[step] and
      scatter_steps_recv_[step] and
      std::all_of(
        scatter_steps_reduced_.cbegin(), scatter_steps_reduced_.cbegin() + step,
        [](const auto val) { return val; })) {
      auto& in_msg = scatter_messages_.at(step);
      auto& in_val = in_msg->val_;
      for (int i = 0; i < in_val.size(); i++) {
        Op<typename DataT::value_type>()(
          val_[r_index_[in_msg->step_] + i], in_val[i]);
      }

      scatter_steps_reduced_[step] = true;
    }
  }

  void scatterReduceIter() {
    if (not scatterIsReady()) {
      return;
    }

    auto vdest = vrt_node_ ^ scatter_mask_;
    auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;
    if constexpr (debug) {
      fmt::print(
        "[{}] Part2 Step {}: Sending to Node {} starting with idx = {} and "
        "count "
        "{} \n",
        this_node_, scatter_step_, dest, s_index_[scatter_step_],
        s_count_[scatter_step_]);
    }
    proxy_[dest].template send<&Rabenseifner::scatterReduceIterHandler>(
      DataT{
        val_.begin() + (s_index_[scatter_step_]),
        val_.begin() + (s_index_[scatter_step_]) + s_count_[scatter_step_]},
      scatter_step_);

    scatter_mask_ <<= 1;
    scatter_step_++;

    scatterTryReduce(scatter_step_ - 1);

    if (scatterIsDone()) {
      printValues();
      finished_scatter_part_ = true;
      gatherIter();
    } else if (scatterAllMessagesReceived()) {
      scatterReduceIter();
    }
  }

  void scatterReduceIterHandler(AllreduceRbnMsg<DataT>* msg) {
    scatter_messages_[msg->step_] = promoteMsg(msg);
    scatter_steps_recv_[msg->step_] = true;
    scatter_num_recv_++;

    if (not finished_adjustment_part_) {
      return;
    }

    scatterTryReduce(msg->step_);

    if constexpr (debug) {
      fmt::print(
        "[{}] Part2 Step {} scatter_mask_= {} nprocs_pof2_ = {}: "
        "idx = {} from {}\n",
        this_node_, msg->step_, scatter_mask_, nprocs_pof2_,
        r_index_[msg->step_], theContext()->getFromNodeCurrentTask());
    }

    if ((scatter_mask_ < nprocs_pof2_) and scatterAllMessagesReceived()) {
      scatterReduceIter();
    } else if (scatterIsDone()) {
      printValues();
      finished_scatter_part_ = true;
      gatherIter();
    }
  }

  bool gatherAllMessagesReceived() {
    return std::all_of(
      gather_steps_recv_.cbegin() + gather_step_ + 1, gather_steps_recv_.cend(),
      [](const auto val) { return val; });
  }

  bool gatherIsDone() {
    return (gather_step_ < 0) and (gather_num_recv_ == num_steps_);
  }

  bool gatherIsReady() {
    return (gather_step_ == num_steps_ - 1) or gatherAllMessagesReceived();
  }

  void gatherTryReduce(int32_t step) {
    const auto doRed = (step > gather_step_) and
      not gather_steps_reduced_[step] and gather_steps_recv_[step] and
      std::all_of(gather_steps_reduced_.cbegin() + step + 1,
                  gather_steps_reduced_.cend(),
                  [](const auto val) { return val; });

    if (doRed) {
      auto& in_msg = gather_messages_.at(step);
      auto& in_val = in_msg->val_;
      for (int i = 0; i < in_val.size(); i++) {
        val_[s_index_[in_msg->step_] + i] = in_val[i];
      }

      gather_steps_reduced_[step] = true;
    }
  }

  void gatherIter() {
    if (not gatherIsReady()) {
      return;
    }

    auto vdest = vrt_node_ ^ gather_mask_;
    auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

    if constexpr (debug) {
      fmt::print(
        "[{}] Part3 Step {}: Sending to Node {} starting with idx = {} and "
        "count "
        "{} \n",
        this_node_, gather_step_, dest, r_index_[gather_step_],
        r_count_[gather_step_]);
    }
    proxy_[dest].template send<&Rabenseifner::gatherIterHandler>(
      DataT{
        val_.begin() + (r_index_[gather_step_]),
        val_.begin() + (r_index_[gather_step_]) + r_count_[gather_step_]},
      gather_step_);

    gather_mask_ >>= 1;
    gather_step_--;

    gatherTryReduce(gather_step_ + 1);
    printValues();

    if (gatherIsDone()) {
      finalPart();
    } else if (gatherIsReady()) {
      gatherIter();
    }
  }

  void gatherIterHandler(AllreduceRbnMsg<DataT>* msg) {
    if constexpr (debug) {
      fmt::print(
        "[{}] Part3 Step {}: Received idx = {} from {}\n", this_node_,
        msg->step_, s_index_[msg->step_],
        theContext()->getFromNodeCurrentTask());
    }

    gather_messages_[msg->step_] = promoteMsg(msg);
    gather_steps_recv_[msg->step_] = true;
    gather_num_recv_++;

    if (not finished_scatter_part_) {
      return;
    }

    gatherTryReduce(msg->step_);
    printValues();

    if (gather_mask_ > 0 and gatherIsReady()) {
      gatherIter();
    } else if (gatherIsDone()) {
      finalPart();
    }
  }

  void finalPart() {
    if (completed_) {
      return;
    }

    if (nprocs_rem_) {
      sendToExcludedNodes();
    }

    parent_proxy_[this_node_].template invoke<finalHandler>(val_);
    completed_ = true;
  }

  void sendToExcludedNodes() {
    if (is_part_of_adjustment_group_ and is_even_) {
      if constexpr (debug) {
        fmt::print(
          "[{}] Part4 : Sending to Node {}  \n", this_node_, this_node_ + 1);
      }
      proxy_[this_node_ + 1]
        .template send<&Rabenseifner::sendToExcludedNodesHandler>(val_, 0);
    }
  }

  void sendToExcludedNodesHandler(AllreduceRbnMsg<DataT>* msg) {
    val_ = msg->val_;

    parent_proxy_[this_node_].template invoke<finalHandler>(val_);
    completed_ = true;
  }

  vt::objgroup::proxy::Proxy<Rabenseifner> proxy_ = {};
  vt::objgroup::proxy::Proxy<ObjT> parent_proxy_ = {};

  DataT val_ = {};
  NodeType this_node_ = {};
  NodeType num_nodes_ = {};
  bool is_even_ = false;
  int32_t num_steps_ = {};
  int32_t nprocs_pof2_ = {};
  int32_t nprocs_rem_ = {};

  std::vector<int32_t> r_index_ = {};
  std::vector<int32_t> r_count_ = {};
  std::vector<int32_t> s_index_ = {};
  std::vector<int32_t> s_count_ = {};

  NodeType vrt_node_ = {};
  bool is_part_of_adjustment_group_ = false;
  bool finished_adjustment_part_ = false;

  bool completed_ = false;

  // Scatter
  int32_t scatter_mask_ = 1;
  int32_t scatter_step_ = 0;
  int32_t scatter_num_recv_ = 0;
  std::vector<bool> scatter_steps_recv_ = {};
  std::vector<bool> scatter_steps_reduced_ = {};
  std::vector<MsgSharedPtr<AllreduceRbnMsg<DataT>>> scatter_messages_ = {};
  bool finished_scatter_part_ = false;

  // Gather
  int32_t gather_mask_ = 1;
  int32_t gather_step_ = 0;
  int32_t gather_num_recv_ = 0;
  std::vector<bool> gather_steps_recv_ = {};
  std::vector<bool> gather_steps_reduced_ = {};
  std::vector<MsgSharedPtr<AllreduceRbnMsg<DataT>>> gather_messages_ = {};
};

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_H*/
