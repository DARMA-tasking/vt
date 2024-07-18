/*
//@HEADER
// *****************************************************************************
//
//                          recursive_doubling.impl.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_IMPL_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_IMPL_H

#include "vt/config.h"

namespace vt::collective::reduce::allreduce {

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
template <typename... Args>
RecursiveDoubling<DataT, Op, ObjT, finalHandler>::RecursiveDoubling(
  vt::objgroup::proxy::Proxy<ObjT> parentProxy, NodeType num_nodes,
  Args&&... data)
  : parent_proxy_(parentProxy),
    num_nodes_(num_nodes),
    this_node_(vt::theContext()->getNode()),
    is_even_(this_node_ % 2 == 0),
    num_steps_(static_cast<int32_t>(log2(num_nodes_))),
    nprocs_pof2_(1 << num_steps_),
    nprocs_rem_(num_nodes_ - nprocs_pof2_),
    is_part_of_adjustment_group_(this_node_ < (2 * nprocs_rem_)){
  if (is_part_of_adjustment_group_) {
    if (is_even_) {
      vrt_node_ = this_node_ / 2;
    } else {
      vrt_node_ = -1;
    }
  } else {
    vrt_node_ = this_node_ - nprocs_rem_;
  }

  initialize(generateNewId(), std::forward<Args>(data)...);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
template <typename ...Args>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::initialize(
  size_t id, Args&&... data) {
  auto& state = states_[id];

  if(not state.initialized_){
    initializeState(id);
  }

  state.val_ = DataT{std::forward<Args>(data)...};

  vt_debug_print(
    terse, allreduce, "RecursiveDoubling Initialize: size {} ID {}\n",
    DataType::size(state.val_), id
  );
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::initializeState(size_t id){
  auto& state = states_[id];

  vt_debug_print(terse, allreduce, "RecursiveDoubling initializing state for ID = {}\n", id);

  state.messages_.resize(num_steps_, nullptr);
  state.steps_recv_.resize(num_steps_, false);
  state.steps_reduced_.resize(num_steps_, false);

  state.completed_ = false;
  state.step_ = 0;
  state.mask_ = 1;
  state.finished_adjustment_part_ = not is_part_of_adjustment_group_;
  state.initialized_ = true;
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::allreduce(size_t id) {
  if (is_part_of_adjustment_group_) {
    adjustForPowerOfTwo(id);
  } else {
    reduceIter(id);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::adjustForPowerOfTwo(size_t id) {
  auto& state = states_.at(id);
  if (is_part_of_adjustment_group_ and not is_even_) {
    vt_debug_print(
      terse, allreduce, "RecursiveDoubling AdjustInitial (To {}): ID = {}  \n",
      this_node_, this_node_ - 1, id
    );

    proxy_[this_node_ - 1]
      .template send<&RecursiveDoubling::adjustForPowerOfTwoHandler>(state.val_, id);
  }else if(state.adjust_message_ != nullptr){
    // We have pending reduce message
    adjustForPowerOfTwoHandler(state.adjust_message_.get());
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::
  adjustForPowerOfTwoHandler(AllreduceDblRawMsg<DataT>* msg) {

  auto& state = states_[msg->id_];
  if (DataType::size(state.val_) == 0) {
    if (not state.initialized_) {
      initializeState(msg->id_);
    }
    state.adjust_message_ = promoteMsg(msg);

    return;
  }

  Op<DataT>()(state.val_, *(msg->val_));

  state.finished_adjustment_part_ = true;

  reduceIter(msg->id_);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
bool RecursiveDoubling<DataT, Op, ObjT, finalHandler>::isDone(size_t id) {
  auto& state = states_.at(id);
  return (state.step_ == num_steps_) and allMessagesReceived(id);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
bool RecursiveDoubling<DataT, Op, ObjT, finalHandler>::isValid(size_t id) {
  auto& state = states_.at(id);
  return (vrt_node_ != -1) and (state.step_ < num_steps_);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
bool RecursiveDoubling<DataT, Op, ObjT, finalHandler>::allMessagesReceived(size_t id) {
  auto& state = states_.at(id);
  return std::all_of(
    state.steps_recv_.cbegin(), state.steps_recv_.cbegin() + state.step_,
    [](const auto val) { return val; });
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
bool RecursiveDoubling<DataT, Op, ObjT, finalHandler>::isReady(size_t id) {
  auto& state = states_.at(id);
  return ((is_part_of_adjustment_group_ and state.finished_adjustment_part_) and
          state.step_ == 0) or
    allMessagesReceived(id);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::reduceIter(size_t id) {
  // Ensure we have received all necessary messages
  if (not isReady(id)) {
    return;
  }

  auto& state = states_.at(id);
  auto vdest = vrt_node_ ^ state.mask_;
  auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;
  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling Part2 (Send step {}): To Node {} ID = {} \n",
    state.step_, dest, id
  );

  proxy_[dest].template send<&RecursiveDoubling::reduceIterHandler>(state.val_, id, state.step_);

  state.mask_ <<= 1;
  state.step_++;

  tryReduce(id, state.step_ - 1);

  if (isDone(id)) {
    finalPart(id);
  } else if (isReady(id)) {
    reduceIter(id);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::tryReduce(size_t id, int32_t step) {
  auto& state = states_.at(id);
  auto const all_msgs_received = std::all_of(
      state.steps_reduced_.cbegin(), state.steps_reduced_.cbegin() + step,
      [](const auto val) { return val; });

  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling Part2 (Reduce step {}): state.step_ = {} "
    "state.steps_reduced_[step] = {} state.steps_recv_[step] = {} "
    "all_msgs_received = {} ID = {} \n",
    step, state.step_, static_cast<bool>(state.steps_reduced_[step]),
    static_cast<bool>(state.steps_recv_[step]), all_msgs_received, id
  );

  if (
    (step < state.step_) and not state.steps_reduced_[step] and
    state.steps_recv_[step] and all_msgs_received) {
    Op<DataT>()(state.val_, *(state.messages_.at(step)->val_));

    state.steps_reduced_[step] = true;
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::reduceIterHandler(
  AllreduceDblRawMsg<DataT>* msg) {
  auto& state = states_[msg->id_];

  if (DataType::size(state.val_) == 0) {
    if (not state.initialized_) {
      initializeState(msg->id_);
    }
    state.messages_.at(msg->step_) = promoteMsg(msg);
    state.steps_recv_[msg->step_] = true;

    return;
  }

  state.messages_.at(msg->step_) = promoteMsg(msg);
  state.steps_recv_[msg->step_] = true;

  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling Part2 (Recv step {}): finished_adjustment_part_ = {} mask_= {} nprocs_pof2_ = {} "
    "from {} ID = {}\n",
    msg->step_, state.finished_adjustment_part_, state.mask_, nprocs_pof2_,
    theContext()->getFromNodeCurrentTask(), msg->id_
  );

  // Special case when we receive step 2 message before step 1 is done on this node
  if (not state.finished_adjustment_part_) {
    return;
  }

  tryReduce(msg->id_, msg->step_);

  if ((state.mask_ < nprocs_pof2_) and isReady(msg->id_)) {
    reduceIter(msg->id_);

  } else if (isDone(msg->id_)) {
    finalPart(msg->id_);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::sendToExcludedNodes(size_t id) {
  if (is_part_of_adjustment_group_ and is_even_) {
    vt_debug_print(
      terse, allreduce, "RecursiveDoubling Part3: Sending to Node {} ID = {}\n",
      this_node_ + 1, id
    );

    auto& state = states_.at(id);
    proxy_[this_node_ + 1]
      .template send<&RecursiveDoubling::sendToExcludedNodesHandler>(state.val_, id);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::
  sendToExcludedNodesHandler(AllreduceDblRawMsg<DataT>* msg) {

  parent_proxy_[this_node_].template invoke<finalHandler>(*(msg->val_));

  states_.at(msg->id_).completed_ = true;
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::finalPart(size_t id) {
  auto& state = states_.at(id);
  if (state.completed_) {
    return;
  }

  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling Part4: Executing final handler ID = {}\n", id
  );

  if (nprocs_rem_) {
    sendToExcludedNodes(id);
  }

  parent_proxy_[this_node_].template invoke<finalHandler>(state.val_);

  state.completed_ = true;
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_IMPL_H*/
