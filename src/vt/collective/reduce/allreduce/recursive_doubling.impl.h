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
RecursiveDoubling<DataT, Op, ObjT, finalHandler>::RecursiveDoubling(
  vt::objgroup::proxy::Proxy<ObjT> parentProxy, NodeType num_nodes,
  const DataT& data)
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

  messages_.resize(num_steps_, nullptr);
  steps_recv_.resize(num_steps_, false);
  steps_reduced_.resize(num_steps_, false);

  initialize(data);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::initialize(
  const DataT& data) {
  val_ = DataType::toVec(data);

  completed_ = false;
  step_ = 0;
  mask_ = 1;
  finished_adjustment_part_ = not is_part_of_adjustment_group_;
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::allreduce() {
  if (is_part_of_adjustment_group_) {
    adjustForPowerOfTwo();
  } else {
    reduceIter();
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::adjustForPowerOfTwo() {
  if (is_part_of_adjustment_group_ and not is_even_) {
    vt_debug_print(
      terse, allreduce, "[{}] RecursiveDoubling Part1: Sending to Node {} \n", this_node_,
      this_node_ - 1
    );

    proxy_[this_node_ - 1]
      .template send<&RecursiveDoubling::adjustForPowerOfTwoHandler>(val_);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::
  adjustForPowerOfTwoHandler(AllreduceDblRawMsg<Scalar>* msg) {

  for(size_t i = 0; i < val_.size(); ++i){
    Op<Scalar>()(val_[i], msg->val_[i]);
  }

  finished_adjustment_part_ = true;

  reduceIter();
}
template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
bool RecursiveDoubling<DataT, Op, ObjT, finalHandler>::done() {
  return step_ == num_steps_ and allMessagesReceived();
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
bool RecursiveDoubling<DataT, Op, ObjT, finalHandler>::isValid() {
  return (vrt_node_ != -1) and (step_ < num_steps_);
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
bool RecursiveDoubling<DataT, Op, ObjT, finalHandler>::allMessagesReceived() {
  return std::all_of(
    steps_recv_.cbegin(), steps_recv_.cbegin() + step_,
    [](const auto val) { return val; });
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
bool RecursiveDoubling<DataT, Op, ObjT, finalHandler>::isReady() {
  return ((is_part_of_adjustment_group_ and finished_adjustment_part_) and
          step_ == 0) or
    allMessagesReceived();
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::reduceIter() {
  // Ensure we have received all necessary messages
  if (not isReady()) {
    return;
  }

  auto vdest = vrt_node_ ^ mask_;
  auto dest = (vdest < nprocs_rem_) ? vdest * 2 : vdest + nprocs_rem_;
  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling Part2 (Send step {}): To Node {} \n",
    step_, dest
  );

  proxy_[dest].template send<&RecursiveDoubling::reduceIterHandler>(
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

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::tryReduce(int32_t step) {
  if (
    (step < step_) and not steps_reduced_[step] and steps_recv_[step] and
    std::all_of(
      steps_reduced_.cbegin(), steps_reduced_.cbegin() + step,
      [](const auto val) { return val; })) {
    for (size_t i = 0; i < val_.size(); ++i) {
      Op<Scalar>()(val_[i], messages_.at(step)->val_[i]);
    }

    steps_reduced_[step] = true;
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::reduceIterHandler(
  AllreduceDblRawMsg<Scalar>* msg) {
  vt_debug_print(
    terse, allreduce,
    "RecursiveDoubling Part2 (Recv step {}): mask_= {} nprocs_pof2_ = {}: "
    "from {}\n",
    msg->step_, mask_, nprocs_pof2_,
    theContext()->getFromNodeCurrentTask()
  );

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

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::sendToExcludedNodes() {
  if (is_part_of_adjustment_group_ and is_even_) {
    vt_debug_print(
      terse, allreduce, "RecursiveDoubling Part3: Sending to Node {}  \n",
      this_node_ + 1
    );

    proxy_[this_node_ + 1]
      .template send<&RecursiveDoubling::sendToExcludedNodesHandler>(val_);
  }
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::
  sendToExcludedNodesHandler(AllreduceDblRawMsg<Scalar>* msg) {
  // val_ = msg->val_;
  // std::memcpy(val_.data(), msg->val_, val_.size() * sizeof(Scalar));

  parent_proxy_[this_node_].template invoke<finalHandler>(
    DataType::fromMemory(msg->val_, msg->size_)
  );
  completed_ = true;
}

template <
  typename DataT, template <typename Arg> class Op, typename ObjT,
  auto finalHandler>
void RecursiveDoubling<DataT, Op, ObjT, finalHandler>::finalPart() {
  if (completed_) {
    return;
  }

  if (nprocs_rem_) {
    sendToExcludedNodes();
  }

  parent_proxy_[this_node_].template invoke<finalHandler>(DataType::fromVec(val_));

  completed_ = true;

  std::fill(messages_.begin(), messages_.end(), nullptr);

  steps_recv_.assign(num_steps_, false);
  steps_reduced_.assign(num_steps_, false);
}

} // namespace vt::collective::reduce::allreduce

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_ALLREDUCE_RECURSIVE_DOUBLING_IMPL_H*/
