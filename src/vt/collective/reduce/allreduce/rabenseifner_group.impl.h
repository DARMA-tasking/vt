/*
//@HEADER
// *****************************************************************************
//
//                          rabenseifner_group.impl.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_GROUP_IMPL_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_GROUP_IMPL_H

#include "vt/collective/reduce/allreduce/data_handler.h"
#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"

#include <type_traits>
#include <algorithm>
#include <iterator>

namespace vt::collective::reduce::allreduce {

template <auto f, template <typename Arg> class Op, typename ...Args>
void RabenseifnerGroup::initialize(size_t id, Args&&... data) {
  using DataT = typename function_traits<decltype(f)>::template arg_type<0>;
  using ScalarT = typename DataHandler<DataT>::Scalar;

  using StateT = StateHolder<ScalarT, DataT, Op>;
  state_holder_[id] = std::make_unique<StateT>();

  auto& state = dynamic_cast<StateT*>(state_holder_[id].get())->state_;

  DataHelper<ScalarT, DataT>::assign(state.val_, std::forward<Args>(data)...);

  if(not state.initialized_){
    initializeState<DataT, Op>(id);
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
    "is_part_of_adjustment_group_ = {} vrt_node_ = {} ID = {:x}\n",
    state.size_, num_steps_, nprocs_pof2_, nprocs_rem_, is_part_of_adjustment_group_,
    vrt_node_, id
  );
}

template <typename DataT, template <typename Arg> class Op>
void RabenseifnerGroup::initializeState(size_t id) {
  using ScalarT = typename DataHandler<DataT>::Scalar;
  using StateT = StateHolder<ScalarT, DataT, Op>;
  auto it = state_holder_.find(id);

  if(it == state_holder_.end()){
    state_holder_[id] = std::make_unique<StateT>();
  }

  auto& state = dynamic_cast<StateT*>(state_holder_.at(id).get())->state_;
  vt_debug_print(terse, allreduce, "Rabenseifner initializing state for ID = {:x}\n", id);

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

template <auto f, template <typename Arg> class Op, typename ...Args>
void RabenseifnerGroup::allreduce(size_t id, Args &&... args){
  using DataT = typename function_traits<decltype(f)>::template arg_type<0>;
  using ScalarT = typename DataHandler<DataT>::Scalar;
  using StateT = StateHolder<ScalarT, DataT, Op>;
  initialize<f, Op>(id, std::forward<Args>(args)...);

  auto& state = dynamic_cast<StateT*>(state_holder_.at(id).get())->state_;

  if (is_part_of_adjustment_group_) {
    adjustForPowerOfTwo(id);
  } else {
    scatterReduceIter(id);
  }
}

template<typename Scalar, typename DataT>
void RabenseifnerGroup::adjustForPowerOfTwoRightHalf(
  RabenseifnerMsg<Scalar, DataT>* msg) {

  // auto& state = states_[msg->id_];

  // if (DataHelperT::empty(state.val_)) {
  //   if (not state.initialized_) {
  //     vt_debug_print(
  //       verbose, allreduce,
  //       "Rabenseifner AdjustRightHalf (From {}): State not initialized ID {}!\n",
  //       theContext()->getFromNodeCurrentTask(), msg->id_
  //     );

  //     initializeState(msg->id_);
  //   }
  //   state.right_adjust_message_ = promoteMsg(msg);

  //   return;
  // }

  // vt_debug_print(
  //   terse, allreduce, "Rabenseifner AdjustRightHalf (From {}): ID = {}\n",
  //   theContext()->getFromNodeCurrentTask(), msg->id_
  // );

  // DataHelperT::template reduce<Op>(state.val_, state.size_ / 2, msg);

  // // Send to left node
  // theMsg()->sendMsg<&RabenseifnerGroup::adjustForPowerOfTwoFinalPart>(theContext()->getNode() - 1,
  //     DataHelperT::createMessage(state.val_, state.size_ / 2, state.size_ - (state.size_ / 2), msg->id_)
  //   );
}

template<typename Scalar, typename DataT>
void RabenseifnerGroup::adjustForPowerOfTwoLeftHalf(
  RabenseifnerMsg<Scalar, DataT>* msg) {

  // auto& state = states_[msg->id_];
  // if (DataHelperT::empty(state.val_)) {
  //   if (not state.initialized_) {
  //     vt_debug_print(
  //       verbose, allreduce,
  //       "Rabenseifner AdjustLeftHalf (From {}): State not initialized ID {}!\n",
  //       theContext()->getFromNodeCurrentTask(), msg->id_);

  //     initializeState(msg->id_);
  //   }
  //   state.left_adjust_message_ = promoteMsg(msg);

  //   return;
  // }

  // vt_debug_print(
  //   terse, allreduce, "Rabenseifner AdjustLeftHalf (From {}): ID = {}\n",
  //   theContext()->getFromNodeCurrentTask(), msg->id_
  // );

  // DataHelperT::template reduce<Op>(state.val_, 0, msg);
}

template<typename Scalar, typename DataT>
void RabenseifnerGroup::adjustForPowerOfTwoFinalPart(
  RabenseifnerMsg<Scalar, DataT>* msg) {

  // vt_debug_print(
  //   terse, allreduce, "Rabenseifner AdjustFinal (From {}): ID = {}\n",
  //   theContext()->getFromNodeCurrentTask(), msg->id_
  // );

  // auto& state = states_[msg->id_];

  // DataHelperT::copy(state.val_, state.size_ / 2, msg);

  // state.finished_adjustment_part_ = true;

  // scatterReduceIter(msg->id_);
}


template<typename Scalar, typename DataT>
void RabenseifnerGroup::scatterReduceIterHandler(
  RabenseifnerMsg<Scalar, DataT>* msg) {
  // auto& state = states_[msg->id_];

  // if (DataHelperT::empty(state.val_)) {
  //   if (not state.initialized_) {
  //     vt_debug_print(
  //       verbose, allreduce,
  //       "Rabenseifner Scatter (Recv step {} from {}): State not initialized "
  //       "for ID = "
  //       "{}!\n",
  //       msg->step_, theContext()->getFromNodeCurrentTask(), msg->id_);
  //     initializeState(msg->id_);
  //   }

  //   state.scatter_messages_[msg->step_] = promoteMsg(msg);
  //   state.scatter_steps_recv_[msg->step_] = true;
  //   state.scatter_num_recv_++;

  //   return;
  // }

  // vt_debug_print(
  //   terse, allreduce,
  //   "Rabenseifner Scatter (Recv step {} from {}): initialized = {} "
  //   "scatter_mask_= {} nprocs_pof2_ = {}: scatterAllMessagesReceived() = {} "
  //   "state.finished_adjustment_part_ = {}"
  //   "idx = {} ID = {}\n",
  //   msg->step_, theContext()->getFromNodeCurrentTask(), state.initialized_,
  //   state.scatter_mask_, nprocs_pof2_, scatterAllMessagesReceived(msg->id_),
  //   state.finished_adjustment_part_, state.r_index_[msg->step_], msg->id_
  // );

  // state.scatter_messages_[msg->step_] = promoteMsg(msg);
  // state.scatter_steps_recv_[msg->step_] = true;
  // state.scatter_num_recv_++;

  // if (not state.finished_adjustment_part_) {
  //   return;
  // }

  // scatterTryReduce(msg->id_, msg->step_);

  // if ((state.scatter_mask_ < nprocs_pof2_) and scatterAllMessagesReceived(msg->id_)) {
  //   scatterReduceIter(msg->id_);
  // } else if (scatterIsDone(msg->id_)) {
  //   state.finished_scatter_part_ = true;
  //   gatherIter(msg->id_);
  // }
}

template<typename Scalar, typename DataT>
void RabenseifnerGroup::gatherIterHandler(
  RabenseifnerMsg<Scalar, DataT>* msg) {
  // auto& state = states_.at(msg->id_);
  // vt_debug_print(
  //   terse, allreduce, "Rabenseifner Gather (Recv step {} from {}): idx = {} ID = {}\n",
  //   msg->step_, theContext()->getFromNodeCurrentTask(), state.s_index_[msg->step_],
  //   msg->id_
  // );

  // state.gather_messages_[msg->step_] = promoteMsg(msg);
  // state.gather_steps_recv_[msg->step_] = true;
  // state.gather_num_recv_++;

  // if (not state.finished_scatter_part_) {
  //   return;
  // }

  // gatherTryReduce(msg->id_, msg->step_);

  // if (state.gather_mask_ > 0 and gatherIsReady(msg->id_)) {
  //   gatherIter(msg->id_);
  // } else if (gatherIsDone(msg->id_)) {
  //   finalPart(msg->id_);
  // }
}

template<typename Scalar, typename DataT>
void RabenseifnerGroup::sendToExcludedNodesHandler(
  RabenseifnerMsg<Scalar, DataT>* msg) {
  // auto& state = states_.at(msg->id_);
  // vt_debug_print(
  //   terse, allreduce,
  //   "RabenseifnerGroup::sendToExcludedNodesHandler(): Received allreduce result "
  //   "with ID = {}\n",
  //   msg->id_
  // );

  // // if constexpr (ShouldUseView_v<Scalar, DataT>) {
  // //   parent_proxy_[this_node_].template invoke<finalHandler>(msg->val_);
  // // } else {
  // //   parent_proxy_[this_node_].template invoke<finalHandler>(
  // //     DataType::fromMemory(msg->val_, msg->size_));
  // // }

  // state.completed_ = true;
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RABENSEIFNER_GROUP_IMPL_H*/
