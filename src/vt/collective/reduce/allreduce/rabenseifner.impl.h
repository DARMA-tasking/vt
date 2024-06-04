/*
//@HEADER
// *****************************************************************************
//
//                             rabenseifner.impl.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_IMPL_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_IMPL_H

#include "vt/config.h"

namespace vt::collective::reduce::allreduce {

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
template <typename... Args>
Rabenseifner<DataT, Op, ObjT, finalHandler>::Rabenseifner(
  vt::objgroup::proxy::Proxy<ObjT> parentProxy, NodeType num_nodes,
  Args&&... args)
  : parent_proxy_(parentProxy),
    num_nodes_(num_nodes),
    this_node_(vt::theContext()->getNode()),
    is_even_(this_node_ % 2 == 0),
    num_steps_(static_cast<int32_t>(log2(num_nodes_))),
    nprocs_pof2_(1 << num_steps_),
    nprocs_rem_(num_nodes_ - nprocs_pof2_),
    finished_adjustment_part_(nprocs_rem_ == 0),
    gather_step_(num_steps_ - 1),
    gather_mask_(nprocs_pof2_ >> 1) {
  initialize(std::forward<Args>(args)...);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
template <typename... Args>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::initialize(Args&&... args) {
  val_ = DataT(std::forward<Args>(args)...);

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
  size_t wsize = DataType::size(val_);
  size_ = wsize;
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

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::executeFinalHan() {
  // theCB()->makeSend<finalHandler>(parent_proxy_[this_node_]).sendTuple(std::make_tuple(val_));
  parent_proxy_[this_node_].template invoke<finalHandler>(val_);
  completed_ = true;
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::allreduce() {
  if (nprocs_rem_) {
    adjustForPowerOfTwo();
  } else {
    scatterReduceIter();
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::adjustForPowerOfTwo() {
  if (is_part_of_adjustment_group_) {
    auto const partner = is_even_ ? this_node_ + 1 : this_node_ - 1;

    if (is_even_) {
      proxy_[partner]
        .template send<&Rabenseifner::adjustForPowerOfTwoRightHalf>(
          DataType::split(val_, size_ / 2, size_));
    } else {
      proxy_[partner].template send<&Rabenseifner::adjustForPowerOfTwoLeftHalf>(
        DataType::split(val_, 0, size_ / 2));
    }
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::adjustForPowerOfTwoRightHalf(
  AllreduceRbnMsg<DataT>* msg) {
  for (uint32_t i = 0; i < DataType::size(msg->val_); i++) {
    Op<typename DataType::Scalar>()(
      DataType::at(val_, (size_ / 2) + i), DataType::at(msg->val_, i));
  }

  // Send to left node
  proxy_[theContext()->getNode() - 1]
    .template send<&Rabenseifner::adjustForPowerOfTwoFinalPart>(
      DataType::split(val_, size_ / 2, size_));
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::adjustForPowerOfTwoLeftHalf(
  AllreduceRbnMsg<DataT>* msg) {
  for (uint32_t i = 0; i < DataType::size(msg->val_); i++) {
    Op<typename DataType::Scalar>()(
      DataType::at(val_, i), DataType::at(msg->val_, i));
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::adjustForPowerOfTwoFinalPart(
  AllreduceRbnMsg<DataT>* msg) {
  for (uint32_t i = 0; i < DataType::size(msg->val_); i++) {
    DataType::at(val_, (size_ / 2) + i) = DataType::at(msg->val_, i);
  }

  finished_adjustment_part_ = true;

  scatterReduceIter();
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterAllMessagesReceived() {
  return std::all_of(
    scatter_steps_recv_.cbegin(), scatter_steps_recv_.cbegin() + scatter_step_,
    [](auto const val) { return val; });
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterIsDone() {
  return scatter_step_ == num_steps_ and scatter_num_recv_ == num_steps_;
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterIsReady() {
  return ((is_part_of_adjustment_group_ and finished_adjustment_part_) and
          scatter_step_ == 0) or
    scatterAllMessagesReceived();
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterTryReduce(
  int32_t step) {
  if (
    (step < scatter_step_) and not scatter_steps_reduced_[step] and
    scatter_steps_recv_[step] and
    std::all_of(
      scatter_steps_reduced_.cbegin(), scatter_steps_reduced_.cbegin() + step,
      [](auto const val) { return val; })) {
    auto& in_msg = scatter_messages_.at(step);
    auto& in_val = in_msg->val_;
    for (uint32_t i = 0; i < DataType::size(in_val); i++) {
      Op<typename DataType::Scalar>()(
        DataType::at(val_, r_index_[in_msg->step_] + i),
        DataType::at(in_val, i)
      );
    }

    scatter_steps_reduced_[step] = true;
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterReduceIter() {
  if (not scatterIsReady()) {
    return;
  }

  auto vdest = vrt_node_ ^ scatter_mask_;
  auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;
  vt_debug_print(
    terse, allreduce,
    "[{}] Rabenseifer Part2 (step {}): Sending to Node {} starting with idx = {} and "
    "count "
    "{} \n",
    this_node_, scatter_step_, dest, s_index_[scatter_step_],
    s_count_[scatter_step_]
  );

  proxy_[dest].template send<&Rabenseifner::scatterReduceIterHandler>(
    DataType::split(
      val_, s_index_[scatter_step_],
      s_index_[scatter_step_] + s_count_[scatter_step_]),
    scatter_step_);

  scatter_mask_ <<= 1;
  scatter_step_++;

  scatterTryReduce(scatter_step_ - 1);

  if (scatterIsDone()) {
    finished_scatter_part_ = true;
    gatherIter();
  } else if (scatterAllMessagesReceived()) {
    scatterReduceIter();
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterReduceIterHandler(
  AllreduceRbnMsg<DataT>* msg) {
  scatter_messages_[msg->step_] = promoteMsg(msg);
  scatter_steps_recv_[msg->step_] = true;
  scatter_num_recv_++;

  if (not finished_adjustment_part_) {
    return;
  }

  scatterTryReduce(msg->step_);

  vt_debug_print(
    terse, allreduce,
    "[{}] Rabenseifner Part2 (step {}): scatter_mask_= {} nprocs_pof2_ = {}: "
    "idx = {} from {}\n",
    this_node_, msg->step_, scatter_mask_, nprocs_pof2_, r_index_[msg->step_],
    theContext()->getFromNodeCurrentTask()
  );

  if ((scatter_mask_ < nprocs_pof2_) and scatterAllMessagesReceived()) {
    scatterReduceIter();
  } else if (scatterIsDone()) {
    finished_scatter_part_ = true;
    gatherIter();
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherAllMessagesReceived() {
  return std::all_of(
    gather_steps_recv_.cbegin() + gather_step_ + 1, gather_steps_recv_.cend(),
    [](auto const val) { return val; });
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherIsDone() {
  return (gather_step_ < 0) and (gather_num_recv_ == num_steps_);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherIsReady() {
  return (gather_step_ == num_steps_ - 1) or gatherAllMessagesReceived();
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherTryReduce(
  int32_t step) {
  auto const doRed = (step > gather_step_) and
    not gather_steps_reduced_[step] and gather_steps_recv_[step] and
    std::all_of(gather_steps_reduced_.cbegin() + step + 1,
                gather_steps_reduced_.cend(),
                [](auto const val) { return val; });

  if (doRed) {
    auto& in_msg = gather_messages_.at(step);
    auto& in_val = in_msg->val_;
    for (uint32_t i = 0; i < DataType::size(in_val); i++) {
      DataType::at(val_, s_index_[in_msg->step_] + i) = DataType::at(in_val, i);
    }

    gather_steps_reduced_[step] = true;
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherIter() {
  if (not gatherIsReady()) {
    return;
  }

  auto vdest = vrt_node_ ^ gather_mask_;
  auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

  vt_debug_print(
    terse, allreduce,
    "[{}] Rabenseifner Part3 (step {}): Sending to Node {} starting with idx = {} and "
    "count "
    "{} \n",
    this_node_, gather_step_, dest, r_index_[gather_step_],
    r_count_[gather_step_]
  );

  proxy_[dest].template send<&Rabenseifner::gatherIterHandler>(
    DataType::split(
      val_, r_index_[gather_step_],
      r_index_[gather_step_] + r_count_[gather_step_]),
    gather_step_);

  gather_mask_ >>= 1;
  gather_step_--;

  gatherTryReduce(gather_step_ + 1);

  if (gatherIsDone()) {
    finalPart();
  } else if (gatherIsReady()) {
    gatherIter();
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherIterHandler(
  AllreduceRbnMsg<DataT>* msg) {
  vt_debug_print(
    terse, allreduce, "[{}] Rabenseifner Part3 (step {}): Received idx = {} from {}\n",
    this_node_, msg->step_, s_index_[msg->step_],
    theContext()->getFromNodeCurrentTask()
  );

  gather_messages_[msg->step_] = promoteMsg(msg);
  gather_steps_recv_[msg->step_] = true;
  gather_num_recv_++;

  if (not finished_scatter_part_) {
    return;
  }

  gatherTryReduce(msg->step_);

  if (gather_mask_ > 0 and gatherIsReady()) {
    gatherIter();
  } else if (gatherIsDone()) {
    finalPart();
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::finalPart() {
  if (completed_) {
    return;
  }

  if (nprocs_rem_) {
    sendToExcludedNodes();
  }

  executeFinalHan();
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::sendToExcludedNodes() {
  if (is_part_of_adjustment_group_ and is_even_) {
    vt_debug_print(
      terse, allreduce, "[{}] Rabenseifner Part4: Sending to Node {}  \n", this_node_,
      this_node_ + 1
    );
    proxy_[this_node_ + 1]
      .template send<&Rabenseifner::sendToExcludedNodesHandler>(val_, 0);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::sendToExcludedNodesHandler(
  AllreduceRbnMsg<DataT>* msg) {
  val_ = msg->val_;

  executeFinalHan();
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_IMPL_H*/
