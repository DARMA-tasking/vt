/*
//@HEADER
// *****************************************************************************
//
//                                seq_parallel.h
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

#if !defined INCLUDED_VT_SEQUENCE_SEQ_PARALLEL_H
#define INCLUDED_VT_SEQUENCE_SEQ_PARALLEL_H

#include <vector>
#include <cstdint>
#include <atomic>

#include "vt/config.h"
#include "vt/utils/atomic/atomic.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_types.h"
#include "vt/sequence/seq_node_fwd.h"

namespace vt { namespace seq {

using ::vt::util::atomic::AtomicType;

struct SeqParallel {
  using SeqFuncLen = uint32_t;
  using SeqFunType = UserSeqFunType;
  using SeqParallelFuncType = std::vector<SeqFunType>;

  template <typename... FuncsT>
  SeqParallel(SeqType const& in_seq, ActionType in_action, FuncsT&&... funcs)
    : seq_id_(in_seq), num_funcs_(sizeof...(FuncsT)), par_funcs_({funcs...}),
      triggered_action_(in_action)
  { }

  SeqParallel(
    SeqType const& in_seq, ActionType in_action, SeqParallelFuncType funcs
  ) : seq_id_(in_seq), num_funcs_(funcs.size()), par_funcs_(funcs),
      triggered_action_(in_action)
  { }

  SeqParallel() = default;

  void setTriggeredAction(ActionType action);
  SeqFuncLen getNumFuncs() const;
  SeqFuncLen getNumFuncsCompleted() const;
  SeqFuncLen getSize() const;
  eSeqNodeState expandParallelNode(SeqNodePtrType this_node);
  bool join();

private:
  SeqType seq_id_ = no_seq;

  SeqFuncLen num_funcs_ = 0;

  AtomicType<SeqFuncLen> num_funcs_completed_ = {0};

  SeqParallelFuncType par_funcs_;

  ActionType triggered_action_ = nullptr;
};

}} //end namespace vt::seq

#endif /* INCLUDED_VT_SEQUENCE_SEQ_PARALLEL_H*/
