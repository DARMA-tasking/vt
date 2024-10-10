/*
//@HEADER
// *****************************************************************************
//
//                          recursive_doubling.impl.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_IMPL_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_IMPL_H

#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/collective/reduce/allreduce/recursive_doubling.h"
#include "vt/collective/reduce/allreduce/state_holder.h"
#include "vt/collective/reduce/allreduce/allreduce_holder.h"
#include "vt/collective/reduce/allreduce/data_handler.h"
#include "vt/collective/reduce/allreduce/type.h"
#include "vt/messaging/active.h"
#include "vt/topos/location/location.impl.h"

namespace vt::collective::reduce::allreduce {

template <typename DataT, typename CallbackType>
void RecursiveDoubling::setFinalHandler(const CallbackType& fin, size_t id) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
  state.final_handler_ = fin;
}

template <typename DataT, template <typename Arg> class Op, typename... Args>
void RecursiveDoubling::storeData(size_t id, Args&&... data) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);

  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling::storeData (this={}): local_col_wait_count_={} ID={} "
    "initialized={}\n",
    print_ptr(this), state.local_col_wait_count_, id, state.initialized_);

  if (not state.value_assigned_) {
    initialize<DataT>(id, std::forward<Args>(data)...);
  } else {
    Op<DataT>()(state.val_, std::forward<Args>(data)...);
  }

  state.local_col_wait_count_++;
}

template <typename DataT, template <typename Arg> class Op>
void RecursiveDoubling::run(size_t id) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
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

template <typename DataT, typename... Args>
void RecursiveDoubling::initialize(size_t id, Args&&... data) {
  using DataType = DataHandler<DataT>;
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);

  if (not state.initialized_) {
    initializeState<DataT>(id);
  }

  state.val_ = DataT{std::forward<Args>(data)...};
  state.value_assigned_ = true;
  state.active_ = true;

  vt_debug_print(
    terse, allreduce, "RecursiveDoubling Initialize: size {} ID {}\n",
    DataType::size(state.val_), id);
}

template <typename DataT>
void RecursiveDoubling::initializeState(size_t id) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);

  vt_debug_print(
    terse, allreduce, "RecursiveDoubling initializing state for ID = {}\n", id);

  state.messages_.resize(num_steps_, nullptr);
  state.steps_recv_.resize(num_steps_, false);
  state.steps_reduced_.resize(num_steps_, false);

  state.completed_ = false;
  state.step_ = 0;
  state.mask_ = 1;
  state.finished_adjustment_part_ = not is_part_of_adjustment_group_;
  state.initialized_ = true;
}

template <typename DataT, template <typename Arg> class Op>
void RecursiveDoubling::allreduce(size_t id) {
  if (is_part_of_adjustment_group_) {
    adjustForPowerOfTwo<DataT, Op>(id);
  } else {
    reduceIter<DataT, Op>(id);
  }
}

template <typename DataT, template <typename Arg> class Op>
void RecursiveDoubling::adjustForPowerOfTwo(size_t id) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
  if (is_part_of_adjustment_group_ and not is_even_) {
    vt_debug_print(
      terse, allreduce, "RecursiveDoubling AdjustInitial (To {}): ID = {}  \n",
      this_node_, this_node_ - 1, id);

    auto const partner = Node{static_cast<NodeType>(this_node_ - NodeType{1})};
    theMsg()
      ->template sendMsg<
        RecursiveDoubling::template adjustForPowerOfTwoHandler<DataT, Op>>(
        partner.get(),
        vt::makeMessage<RecursiveDoublingMsg<DataT>>(info_, state.val_, id));
  } else if (state.adjust_message_ != nullptr) {
    // We have pending reduce message
    adjustForPowerOfTwoHan<DataT, Op>(state.adjust_message_.get());
  }
}

template <typename DataT, template <typename Arg> class Op>
/*static*/ void RecursiveDoubling::adjustForPowerOfTwoHandler(
  RecursiveDoublingMsg<DataT>* msg) {
  auto* reducer = getAllreducer<RecursiveDoublingT>(msg->info_);

  if(reducer){
    reducer->template adjustForPowerOfTwoHan<DataT, Op>(msg);
  }else{
    auto& state = getState<RecursiveDoublingT, DataT>(msg->info_, msg->id_);
    state.adjust_message_ = promoteMsg(msg);
  }
}

template <typename DataT, template <typename Arg> class Op>
void RecursiveDoubling::adjustForPowerOfTwoHan(
  RecursiveDoublingMsg<DataT>* msg) {
  using DataType = DataHandler<DataT>;
  auto& state = getState<RecursiveDoublingT, DataT>(info_, msg->id_);
  if (not state.value_assigned_) {
    if (not state.initialized_) {
      initializeState<DataT>(msg->id_);
    }
    state.adjust_message_ = promoteMsg(msg);

    return;
  }

  Op<DataT>()(state.val_, *(msg->val_));

  state.finished_adjustment_part_ = true;

  reduceIter<DataT, Op>(msg->id_);
}

template <typename DataT>
bool RecursiveDoubling::isDone(size_t id) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
  return (state.step_ == num_steps_) and allMessagesReceived<DataT>(id);
}

template <typename DataT>
bool RecursiveDoubling::isValid(size_t id) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
  return (vrt_node_ != -1) and (state.step_ < num_steps_);
}

template <typename DataT>
bool RecursiveDoubling::allMessagesReceived(size_t id) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
  return std::all_of(
    state.steps_recv_.cbegin(), state.steps_recv_.cbegin() + state.step_,
    [](const auto val) { return val; });
}

template <typename DataT>
bool RecursiveDoubling::isReady(size_t id) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
  return ((is_part_of_adjustment_group_ and state.finished_adjustment_part_) and
          state.step_ == 0) or
    allMessagesReceived<DataT>(id);
}

template <typename DataT, template <typename Arg> class Op>
void RecursiveDoubling::reduceIter(size_t id) {
  // Ensure we have received all necessary messages
  if (not isReady<DataT>(id)) {
    return;
  }

  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
  auto vdest = vrt_node_ ^ state.mask_;
  auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;
  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling Part2 (Send step {}): To Node {} ID = {} \n",
    state.step_, dest, id);

  auto const partner = static_cast<NodeType>(dest);
  theMsg()
    ->sendMsg<
      reduceIterHandler<DataT, Op>>(
      partner,
      vt::makeMessage<RecursiveDoublingMsg<DataT>>(
        info_, state.val_, id, state.step_));

  state.mask_ <<= 1;
  state.step_++;

  tryReduce<DataT, Op>(id, state.step_ - 1);

  if (isDone<DataT>(id)) {
    finalPart<DataT>(id);
  } else if (isReady<DataT>(id)) {
    reduceIter<DataT, Op>(id);
  }
}

template <typename DataT, template <typename Arg> class Op>
void RecursiveDoubling::tryReduce(size_t id, int32_t step) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
  auto const all_msgs_received = std::all_of(
    state.steps_reduced_.cbegin(), state.steps_reduced_.cbegin() + step,
    [](const auto val) { return val; });

  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling Part2 (Reduce step {}): state.step_ = {} "
    "state.steps_reduced_[step] = {} state.steps_recv_[step] = {} "
    "all_msgs_received = {} ID = {} \n",
    step, state.step_, static_cast<bool>(state.steps_reduced_[step]),
    static_cast<bool>(state.steps_recv_[step]), all_msgs_received, id);

  if (
    (step < state.step_) and not state.steps_reduced_[step] and
    state.steps_recv_[step] and all_msgs_received) {
    Op<DataT>()(state.val_, *(state.messages_.at(step)->val_));

    state.steps_reduced_[step] = true;
  }
}

template <typename DataT, template <typename Arg> class Op>
/*static*/ void
RecursiveDoubling::reduceIterHandler(RecursiveDoublingMsg<DataT>* msg) {
  auto* reducer = getAllreducer<RecursiveDoublingT>(msg->info_);

  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling::reduceIterHandler reducer={} ID={} \n",
    (reducer != nullptr), msg->id_
  );

  if(reducer){
    reducer->template reduceIterHan<DataT, Op>(msg);
  }else{
    auto& state = getState<RecursiveDoublingT, DataT>(msg->info_, msg->id_);
    if(state.messages_.size() < static_cast<size_t>(msg->step_ + 1)){
      state.messages_.resize(msg->step_ + 1, nullptr);
    }
    state.messages_.at(msg->step_) = promoteMsg(msg);

    if(state.steps_recv_.size() < static_cast<size_t>(msg->step_ + 1)){
      state.steps_recv_.resize(msg->step_ + 1, false);
    }
    state.steps_recv_[msg->step_] = true;
  }
}

template <typename DataT, template <typename Arg> class Op>
void RecursiveDoubling::reduceIterHan(RecursiveDoublingMsg<DataT>* msg) {
  using DataType = DataHandler<DataT>;
  auto& state = getState<RecursiveDoublingT, DataT>(info_, msg->id_);

  if (not state.value_assigned_) {
    if (not state.initialized_) {
      initializeState<DataT>(msg->id_);
    }
    state.messages_.at(msg->step_) = promoteMsg(msg);
    state.steps_recv_[msg->step_] = true;

    return;
  }

  state.messages_.at(msg->step_) = promoteMsg(msg);
  state.steps_recv_[msg->step_] = true;

  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling Part2 (Recv step {}): finished_adjustment_part_ = {} "
    "mask_= {} nprocs_pof2_ = {} "
    "from {} ID = {}\n",
    msg->step_, state.finished_adjustment_part_, state.mask_, nprocs_pof2_,
    theContext()->getFromNodeCurrentTask(), msg->id_);

  // Special case when we receive step 2 message before step 1 is done on this node
  if (not state.finished_adjustment_part_) {
    return;
  }

  tryReduce<DataT, Op>(msg->id_, msg->step_);

  if ((state.mask_ < nprocs_pof2_) and isReady<DataT>(msg->id_)) {
    reduceIter<DataT, Op>(msg->id_);

  } else if (isDone<DataT>(msg->id_)) {
    finalPart<DataT>(msg->id_);
  }
}

template <typename DataT>
void RecursiveDoubling::sendToExcludedNodes(size_t id) {
  if (is_part_of_adjustment_group_ and is_even_) {
    vt_debug_print(
      terse, allreduce, "RecursiveDoubling Part3: Sending to Node {} ID = {}\n",
      this_node_ + 1, id);

    auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
    auto const partner = static_cast<NodeType>(this_node_ + 1);
    theMsg()
      ->sendMsg<
        sendToExcludedNodesHandler<DataT>>(
        partner,
        vt::makeMessage<RecursiveDoublingMsg<DataT>>(info_, state.val_, id));
  }
}

template <typename DataT>
/*static*/ void RecursiveDoubling::sendToExcludedNodesHandler(
  RecursiveDoublingMsg<DataT>* msg) {
  getAllreducer<RecursiveDoublingT>(msg->info_)
    ->template sendToExcludedNodesHan<DataT>(msg);
}

template <typename DataT>
void RecursiveDoubling::sendToExcludedNodesHan(
  RecursiveDoublingMsg<DataT>* msg) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, msg->id_);
  state.val_ = *msg->val_;
  executeFinalHan<DataT>(msg->id_);
}

template <typename DataT>
void RecursiveDoubling::finalPart(size_t id) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
  if (state.completed_) {
    return;
  }

  if (nprocs_rem_) {
    sendToExcludedNodes<DataT>(id);
  }

  executeFinalHan<DataT>(id);
}

template <typename DataT>
void RecursiveDoubling::executeFinalHan(size_t id) {
  auto& state = getState<RecursiveDoublingT, DataT>(info_, id);
  vt_debug_print(
    terse, allreduce, "RecursiveDoubling executing final handler ID = {}\n",
    id);

  state.final_handler_.send(std::move(state.val_));

  state.completed_ = true;
  cleanupState(info_, id);
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_IMPL_H*/
