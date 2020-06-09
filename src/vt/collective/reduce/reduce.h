/*
//@HEADER
// *****************************************************************************
//
//                                   reduce.h
//                           DARMA Toolkit v. 1.0.0
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

#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_H

#include "vt/config.h"
#include "vt/collective/reduce/reduce.fwd.h"
#include "vt/collective/reduce/reduce_hash.h"
#include "vt/collective/reduce/reduce_state.h"
#include "vt/collective/reduce/reduce_state_holder.h"
#include "vt/collective/reduce/reduce_msg.h"
#include "vt/collective/reduce/operators/default_msg.h"
#include "vt/collective/reduce/operators/default_op.h"
#include "vt/collective/reduce/operators/callback_op.h"
#include "vt/messaging/active.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/collective/tree/tree.h"
#include "vt/utils/hash/hash_tuple.h"

#include <tuple>
#include <unordered_map>
#include <cassert>
#include <cstdint>

namespace vt { namespace collective { namespace reduce {

struct Reduce : virtual collective::tree::Tree {
  using ReduceStateType = ReduceState;
  using ReduceNumType   = typename ReduceStateType::ReduceNumType;

  explicit Reduce(detail::ReduceScope const& in_scope);

  Reduce(
    detail::ReduceScope const& in_scope, collective::tree::Tree* in_tree
  );

  detail::ReduceStamp generateNextID();

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  detail::ReduceStamp reduce(
    NodeType root, MsgT* const msg,
    detail::ReduceStamp id = detail::ReduceStamp{},
    ReduceNumType num_contrib = 1
  );

  template <
    typename OpT,
    typename MsgT,
    ActiveTypedFnType<MsgT> *f = MsgT::template msgHandler<
      MsgT, OpT, collective::reduce::operators::ReduceCallback<MsgT>
    >
  >
  detail::ReduceStamp reduce(
    NodeType const& root, MsgT* msg, Callback<MsgT> cb,
    detail::ReduceStamp id = detail::ReduceStamp{},
    ReduceNumType const& num_contrib = 1
  );

  template <
    typename OpT,
    typename FunctorT,
    typename MsgT,
    ActiveTypedFnType<MsgT> *f = MsgT::template msgHandler<MsgT, OpT, FunctorT>
  >
  detail::ReduceStamp reduce(
    NodeType const& root, MsgT* msg,
    detail::ReduceStamp id = detail::ReduceStamp{},
    ReduceNumType const& num_contrib = 1
  );

  template <typename MsgT>
  void reduceAddMsg(
    MsgT* msg, bool const local, ReduceNumType num_contrib = -1
  );

  template <typename MsgT>
  void reduceNewMsg(MsgT* msg);

  /*
   *  Explicitly start the reduction when the number of contributions is not
   *  known up front
   */
  template <typename MsgT>
  void startReduce(detail::ReduceStamp id, bool use_num_contrib = true);

  template <typename MsgT>
  void reduceRootRecv(MsgT* msg);

  template <typename MsgT>
  void reduceUp(MsgT* msg);

private:
  detail::ReduceScope scope_;
  ReduceStateHolder state_;
  detail::StrongSeq next_seq_;
};

}}} /* end namespace vt::collective::reduce */

#include "vt/collective/reduce/reduce.impl.h"

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_H*/
