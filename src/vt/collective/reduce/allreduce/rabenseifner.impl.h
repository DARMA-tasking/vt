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

#include "vt/collective/reduce/allreduce/data_handler.h"
#include "vt/config.h"

namespace vt::collective::reduce::allreduce {

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
template <typename... Args>
Rabenseifner<DataT, Op, ObjT, finalHandler>::Rabenseifner(
  vt::objgroup::proxy::Proxy<ObjT> parentProxy, NodeType num_nodes,
  Args&&... data)
  : parent_proxy_(parentProxy),
    num_nodes_(num_nodes),
    this_node_(vt::theContext()->getNode()),
    is_even_(this_node_ % 2 == 0),
    num_steps_(static_cast<int32_t>(log2(num_nodes_))),
    nprocs_pof2_(1 << num_steps_),
    nprocs_rem_(num_nodes_ - nprocs_pof2_),
    is_part_of_adjustment_group_(this_node_ < (2 * nprocs_rem_)) {
  if (is_part_of_adjustment_group_) {
    if (is_even_) {
      vrt_node_ = this_node_ / 2;
    } else {
      vrt_node_ = -1;
    }
  } else {
    vrt_node_ = this_node_ - nprocs_rem_;
  }

  vt_debug_print(terse, allreduce, "Rabenseifner constructor\n");
  initialize(generateNewId(), std::forward<Args>(data)...);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>void Rabenseifner<DataT, Op, ObjT, finalHandler>::initializeState(size_t id)
{
  auto& state = states_[id];

  vt_debug_print(terse, allreduce, "Rabenseifner initializing state for ID = {}\n", id);

  state.scatter_messages_.resize(num_steps_, nullptr);
  state.scatter_steps_recv_.resize(num_steps_, false);
  state.scatter_steps_reduced_.resize(num_steps_, false);

  state.gather_messages_.resize(num_steps_, nullptr);
  state.gather_steps_recv_.resize(num_steps_, false);
  state.gather_steps_reduced_.resize(num_steps_, false);

  state.finished_adjustment_part_ = not is_part_of_adjustment_group_;
  state.completed_ = false;

  state.scatter_mask_ = 1;
  state.scatter_step_ = 0;
  state.scatter_num_recv_ = 0;
  state.finished_scatter_part_ = false;

  state.gather_step_ = num_steps_ - 1;
  state.gather_mask_ = nprocs_pof2_ >> 1;
  state.gather_num_recv_ = 0;

  state.r_index_.resize(num_steps_, 0);
  state.r_count_.resize(num_steps_, 0);
  state.s_index_.resize(num_steps_, 0);
  state.s_count_.resize(num_steps_, 0);

  state.initialized_ = true;
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
template <typename ...Args>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::initialize(size_t id, Args&&... data) {
  auto& state = states_[id];
  state.val_ = DataType::toVec(std::forward<Args>(data)...);

  if(not state.initialized_){
    initializeState(id);
  }

  int step = 0;
  state.size_ = state.val_.size();
  auto size = state.size_;
  for (int mask = 1; mask < nprocs_pof2_; mask <<= 1) {
    auto vdest = vrt_node_ ^ mask;
    auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

    if (this_node_ < dest) {
      state.r_count_[step] = size / 2;
      state.s_count_[step] = size - state.r_count_[step];
      state.s_index_[step] = state.r_index_[step] + state.r_count_[step];
    } else {
      state.s_count_[step] = size / 2;
      state.r_count_[step] = size - state.s_count_[step];
      state.r_index_[step] = state.s_index_[step] + state.s_count_[step];
    }

    if (step + 1 < num_steps_) {
      state.r_index_[step + 1] = state.r_index_[step];
      state.s_index_[step + 1] = state.r_index_[step];
      size = state.r_count_[step];
      step++;
    }
  }

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner initialize: size_ = {} num_steps_ = {} nprocs_pof2_ = {} nprocs_rem_ = "
    "{} "
    "is_part_of_adjustment_group_ = {} vrt_node_ = {} ID = {}\n",
    state.size_, num_steps_, nprocs_pof2_, nprocs_rem_, is_part_of_adjustment_group_,
    vrt_node_, id
  );
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::executeFinalHan(size_t id) {
  // theCB()->makeSend<finalHandler>(parent_proxy_[this_node_]).sendTuple(std::make_tuple(val_));
  auto& state = states_.at(id);
  vt_debug_print(terse, allreduce, "Rabenseifner executing final handler ID = {}\n", id);
  parent_proxy_[this_node_].template invoke<finalHandler>(state.val_);
  state.completed_ = true;
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::allreduce(size_t id) {
  vt_debug_print(terse, allreduce, "Rabenseifner allreduce is_part_of_adjustment_group_ = {}\n", is_part_of_adjustment_group_);
  if (is_part_of_adjustment_group_) {
    adjustForPowerOfTwo(id);
  } else {
    scatterReduceIter(id);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::adjustForPowerOfTwo(size_t id) {
  if (is_part_of_adjustment_group_) {
    auto& state = states_.at(id);
    auto const partner = is_even_ ? this_node_ + 1 : this_node_ - 1;

    vt_debug_print(
      terse, allreduce, "Rabenseifner::adjustForPowerOfTwo: To Node {} ID = {}\n", partner, id
    );

    if (is_even_) {
      proxy_[partner]
        .template send<&Rabenseifner::adjustForPowerOfTwoRightHalf>(
          state.val_.data() + (state.size_ / 2), state.size_ - (state.size_ / 2), id);

      if(state.left_adjust_message_ != nullptr){
        adjustForPowerOfTwoLeftHalf(state.left_adjust_message_.get());
      }
    } else {
      proxy_[partner].template send<&Rabenseifner::adjustForPowerOfTwoLeftHalf>(
        state.val_.data(), state.size_ / 2, id);

      if(state.right_adjust_message_ != nullptr){
        adjustForPowerOfTwoRightHalf(state.right_adjust_message_.get());
      }
    }
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::adjustForPowerOfTwoRightHalf(
  AllreduceRbnRawMsg<Scalar>* msg) {

  auto& state = states_[msg->id_];

  if(not state.initialized_){
    initializeState(msg->id_);
    state.right_adjust_message_ = promoteMsg(msg);

    return;
  }

  vt_debug_print(
    terse, allreduce, "Rabenseifner::adjustForPowerOfTwoRightHalf: From Node {} ID = {}\n",
    theContext()->getFromNodeCurrentTask(), msg->id_
  );

  for (uint32_t i = 0; i < msg->size_; i++) {
    Op<Scalar>()(state.val_[(state.size_ / 2) + i], msg->val_[i]);
  }

  // Send to left node
  proxy_[theContext()->getNode() - 1]
    .template send<&Rabenseifner::adjustForPowerOfTwoFinalPart>(
      state.val_.data() + (state.size_ / 2), state.size_ - (state.size_ / 2), msg->id_);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::adjustForPowerOfTwoLeftHalf(
  AllreduceRbnRawMsg<Scalar>* msg) {

  auto& state = states_[msg->id_];
  if(not state.initialized_){
    initializeState(msg->id_);
    state.left_adjust_message_ = promoteMsg(msg);

    return;
  }

  vt_debug_print(
    terse, allreduce, "Rabenseifner::adjustForPowerOfTwoLeftHalf: From Node {} ID = {}\n",
    theContext()->getFromNodeCurrentTask(), msg->id_
  );

  for (uint32_t i = 0; i < msg->size_; i++) {
    Op<Scalar>()(state.val_[i], msg->val_[i]);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::adjustForPowerOfTwoFinalPart(
  AllreduceRbnRawMsg<Scalar>* msg) {

  vt_debug_print(
    terse, allreduce, "Rabenseifner::adjustForPowerOfTwoFinalPart: From Node {} ID = {}\n",
    theContext()->getFromNodeCurrentTask(), msg->id_
  );

  auto& state = states_[msg->id_];

  for (uint32_t i = 0; i < msg->size_; i++) {
    state.val_[(state.size_ / 2) + i] = msg->val_[i];
  }

  state.finished_adjustment_part_ = true;

  scatterReduceIter(msg->id_);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterAllMessagesReceived(size_t id) {
  auto& state = states_.at(id);

  return std::all_of(
    state.scatter_steps_recv_.cbegin(), state.scatter_steps_recv_.cbegin() + state.scatter_step_,
    [](auto const val) { return val; });
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterIsDone(size_t id) {
  auto& state = states_.at(id);
  return (state.scatter_step_ == num_steps_) and (state.scatter_num_recv_ == num_steps_);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterIsReady(size_t id) {
  auto& state = states_.at(id);
  return ((is_part_of_adjustment_group_ and state.finished_adjustment_part_) and
          state.scatter_step_ == 0) or
    ((state.scatter_mask_ < nprocs_pof2_) and scatterAllMessagesReceived(id));
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterTryReduce(
  size_t id, int32_t step) {
  auto& state = states_.at(id);
  if (
    (step < state.scatter_step_) and not state.scatter_steps_reduced_[step] and
    state.scatter_steps_recv_[step] and
    std::all_of(
      state.scatter_steps_reduced_.cbegin(), state.scatter_steps_reduced_.cbegin() + step,
      [](auto const val) { return val; })) {
    auto& in_msg = state.scatter_messages_.at(step);
    auto& in_val = in_msg->val_;
    for (uint32_t i = 0; i < in_msg->size_; i++) {
      Op<Scalar>()(state.val_[state.r_index_[in_msg->step_] + i], in_val[i]);
    }

    state.scatter_steps_reduced_[step] = true;
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterReduceIter(size_t id) {
  if (not scatterIsReady(id)) {
    return;
  }

  auto& state = states_.at(id);
  auto vdest = vrt_node_ ^ state.scatter_mask_;
  auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner Scatter (Send step {}): To Node {} starting with idx = {} and "
    "count "
    "{} ID = {}\n",
    state.scatter_step_, dest, state.s_index_[state.scatter_step_],
    state.s_count_[state.scatter_step_], id
  );

  proxy_[dest].template send<&Rabenseifner::scatterReduceIterHandler>(
    state.val_.data() + state.s_index_[state.scatter_step_], state.s_count_[state.scatter_step_], id, state.scatter_step_
  );

  state.scatter_mask_ <<= 1;
  state.scatter_step_++;

  scatterTryReduce(id, state.scatter_step_ - 1);

  if (scatterIsDone(id)) {
    state.finished_scatter_part_ = true;
    gatherIter(id);
  } else {
    scatterReduceIter(id);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::scatterReduceIterHandler(
  AllreduceRbnRawMsg<Scalar>* msg) {
  auto& state = states_[msg->id_];

  if(not state.initialized_){
    initializeState(msg->id_);
    state.scatter_messages_[msg->step_] = promoteMsg(msg);
    state.scatter_steps_recv_[msg->step_] = true;
    state.scatter_num_recv_++;

    return;
  }

  state.scatter_messages_[msg->step_] = promoteMsg(msg);
  state.scatter_steps_recv_[msg->step_] = true;
  state.scatter_num_recv_++;

  if (not state.finished_adjustment_part_) {
    return;
  }

  scatterTryReduce(msg->id_, msg->step_);

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner Scatter (Recv step {}): scatter_mask_= {} nprocs_pof2_ = {}: "
    "idx = {} from {} ID = {}\n",
    msg->step_, state.scatter_mask_, nprocs_pof2_, state.r_index_[msg->step_],
    theContext()->getFromNodeCurrentTask(), msg->id_
  );

  if ((state.scatter_mask_ < nprocs_pof2_) and scatterAllMessagesReceived(msg->id_)) {
    scatterReduceIter(msg->id_);
  } else if (scatterIsDone(msg->id_)) {
    state.finished_scatter_part_ = true;
    gatherIter(msg->id_);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherAllMessagesReceived(size_t id) {
  auto& state = states_.at(id);
  return std::all_of(
    state.gather_steps_recv_.cbegin() + state.gather_step_ + 1, state.gather_steps_recv_.cend(),
    [](auto const val) { return val; });
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherIsDone(size_t id) {
  auto& state = states_.at(id);
  return (state.gather_step_ < 0) and (state.gather_num_recv_ == num_steps_);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
bool Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherIsReady(size_t id) {
  auto& state = states_.at(id);
  return (state.gather_step_ == num_steps_ - 1) or gatherAllMessagesReceived(id);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherTryReduce(
  size_t id, int32_t step) {
  auto& state = states_.at(id);

  auto const doRed = (step > state.gather_step_) and
    not state.gather_steps_reduced_[step] and state.gather_steps_recv_[step] and
    std::all_of(state.gather_steps_reduced_.cbegin() + step + 1,
                state.gather_steps_reduced_.cend(),
                [](auto const val) { return val; });

  if (doRed) {
    auto& in_msg = state.gather_messages_.at(step);
    auto& in_val = in_msg->val_;
    for (uint32_t i = 0; i < in_msg->size_; i++) {
      state.val_[state.s_index_[in_msg->step_] + i] = in_val[i];
    }

    state.gather_steps_reduced_[step] = true;
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherIter(size_t id) {
  if (not gatherIsReady(id)) {
    return;
  }

  auto& state = states_.at(id);
  auto vdest = vrt_node_ ^ state.gather_mask_;
  auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner Gather (step {}): Sending to Node {} starting with idx = {} and "
    "count "
    "{} ID = {}\n",
    state.gather_step_, dest, state.r_index_[state.gather_step_],
    state.r_count_[state.gather_step_], id
  );

  proxy_[dest].template send<&Rabenseifner::gatherIterHandler>(
    state.val_.data() + state.r_index_[state.gather_step_], state.r_count_[state.gather_step_], id, state.gather_step_
  );

  state.gather_mask_ >>= 1;
  state.gather_step_--;

  gatherTryReduce(id, state.gather_step_ + 1);

  if (gatherIsDone(id)) {
    finalPart(id);
  } else if (gatherIsReady(id)) {
    gatherIter(id);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::gatherIterHandler(
  AllreduceRbnRawMsg<Scalar>* msg) {
  auto& state = states_.at(msg->id_);
  vt_debug_print(
    terse, allreduce, "Rabenseifner Gather (step {}): Received idx = {} from {} ID = {}\n",
    msg->step_, state.s_index_[msg->step_],
    theContext()->getFromNodeCurrentTask(), msg->id_
  );

  state.gather_messages_[msg->step_] = promoteMsg(msg);
  state.gather_steps_recv_[msg->step_] = true;
  state.gather_num_recv_++;

  if (not state.finished_scatter_part_) {
    return;
  }

  gatherTryReduce(msg->id_, msg->step_);

  if (state.gather_mask_ > 0 and gatherIsReady(msg->id_)) {
    gatherIter(msg->id_);
  } else if (gatherIsDone(msg->id_)) {
    finalPart(msg->id_);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::finalPart(size_t id) {
  auto& state = states_.at(id);
  if (state.completed_) {
    return;
  }

  if (nprocs_rem_) {
    sendToExcludedNodes(id);
  }

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner::finalPart(): Executing final handler with size {} ID = {}\n", state.val_.size(), id
  );

  parent_proxy_[this_node_].template invoke<finalHandler>(
    DataType::fromVec(state.val_)
  );

  state.completed_ = true;

  std::fill(state.scatter_messages_.begin(), state.scatter_messages_.end(), nullptr);
  std::fill(state.gather_messages_.begin(), state.gather_messages_.end(), nullptr);

  state.scatter_steps_recv_.assign(num_steps_, false);
  state.gather_steps_recv_.assign(num_steps_, false);

  state.scatter_steps_reduced_.assign(num_steps_, false);
  state.gather_steps_reduced_.assign(num_steps_, false);

  state.r_index_.assign(num_steps_, 0);
  state.r_count_.assign(num_steps_, 0);
  state.s_index_.assign(num_steps_, 0);
  state.s_count_.assign(num_steps_, 0);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::sendToExcludedNodes(size_t id) {
  auto& state = states_.at(id);
  if (is_part_of_adjustment_group_ and is_even_) {
    vt_debug_print(
      terse, allreduce, "Rabenseifner::sendToExcludedNodes(): Sending to Node {} ID = {}\n",
      this_node_ + 1, id
    );
    proxy_[this_node_ + 1]
      .template send<&Rabenseifner::sendToExcludedNodesHandler>(state.val_.data(), state.size_, id);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT, auto finalHandler
>
void Rabenseifner<DataT, Op, ObjT, finalHandler>::sendToExcludedNodesHandler(
  AllreduceRbnRawMsg<Scalar>* msg) {
  auto& state = states_.at(msg->id_);
  vt_debug_print(
    terse, allreduce,
    "Rabenseifner::sendToExcludedNodesHandler(): Received allreduce result with size {} ID = {}\n", msg->size_, msg->id_
  );

  parent_proxy_[this_node_].template invoke<finalHandler>(
    DataType::fromMemory(msg->val_, msg->size_)
  );
  state.completed_ = true;
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_IMPL_H*/
