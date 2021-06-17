/*
//@HEADER
// *****************************************************************************
//
//                               seq_parallel.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_parallel.h"
#include "vt/sequence/sequencer.h"
#include "vt/termination/term_headers.h"

namespace vt { namespace seq {

void SeqParallel::setTriggeredAction(ActionType action) {
  triggered_action_ = action;
}

SeqParallel::SeqFuncLen SeqParallel::getNumFuncs() const {
  return num_funcs_;
}

SeqParallel::SeqFuncLen SeqParallel::getNumFuncsCompleted() const {
  return num_funcs_completed_;
}

SeqParallel::SeqFuncLen SeqParallel::getSize() const {
  return num_funcs_ - num_funcs_completed_;
}

SeqNodeStateEnumType SeqParallel::expandParallelNode(SeqNodePtrType this_node) {
  vt_debug_print(
    normal, sequence,
    "SeqParallel: expandParallelNode: funcs={}, skip_queue={}, node={}\n",
    num_funcs_, print_bool(seq_skip_queue), PRINT_SEQ_NODE_PTR(this_node)
  );

  auto cur_state = SeqNodeStateEnumType::KeepExpandingState;

  // Inform the termination detector to produce children nodes---ensures that
  // deferred execution of these does not cause termination be reached
  // incorrectly if the wait execution is delayed
  theTerm()->produce(term::any_epoch_sentinel, par_funcs_.size());

  for (auto&& par_fn : par_funcs_) {
    vt_debug_print(
      normal, sequence,
      "SeqParallel: expandParallelNode: num_funcs_={}, expanding\n", num_funcs_
    );

    if (seq_skip_queue) {
      bool const blocked = executeSeqExpandContext(seq_id_, this_node, par_fn);
      if (blocked) {
        cur_state = SeqNodeStateEnumType::WaitingNextState;
      }
    } else {
      auto defer_work = [=]{
        vt_debug_print(
          normal, sequence,
          "SeqParallel: parallel node: expand deferred: id={}\n", seq_id_
        );

        executeSeqExpandContext(seq_id_, this_node, par_fn);
      };
      enqueueAction(seq_id_, defer_work);
    }
  }

  if (seq_skip_queue) {
    return cur_state;
  } else {
    return SeqNodeStateEnumType::WaitingNextState;
  }
}

bool SeqParallel::join() {
  auto const& old_val = num_funcs_completed_.fetch_add(1);

  vt_debug_print(
    normal, sequence,
    "SeqParallel: join: old_val={}, num_funcs={}\n", old_val, num_funcs_
  );

  // Inform the termination detector that a child is consumed
  theTerm()->consume(term::any_epoch_sentinel, 1);

  if (old_val == num_funcs_ - 1) {
    if (triggered_action_ != nullptr) {
      triggered_action_();
    }
    return true;
  } else if (old_val > num_funcs_ - 1) {
    return true;
  } else {
    return false;
  }
}

}} //end namespace vt::seq
