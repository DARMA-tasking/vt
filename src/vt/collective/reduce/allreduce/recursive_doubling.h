/*
//@HEADER
// *****************************************************************************
//
//                             recursive_doubling.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_H

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

  AllreduceDblMsg(DataT&& in_val, int step = 0)
    : MessageParentType(),
      val_(std::forward<DataT>(in_val)),
      step_(step) { }
  AllreduceDblMsg(DataT const& in_val, int step = 0)
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
struct DistanceDoubling {
  template <typename... Args>
  DistanceDoubling(
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

    messages_.resize(num_steps_, nullptr);
    steps_recv_.resize(num_steps_, false);
    steps_reduced_.resize(num_steps_, false);
  }

  void allreduce() {
    if (nprocs_rem_) {
      adjustForPowerOfTwo();
    } else {
      reduceIter();
    }
  }

  void adjustForPowerOfTwo() {
    if (is_part_of_adjustment_group_ and not is_even_) {
      if constexpr (isdebug) {
        fmt::print(
          "[{}] Part1: Sending to Node {} \n", this_node_, this_node_ - 1);
      }

      proxy_[this_node_ - 1]
        .template send<&DistanceDoubling::adjustForPowerOfTwoHandler>(val_);
    }
  }

  void adjustForPowerOfTwoHandler(AllreduceDblMsg<DataT>* msg) {
    if constexpr (isdebug) {
      std::string data(1024, 0x0);
      for (auto val : msg->val_) {
        data.append(fmt::format("{} ", val));
      }
      fmt::print(
        "[{}] Part1 Handler: Received data ({}) "
        "from {}\n",
        this_node_, data, theContext()->getFromNodeCurrentTask());
    }

    Op<DataT>()(val_, msg->val_);

    finished_adjustment_part_ = true;

    reduceIter();
  }

  bool done() { return step_ == num_steps_ and allMessagesReceived(); }
  bool isValid() { return (vrt_node_ != -1) and (step_ < num_steps_); }
  bool allMessagesReceived() {
    return std::all_of(
      steps_recv_.cbegin(), steps_recv_.cbegin() + step_,
      [](const auto val) { return val; });
  }
  bool isReady() {
    return (is_part_of_adjustment_group_ and finished_adjustment_part_) and
      step_ == 0 or
      allMessagesReceived();
  }

  void reduceIter() {
    // Ensure we have received all necessary messages
    if (not isReady()) {
      return;
    }

    auto vdest = vrt_node_ ^ mask_;
    auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;
    if constexpr (isdebug) {
      fmt::print(
        "[{}] Part2 Step {}: Sending to Node {} \n", this_node_, step_, dest);
    }

    proxy_[dest].template send<&DistanceDoubling::reduceIterHandler>(
      val_, step_);

    mask_ <<= 1;
    step_++;

    tryReduce(step_ - 1);

    if (done()) {
      finalPart();
    } else if (isReady()) {
      reduceIter();
    }
  }

  void tryReduce(int32_t step) {
    if (
      (step < step_) and not steps_reduced_[step] and steps_recv_[step] and
      std::all_of(
        steps_reduced_.cbegin(), steps_reduced_.cbegin() + step,
        [](const auto val) { return val; })) {
      Op<DataT>()(val_, messages_.at(step)->val_);
      steps_reduced_[step] = true;
    }
  }

  void reduceIterHandler(AllreduceDblMsg<DataT>* msg) {
    if constexpr (isdebug) {
      std::string data(1024, 0x0);
      for (auto val : msg->val_) {
        data.append(fmt::format("{} ", val));
      }
      fmt::print(
        "[{}] Part2 Step {} mask_= {} nprocs_pof2_ = {}: "
        "Received data ({}) "
        "from {}\n",
        this_node_, msg->step_, mask_, nprocs_pof2_, data,
        theContext()->getFromNodeCurrentTask());
    }

    messages_.at(msg->step_) = promoteMsg(msg);
    steps_recv_[msg->step_] = true;

    // Special case when we receive step 2 message before step 1 is done on this node
    if (not finished_adjustment_part_) {
      return;
    }

    tryReduce(msg->step_);

    if ((mask_ < nprocs_pof2_) and isReady()) {
      reduceIter();

    } else if (done()) {
      finalPart();
    }
  }

  void sendToExcludedNodes() {
    if (is_part_of_adjustment_group_ and is_even_) {
      if constexpr (isdebug) {
        fmt::print(
          "[{}] Part3 : Sending to Node {}  \n", this_node_, this_node_ + 1);
      }
      proxy_[this_node_ + 1]
        .template send<&DistanceDoubling::sendToExcludedNodesHandler>(val_);
    }
  }

  void sendToExcludedNodesHandler(AllreduceDblMsg<DataT>* msg) {
    val_ = msg->val_;

    parent_proxy_[this_node_].template invoke<finalHandler>(val_);
    completed_ = true;
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

  vt::objgroup::proxy::Proxy<DistanceDoubling> proxy_ = {};
  vt::objgroup::proxy::Proxy<ObjT> parent_proxy_ = {};

  DataT val_ = {};
  NodeType this_node_ = {};
  NodeType num_nodes_ = {};
  bool is_even_ = false;
  int32_t num_steps_ = {};
  int32_t nprocs_pof2_ = {};
  int32_t nprocs_rem_ = {};

  NodeType vrt_node_ = {};
  bool is_part_of_adjustment_group_ = false;
  bool finished_adjustment_part_ = false;

  int32_t mask_ = 1;

  int32_t step_ = 0;
  bool completed_ = false;

  std::vector<bool> steps_recv_ = {};
  std::vector<bool> steps_reduced_ = {};

  std::vector<MsgSharedPtr<AllreduceDblMsg<DataT>>> messages_ = {};
};

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_H*/
