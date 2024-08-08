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

#include "rabenseifner_group.h"
#include "vt/collective/reduce/allreduce/data_handler.h"
#include "vt/config.h"
#include "vt/context/context.h"
#include "vt/messaging/active.h"

#include <type_traits>
#include <algorithm>
#include <iterator>

namespace vt::collective::reduce::allreduce {

RabenseifnerGroup::RabenseifnerGroup(
  GroupType group, std::vector<NodeType> const& nodes) :
    group_(group),
    nodes_(nodes),
    num_nodes_(nodes.size()),
    num_steps_(static_cast<int32_t>(log2(num_nodes_))),
    nprocs_pof2_(1 << num_steps_),
    nprocs_rem_(num_nodes_ - nprocs_pof2_) {

  auto it = std::find(nodes.begin(), nodes.end(), theContext()->getNode());

  vtAssert(it != nodes.end(), "This node was not found in group nodes!");
  vtAssert(std::is_sorted(nodes.begin(), nodes.end()), "Has to be sorted!");

  // index in group list
  this_node_ = it - nodes.begin();
  is_even_ = this_node_ % 2 == 0;
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

  vt_debug_print(
    terse, allreduce,
    "RabenseifnerGroup constructor: this_node_={} vrt_node_={} "
    "is_part_of_adjustment_group_={} \n",
    this_node_, vrt_node_, is_part_of_adjustment_group_
  );
  // initialize(generateNewId(), std::forward<Args>(data)...);
}

void RabenseifnerGroup::executeFinalHan(size_t ) {
//   auto& state = states_.at(id);
//   vt_debug_print(terse, allreduce, "Rabenseifner executing final handler ID = {}\n", id);

//   // TODO convert
//   // parent_proxy_[this_node_].template invoke<finalHandler>(state.val_);
//   state.completed_ = true;
// }


// void RabenseifnerGroup::allreduce(size_t) {
//   if (is_part_of_adjustment_group_) {
//     adjustForPowerOfTwo(id);
//   } else {
//     scatterReduceIter(id);
//   }
}


void RabenseifnerGroup::adjustForPowerOfTwo(size_t ) {
  // if (is_part_of_adjustment_group_) {
  //   auto& state = states_.at(id);
  //   auto const partner = is_even_ ? this_node_ + 1 : this_node_ - 1;

  //   vt_debug_print(
  //     terse, allreduce, "Rabenseifner AdjustInitial (To {}): ID = {}\n", partner, id
  //   );

  //   if (is_even_) {
  //     theMsg()->sendMsg<&RabenseifnerGroup::adjustForPowerOfTwoRightHalf>(
  //         partner, DataHelperT::createMessage(
  //           state.val_, state.size_ / 2, state.size_ - (state.size_ / 2), id
  //         )
  //       );

  //     if(state.left_adjust_message_ != nullptr){
  //       adjustForPowerOfTwoLeftHalf(state.left_adjust_message_.get());
  //     }
  //   } else {
  //     theMsg()->sendMsg<&RabenseifnerGroup::adjustForPowerOfTwoLeftHalf>(
  //         partner, DataHelperT::createMessage(state.val_, 0, state.size_ / 2, id)
  //       );

  //     if(state.right_adjust_message_ != nullptr){
  //       adjustForPowerOfTwoRightHalf(state.right_adjust_message_.get());
  //     }
  //   }
  // }
}

bool RabenseifnerGroup::scatterAllMessagesReceived(size_t) {
  return false;
  // auto const& state = states_.at(id);

  // return std::all_of(
  //   state.scatter_steps_recv_.cbegin(), state.scatter_steps_recv_.cbegin() + state.scatter_step_,
  //   [](auto const val) { return val; });
}


bool RabenseifnerGroup::scatterIsDone(size_t) {
  return false;
  //auto const& state = states_.at(id);
  //return (state.scatter_step_ == num_steps_) and (state.scatter_num_recv_ == num_steps_);
}


bool RabenseifnerGroup::scatterIsReady(size_t) {
  return false;
  // auto const& state = states_.at(id);
  // return ((is_part_of_adjustment_group_ and state.finished_adjustment_part_) and
  //         state.scatter_step_ == 0) or
  //   ((state.scatter_mask_ < nprocs_pof2_) and scatterAllMessagesReceived(id));
}


void RabenseifnerGroup::scatterTryReduce(
  size_t, int32_t ) {
  // auto& state = states_.at(id);

  // auto do_reduce = (step < state.scatter_step_) and
  //   not state.scatter_steps_reduced_[step] and
  //   state.scatter_steps_recv_[step] and
  //   std::all_of(state.scatter_steps_reduced_.cbegin(),
  //               state.scatter_steps_reduced_.cbegin() + step,
  //               [](auto const val) { return val; });

  // vt_debug_print(
  //   verbose, allreduce, "Rabenseifner ScatterTryReduce (Step = {} ID = {}): {}\n",
  //   step, id, do_reduce
  // );

  // if (do_reduce) {
  //   auto& in_msg = state.scatter_messages_.at(step);
  //   DataHelperT::template reduce<Op>(state.val_, state.r_index_[in_msg->step_], in_msg.get());

  //   state.scatter_steps_reduced_[step] = true;
  // }
}


void RabenseifnerGroup::scatterReduceIter(size_t) {
  // if (not scatterIsReady(id)) {
  //   return;
  // }

  // auto& state = states_.at(id);
  // auto vdest = vrt_node_ ^ state.scatter_mask_;
  // auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

  // vt_debug_print(
  //   terse, allreduce,
  //   "Rabenseifner Scatter (Send step {} to {}): Starting with idx = {} and "
  //   "count "
  //   "{} ID = {}\n",
  //   state.scatter_step_, dest, state.s_index_[state.scatter_step_],
  //   state.s_count_[state.scatter_step_], id
  // );

  // theMsg()->sendMsg<&RabenseifnerGroup::scatterReduceIterHandler>(dest,
  //   DataHelperT::createMessage(state.val_, state.s_index_[state.scatter_step_], state.s_count_[state.scatter_step_], id, state.scatter_step_)
  // );

  // state.scatter_mask_ <<= 1;
  // state.scatter_step_++;

  // scatterTryReduce(id, state.scatter_step_ - 1);

  // if (scatterIsDone(id)) {
  //   state.finished_scatter_part_ = true;
  //   gatherIter(id);
  // } else {
  //   scatterReduceIter(id);
  // }
}

bool RabenseifnerGroup::gatherAllMessagesReceived(size_t) {
  return false;
  // auto& state = states_.at(id);
  // return std::all_of(
  //   state.gather_steps_recv_.cbegin() + state.gather_step_ + 1, state.gather_steps_recv_.cend(),
  //   [](auto const val) { return val; });
}


bool RabenseifnerGroup::gatherIsDone(size_t) {
  return false;
  // auto& state = states_.at(id);
  // return (state.gather_step_ < 0) and (state.gather_num_recv_ == num_steps_);
}


bool RabenseifnerGroup::gatherIsReady(size_t) {
  return false;
  // auto& state = states_.at(id);
  // return (state.gather_step_ == num_steps_ - 1) or gatherAllMessagesReceived(id);
}


void RabenseifnerGroup::gatherTryReduce(
  size_t, int32_t ) {
  // auto& state = states_.at(id);

  // auto const doRed = (step > state.gather_step_) and
  //   not state.gather_steps_reduced_[step] and state.gather_steps_recv_[step] and
  //   std::all_of(state.gather_steps_reduced_.cbegin() + step + 1,
  //               state.gather_steps_reduced_.cend(),
  //               [](auto const val) { return val; });

  // if (doRed) {
  //   auto& in_msg = state.gather_messages_.at(step);
  //   DataHelperT::copy(state.val_, state.s_index_[in_msg->step_], in_msg.get());

  //   state.gather_steps_reduced_[step] = true;
  // }
}


void RabenseifnerGroup::gatherIter(size_t) {
  // if (not gatherIsReady(id)) {
  //   return;
  // }

  // auto& state = states_.at(id);
  // auto vdest = vrt_node_ ^ state.gather_mask_;
  // auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;

  // vt_debug_print(
  //   terse, allreduce,
  //   "Rabenseifner Gather (step {}): Sending to Node {} starting with idx = {} and "
  //   "count "
  //   "{} ID = {}\n",
  //   state.gather_step_, dest, state.r_index_[state.gather_step_],
  //   state.r_count_[state.gather_step_], id
  // );

  // theMsg()->sendMsg<&RabenseifnerGroup::gatherIterHandler>(dest,
  //   DataHelperT::createMessage(
  //     state.val_, state.r_index_[state.gather_step_],
  //     state.r_count_[state.gather_step_], id, state.gather_step_
  //   )
  // );

  // state.gather_mask_ >>= 1;
  // state.gather_step_--;

  // gatherTryReduce(id, state.gather_step_ + 1);

  // if (gatherIsDone(id)) {
  //   finalPart(id);
  // } else if (gatherIsReady(id)) {
  //   gatherIter(id);
  // }
}

void RabenseifnerGroup::finalPart(size_t) {
  // auto& state = states_.at(id);
  // if (state.completed_) {
  //   return;
  // }

  // if (nprocs_rem_) {
  //   sendToExcludedNodes(id);
  // }

  // vt_debug_print(
  //   terse, allreduce,
  //   "RabenseifnerGroup::finalPart(): Executing final handler with size {} ID = {}\n",
  //   state.val_.size(), id
  // );

  // // if constexpr (ShouldUseView_v<Scalar, DataT>) {
  // //   parent_proxy_[this_node_].template invoke<finalHandler>(state.val_);
  // // } else {
  // //   parent_proxy_[this_node_].template invoke<finalHandler>(
  // //     DataType::fromVec(state.val_)
  // //   );
  // // }

  // state.completed_ = true;

  // std::fill(state.scatter_messages_.begin(), state.scatter_messages_.end(), nullptr);
  // std::fill(state.gather_messages_.begin(), state.gather_messages_.end(), nullptr);

  // state.scatter_steps_recv_.assign(num_steps_, false);
  // state.gather_steps_recv_.assign(num_steps_, false);

  // state.scatter_steps_reduced_.assign(num_steps_, false);
  // state.gather_steps_reduced_.assign(num_steps_, false);

  // state.r_index_.assign(num_steps_, 0);
  // state.r_count_.assign(num_steps_, 0);
  // state.s_index_.assign(num_steps_, 0);
  // state.s_count_.assign(num_steps_, 0);
}


void RabenseifnerGroup::sendToExcludedNodes(size_t) {
  // auto& state = states_.at(id);
  // if (is_part_of_adjustment_group_ and is_even_) {
  //   vt_debug_print(
  //     terse, allreduce, "RabenseifnerGroup::sendToExcludedNodes(): Sending to Node {} ID = {}\n",
  //     this_node_ + 1, id
  //   );

  //     theMsg()->sendMsg<&RabenseifnerGroup::sendToExcludedNodesHandler>(this_node_ + 1,
  //       DataHelperT::createMessage(state.val_, 0, state.size_, id)
  //     );
  // }
}


} // namespace vt::collective::reduce::allreduce

