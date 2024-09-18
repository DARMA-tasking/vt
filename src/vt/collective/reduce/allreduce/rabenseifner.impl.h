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
#include "vt/context/context.h"
#include "vt/configs/error/config_assert.h"
#include "vt/group/global/group_default.h"
#include "vt/group/group_manager.h"
#include "vt/group/group_info.h"
#include "vt/configs/types/types_sentinels.h"
#include "vt/registry/auto/auto_registry.h"
#include "vt/utils/fntraits/fntraits.h"
#include "vt/configs/debug/debug_print.h"
#include "vt/configs/debug/debug_printconst.h"
#include "vt/collective/reduce/allreduce/type.h"
#include "vt/collective/reduce/allreduce/state_holder.h"
#include "vt/collective/reduce/scoping/strong_types.h"
#include "vt/configs/types/types_type.h"

#include <string>
#include <type_traits>

namespace vt::collective::reduce::allreduce {

template <typename DataT, typename CallbackType>
void Rabenseifner::setFinalHandler(const CallbackType& fin, size_t id) {
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);
  state.final_handler_ = fin;
}

template <typename DataT, template <typename Arg> class Op, typename... Args>
void Rabenseifner::localReduce(size_t id, Args&&... data) {
  using DataHelperT = DataHelper<typename DataHandler<DataT>::Scalar, DataT>;
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner (this={}): local_col_wait_count_={} ID={} initialized={}\n",
    print_ptr(this), state.local_col_wait_count_, id, state.initialized_);


  if (DataHelperT::empty(state.val_)) {
    initialize<DataT>(id, std::forward<Args>(data)...);
  } else {
    DataHelper<typename DataHandler<DataT>::Scalar, DataT>::template reduce<Op>(
      state.val_, std::forward<Args>(data)...);
  }

  state.local_col_wait_count_++;
  auto const is_ready = state.local_col_wait_count_ == local_num_elems_;

  if (is_ready) {
    // Execute early in case we're the only node
    if (num_nodes_ < 2) {
      executeFinalHan<DataT>(id);
    } else {
      allreduce<DataT, Op>(id);
    }
  }
}

template <typename DataT>
void Rabenseifner::initializeState(size_t id) {
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);

  vt_debug_print(
    terse, allreduce, "Rabenseifner initializing state for ID = {} num_steps_ = {}\n", id, num_steps_);

  state.finished_adjustment_part_ = not is_part_of_adjustment_group_;
  state.completed_ = false;

  state.scatter_mask_ = 1;
  state.scatter_step_ = 0;
  state.scatter_num_recv_ = 0;
  state.finished_scatter_part_ = false;

  state.gather_step_ = num_steps_ - 1;
  state.gather_mask_ = nprocs_pof2_ >> 1;
  state.gather_num_recv_ = 0;

  state.scatter_messages_.resize(num_steps_, nullptr);
  state.scatter_steps_recv_.resize(num_steps_, false);
  state.scatter_steps_reduced_.resize(num_steps_, false);

  state.gather_messages_.resize(num_steps_, nullptr);
  state.gather_steps_recv_.resize(num_steps_, false);
  state.gather_steps_reduced_.resize(num_steps_, false);

  state.r_index_.resize(num_steps_, 0);
  state.r_count_.resize(num_steps_, 0);
  state.s_index_.resize(num_steps_, 0);
  state.s_count_.resize(num_steps_, 0);

  state.initialized_ = true;
}

template <typename DataT, typename... Args>
void Rabenseifner::initialize(size_t id, Args&&... data) {
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);

  DataHelper<typename DataHandler<DataT>::Scalar, DataT>::assign(
    state.val_, std::forward<Args>(data)...);

  if (not state.initialized_) {
    initializeState<DataT>(id);
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
    "Rabenseifner initialize: size_ = {} num_steps_ = {} nprocs_pof2_ = {} "
    "nprocs_rem_ = "
    "{} "
    "is_part_of_adjustment_group_ = {} vrt_node_ = {} ID = {} state = {}\n",
    state.size_, num_steps_, nprocs_pof2_, nprocs_rem_,
    is_part_of_adjustment_group_, vrt_node_, id, print_ptr(&state));
}

template <typename DataT>
void Rabenseifner::executeFinalHan(size_t id) {
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);

  vt_debug_print(
    terse, allreduce, "Rabenseifner executing final handler ID = {}\n", id);
  vtAssert(state.final_handler_.valid(), "Final handler is not set!");

  if constexpr (ShouldUseView_v<typename DataHandler<DataT>::Scalar, DataT>) {
    state.final_handler_.send(state.val_);
  } else {
    state.final_handler_.send(
      std::move(DataHandler<DataT>::fromVec(state.val_)));
  }

  state.completed_ = true;
  cleanupState(collection_proxy_, objgroup_proxy_, group_, id);
}

template <typename DataT, template <typename Arg> class Op>
void Rabenseifner::allreduce(size_t id) {
  if (is_part_of_adjustment_group_) {
    adjustForPowerOfTwo<DataT, Op>(id);
  } else {
    scatterReduceIter<DataT, Op>(id);
  }
}

template <typename DataT, template <typename Arg> class Op>
void Rabenseifner::adjustForPowerOfTwo(size_t id) {
  if (is_part_of_adjustment_group_) {
    using Scalar = typename DataHandler<DataT>::Scalar;
    using DataHelperT = DataHelper<Scalar, DataT>;
    auto& state = getState<RabenseifnerT, DataT>(
      collection_proxy_, objgroup_proxy_, group_, id);

    auto const partner = is_even_ ? this_node_ + 1 : this_node_ - 1;
    auto const actual_partner = nodes_[partner];

    vt_debug_print(
      terse, allreduce, "Rabenseifner AdjustInitial (To {}): ID = {}\n",
      actual_partner, id);

    if (is_even_) {
      proxy_[actual_partner]
        .template sendMsg<&Rabenseifner::adjustForPowerOfTwoRightHalf<
          DataT, Scalar, Op>>(
          DataHelperT::createMessage(
            state.val_, state.size_ / 2, state.size_ - (state.size_ / 2), id));

      if (state.left_adjust_message_ != nullptr) {
        adjustForPowerOfTwoLeftHalf<DataT, Scalar, Op>(
          state.left_adjust_message_.get());
      }
    } else {
      proxy_[actual_partner]
        .template sendMsg<&Rabenseifner::template adjustForPowerOfTwoLeftHalf<
          DataT, Scalar, Op>>(
          DataHelperT::createMessage(state.val_, 0, state.size_ / 2, id));

      if (state.right_adjust_message_ != nullptr) {
        adjustForPowerOfTwoRightHalf<DataT, Scalar, Op>(
          state.right_adjust_message_.get());
      }
    }
  }
}

template <typename DataT, typename Scalar, template <typename Arg> class Op>
void Rabenseifner::adjustForPowerOfTwoRightHalf(
  RabenseifnerMsg<Scalar, DataT>* msg) {
  using DataHelperT = DataHelper<Scalar, DataT>;
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, msg->id_);

  if (DataHelperT::empty(state.val_)) {
    if (not state.initialized_) {
      vt_debug_print(
        verbose, allreduce,
        "Rabenseifner AdjustRightHalf (From {}): State not initialized ID "
        "{}!\n",
        theContext()->getFromNodeCurrentTask(), msg->id_);

      initializeState<DataT>(msg->id_);
    }
    state.right_adjust_message_ = promoteMsg(msg);

    return;
  }

  vt_debug_print(
    terse, allreduce, "Rabenseifner AdjustRightHalf (From {}): ID = {}\n",
    theContext()->getFromNodeCurrentTask(), msg->id_);

  DataHelperT::template reduceMsg<Op>(state.val_, state.size_ / 2, msg);

  // Send to left node
  auto const actual_partner = nodes_[this_node_ - 1];
  proxy_[actual_partner]
    .template sendMsg<
      &Rabenseifner::template adjustForPowerOfTwoFinalPart<DataT, Scalar, Op>>(
      DataHelperT::createMessage(
        state.val_, state.size_ / 2, state.size_ - (state.size_ / 2),
        msg->id_));
}

template <typename DataT, typename Scalar, template <typename Arg> class Op>
void Rabenseifner::adjustForPowerOfTwoLeftHalf(
  RabenseifnerMsg<Scalar, DataT>* msg) {
  using DataHelperT = DataHelper<typename DataHandler<DataT>::Scalar, DataT>;
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, msg->id_);
  if (DataHelperT::empty(state.val_)) {
    if (not state.initialized_) {
      vt_debug_print(
        verbose, allreduce,
        "Rabenseifner AdjustLeftHalf (From {}): State not initialized ID {}!\n",
        theContext()->getFromNodeCurrentTask(), msg->id_);

      initializeState<DataT>(msg->id_);
    }
    state.left_adjust_message_ = promoteMsg(msg);

    return;
  }

  vt_debug_print(
    terse, allreduce, "Rabenseifner AdjustLeftHalf (From {}): ID = {}\n",
    theContext()->getFromNodeCurrentTask(), msg->id_);

  DataHelperT::template reduceMsg<Op>(state.val_, 0, msg);
}

template <typename DataT, typename Scalar, template <typename Arg> class Op>
void Rabenseifner::adjustForPowerOfTwoFinalPart(
  RabenseifnerMsg<Scalar, DataT>* msg) {
  vt_debug_print(
    terse, allreduce, "Rabenseifner AdjustFinal (From {}): ID = {}\n",
    theContext()->getFromNodeCurrentTask(), msg->id_);

  using DataHelperT = DataHelper<typename DataHandler<DataT>::Scalar, DataT>;
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, msg->id_);

  DataHelperT::copy(state.val_, state.size_ / 2, msg);

  state.finished_adjustment_part_ = true;

  scatterReduceIter<DataT, Op>(msg->id_);
}

template <typename DataT>
bool Rabenseifner::scatterAllMessagesReceived(size_t id) {
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);

  return std::all_of(
    state.scatter_steps_recv_.cbegin(),
    state.scatter_steps_recv_.cbegin() + state.scatter_step_,
    [](auto const val) { return val; });
}

template <typename DataT>
bool Rabenseifner::scatterIsDone(size_t id) {
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);
  return (state.scatter_step_ == num_steps_) and
    (state.scatter_num_recv_ == num_steps_);
}

template <typename DataT>
bool Rabenseifner::scatterIsReady(size_t id) {
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);
  return ((is_part_of_adjustment_group_ and state.finished_adjustment_part_) and
          state.scatter_step_ == 0) or
    ((state.scatter_mask_ < nprocs_pof2_) and
     scatterAllMessagesReceived<DataT>(id));
}

template <typename DataT, template <typename Arg> class Op>
void Rabenseifner::scatterTryReduce(size_t id, int32_t step) {
  using DataHelperT = DataHelper<typename DataHandler<DataT>::Scalar, DataT>;
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);

  auto do_reduce = (step < state.scatter_step_) and
    not state.scatter_steps_reduced_[step] and
    state.scatter_steps_recv_[step] and
    std::all_of(state.scatter_steps_reduced_.cbegin(),
                state.scatter_steps_reduced_.cbegin() + step,
                [](auto const val) { return val; });

  vt_debug_print(
    verbose, allreduce,
    "Rabenseifner ScatterTryReduce (Step = {} ID = {}): {}\n", step, id,
    do_reduce);

  if (do_reduce) {
    auto& in_msg = state.scatter_messages_.at(step);
    DataHelperT::template reduceMsg<Op>(
      state.val_, state.r_index_[in_msg->step_], in_msg.get());

    state.scatter_steps_reduced_[step] = true;
  }
}

template <typename DataT, template <typename Arg> class Op>
void Rabenseifner::scatterReduceIter(size_t id) {
  if (not scatterIsReady<DataT>(id)) {
    return;
  }

  using Scalar = typename DataHandler<DataT>::Scalar;
  using DataHelperT = DataHelper<Scalar, DataT>;
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);
  vt_debug_print(
    terse, allreduce,
    "Rabenseifner Scatter (Send step {}): s_index_.size() = {} and "
    "s_count_.size() = {} ID = {} proxy_={} state = {}\n",
    state.scatter_step_, state.s_index_.size(), state.s_count_.size(), id,
    proxy_.getProxy(), print_ptr(&state));

  auto vdest = vrt_node_ ^ state.scatter_mask_;
  auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;
  auto const actual_partner = nodes_[dest];

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner Scatter (Send step {} to {}): Starting with idx = {} and "
    "count "
    "{} ID = {} proxy_={}\n",
    state.scatter_step_, actual_partner, state.s_index_[state.scatter_step_],
    state.s_count_[state.scatter_step_], id, proxy_.getProxy());

  proxy_[actual_partner]
    .template sendMsg<
      &Rabenseifner::template scatterReduceIterHandler<DataT, Scalar, Op>>(
      DataHelperT::createMessage(
        state.val_, state.s_index_[state.scatter_step_],
        state.s_count_[state.scatter_step_], id, state.scatter_step_));

  state.scatter_mask_ <<= 1;
  state.scatter_step_++;

  scatterTryReduce<DataT, Op>(id, state.scatter_step_ - 1);

  if (scatterIsDone<DataT>(id)) {
    state.finished_scatter_part_ = true;
    gatherIter<DataT>(id);
  } else {
    scatterReduceIter<DataT, Op>(id);
  }
}

template <typename DataT, typename Scalar, template <typename Arg> class Op>
void Rabenseifner::scatterReduceIterHandler(
  RabenseifnerMsg<Scalar, DataT>* msg) {
  using DataHelperT = DataHelper<typename DataHandler<DataT>::Scalar, DataT>;
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, msg->id_);

  if (DataHelperT::empty(state.val_)) {
    if (not state.initialized_) {
      vt_debug_print(
        verbose, allreduce,
        "Rabenseifner Scatter (Recv step {} from {}): State not initialized "
        "for ID = "
        "{}!\n",
        msg->step_, theContext()->getFromNodeCurrentTask(), msg->id_);
      initializeState<DataT>(msg->id_);
    }

    state.scatter_messages_[msg->step_] = promoteMsg(msg);
    state.scatter_steps_recv_[msg->step_] = true;
    state.scatter_num_recv_++;

    return;
  }

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner Scatter (Recv step {} from {}): initialized = {} "
    "scatter_mask_= {} nprocs_pof2_ = {}: scatterAllMessagesReceived() = {} "
    "state.finished_adjustment_part_ = {} "
    "idx = {} ID = {}\n",
    msg->step_, theContext()->getFromNodeCurrentTask(), state.initialized_,
    state.scatter_mask_, nprocs_pof2_,
    scatterAllMessagesReceived<DataT>(msg->id_),
    state.finished_adjustment_part_, state.r_index_[msg->step_], msg->id_);

  state.scatter_messages_[msg->step_] = promoteMsg(msg);
  state.scatter_steps_recv_[msg->step_] = true;
  state.scatter_num_recv_++;

  if (not state.finished_adjustment_part_) {
    return;
  }

  scatterTryReduce<DataT, Op>(msg->id_, msg->step_);

  if (
    (state.scatter_mask_ < nprocs_pof2_) and
    scatterAllMessagesReceived<DataT>(msg->id_)) {
    scatterReduceIter<DataT, Op>(msg->id_);
  } else if (scatterIsDone<DataT>(msg->id_)) {
    state.finished_scatter_part_ = true;
    gatherIter<DataT>(msg->id_);
  }
}

template <typename DataT>
bool Rabenseifner::gatherAllMessagesReceived(size_t id) {
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);
  return std::all_of(
    state.gather_steps_recv_.cbegin() + state.gather_step_ + 1,
    state.gather_steps_recv_.cend(), [](auto const val) { return val; });
}

template <typename DataT>
bool Rabenseifner::gatherIsDone(size_t id) {
  //auto& state = states_.at(id);
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);
  return (state.gather_step_ < 0) and (state.gather_num_recv_ == num_steps_);
}

template <typename DataT>
bool Rabenseifner::gatherIsReady(size_t id) {
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);
  return (state.gather_step_ == num_steps_ - 1) or
    gatherAllMessagesReceived<DataT>(id);
}

template <typename DataT>
void Rabenseifner::gatherTryReduce(size_t id, int32_t step) {
  using DataHelperT = DataHelper<typename DataHandler<DataT>::Scalar, DataT>;
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);

  auto const doRed = (step > state.gather_step_) and
    not state.gather_steps_reduced_[step] and state.gather_steps_recv_[step] and
    std::all_of(state.gather_steps_reduced_.cbegin() + step + 1,
                state.gather_steps_reduced_.cend(),
                [](auto const val) { return val; });

  if (doRed) {
    auto& in_msg = state.gather_messages_.at(step);
    DataHelperT::copy(state.val_, state.s_index_[in_msg->step_], in_msg.get());

    state.gather_steps_reduced_[step] = true;
  }
}

template <typename DataT>
void Rabenseifner::gatherIter(size_t id) {
  if (not gatherIsReady<DataT>(id)) {
    return;
  }

  using Scalar = typename DataHandler<DataT>::Scalar;
  using DataHelperT = DataHelper<Scalar, DataT>;
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);
  auto vdest = vrt_node_ ^ state.gather_mask_;
  auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;
  auto const actual_partner = nodes_[dest];

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner Gather (step {}): Sending to Node {} starting with idx = {} "
    "and "
    "count "
    "{} ID = {}\n",
    state.gather_step_, actual_partner, state.r_index_[state.gather_step_],
    state.r_count_[state.gather_step_], id);

  proxy_[actual_partner]
    .template sendMsg<&Rabenseifner::template gatherIterHandler<DataT, Scalar>>(
      DataHelperT::createMessage(
        state.val_, state.r_index_[state.gather_step_],
        state.r_count_[state.gather_step_], id, state.gather_step_));

  state.gather_mask_ >>= 1;
  state.gather_step_--;

  gatherTryReduce<DataT>(id, state.gather_step_ + 1);

  if (gatherIsDone<DataT>(id)) {
    finalPart<DataT>(id);
  } else if (gatherIsReady<DataT>(id)) {
    gatherIter<DataT>(id);
  }
}

template <typename DataT, typename Scalar>
void Rabenseifner::gatherIterHandler(RabenseifnerMsg<Scalar, DataT>* msg) {
  //auto& state = states_.at(msg->id_);
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, msg->id_);
  vt_debug_print(
    terse, allreduce,
    "Rabenseifner Gather (Recv step {} from {}): idx = {} ID = {}\n",
    msg->step_, theContext()->getFromNodeCurrentTask(),
    state.s_index_[msg->step_], msg->id_);

  state.gather_messages_[msg->step_] = promoteMsg(msg);
  state.gather_steps_recv_[msg->step_] = true;
  state.gather_num_recv_++;

  if (not state.finished_scatter_part_) {
    return;
  }

  gatherTryReduce<DataT>(msg->id_, msg->step_);

  if (state.gather_mask_ > 0 and gatherIsReady<DataT>(msg->id_)) {
    gatherIter<DataT>(msg->id_);
  } else if (gatherIsDone<DataT>(msg->id_)) {
    finalPart<DataT>(msg->id_);
  }
}

template <typename DataT>
void Rabenseifner::finalPart(size_t id) {
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);
  if (state.completed_) {
    return;
  }

  if (nprocs_rem_) {
    sendToExcludedNodes<DataT>(id);
  }

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner::finalPart(): Executing final handler with size {} ID = {}\n",
    state.val_.size(), id);

  executeFinalHan<DataT>(id);
}

template <typename DataT>
void Rabenseifner::sendToExcludedNodes(size_t id) {
  using Scalar = typename DataHandler<DataT>::Scalar;
  using DataHelperT = DataHelper<Scalar, DataT>;
  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, id);
  if (is_part_of_adjustment_group_ and is_even_) {
    auto const actual_partner = nodes_[this_node_ + 1];
    vt_debug_print(
      terse, allreduce,
      "Rabenseifner::sendToExcludedNodes(): Sending to Node {} ID = {} size={}\n",
      actual_partner, id, state.size_);

    proxy_[actual_partner]
      .template sendMsg<
        &Rabenseifner::template sendToExcludedNodesHandler<DataT, Scalar>>(
        DataHelperT::createMessage(state.val_, 0, state.size_, id));
  }
}

template <typename DataT, typename Scalar>
void Rabenseifner::sendToExcludedNodesHandler(
  RabenseifnerMsg<Scalar, DataT>* msg) {

  vt_debug_print(
    terse, allreduce,
    "Rabenseifner::sendToExcludedNodesHandler(): Received allreduce result "
    "with ID = {}\n",
    msg->id_);

  auto& state = getState<RabenseifnerT, DataT>(
    collection_proxy_, objgroup_proxy_, group_, msg->id_
  );
  if constexpr (ShouldUseView_v<typename DataHandler<DataT>::Scalar, DataT>) {
    state.val_ = msg->val_;
  } else {
    DataHelper<typename DataHandler<DataT>::Scalar, DataT>::assignFromMem(
      state.val_, msg->val_, msg->size_
    );
  }

  executeFinalHan<DataT>(msg->id_);
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_IMPL_H*/
