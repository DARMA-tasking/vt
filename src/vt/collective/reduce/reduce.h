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
#include "vt/collective/reduce/reduce_tags.h"
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
  template <typename T>
  using ReduceStateType = ReduceState<T>;
  using ReduceNumType   = typename ReduceState<void>::ReduceNumType;

  Reduce();
  Reduce(GroupType const& group, collective::tree::Tree* in_tree);

  template <typename MessageT, ActiveTypedFnType<MessageT>* f>
  SequentialIDType reduce(
    NodeType root, MessageT* const msg, TagType tag = no_tag,
    SequentialIDType seq = no_seq_id, ReduceNumType num_contrib = 1,
    VirtualProxyType proxy = no_vrt_proxy,
    ObjGroupProxyType obj_group = no_obj_group
  );

  template <
    typename OpT,
    typename MsgT,
    ActiveTypedFnType<MsgT> *f
  >
  SequentialIDType reduce(
    NodeType const& root, MsgT* msg, Callback<MsgT> cb,
    TagType const& tag = no_tag, SequentialIDType const& seq = no_seq_id,
    ReduceNumType const& num_contrib = 1,
    VirtualProxyType const& proxy = no_vrt_proxy,
    ObjGroupProxyType objgroup = no_obj_group
  );
  template <
    typename OpT,
    typename MsgT
  >
  SequentialIDType reduce(
    NodeType const& root, MsgT* msg, Callback<MsgT> cb,
    TagType const& tag = no_tag, SequentialIDType const& seq = no_seq_id,
    ReduceNumType const& num_contrib = 1,
    VirtualProxyType const& proxy = no_vrt_proxy,
    ObjGroupProxyType objgroup = no_obj_group
  )
  {
    return reduce<
      OpT,
      MsgT,
      &MsgT::template msgHandler<
        MsgT,
        OpT,
        collective::reduce::operators::ReduceCallback<MsgT>
        >
      >(root, msg, cb, tag, seq, num_contrib, proxy, objgroup);
  }

  template <
    typename OpT,
    typename FunctorT,
    typename MsgT,
    ActiveTypedFnType<MsgT> *f
  >
  SequentialIDType reduce(
    NodeType const& root, MsgT* msg, TagType const& tag = no_tag,
    SequentialIDType const& seq = no_seq_id, ReduceNumType const& num_contrib = 1,
    VirtualProxyType const& proxy = no_vrt_proxy
  );
  template <
    typename OpT,
    typename FunctorT,
    typename MsgT
  >
  SequentialIDType reduce(
    NodeType const& root, MsgT* msg, TagType const& tag = no_tag,
    SequentialIDType const& seq = no_seq_id, ReduceNumType const& num_contrib = 1,
    VirtualProxyType const& proxy = no_vrt_proxy
  )
  {
    return reduce<
      OpT,
      FunctorT,
      MsgT,
      &MsgT::template msgHandler<MsgT, OpT, FunctorT>
      >(root, msg, tag, seq, num_contrib, proxy);
  }

  template <typename MessageT>
  void reduceAddMsg(
    MessageT* msg, bool const local, ReduceNumType num_contrib = -1
  );

  template <typename MessageT>
  void reduceNewMsg(MessageT* msg);

  /*
   *  Explicitly start the reduction when the number of contributions is not
   *  known up front
   */
  template <typename MessageT>
  void startReduce(
    TagType tag, SequentialIDType seq, VirtualProxyType proxy,
    ObjGroupProxyType objgroup, bool use_num_contrib = true
  );

  template <typename MessageT>
  static void reduceRootRecv(MessageT* msg);
  template <typename MessageT>
  static void reduceUp(MessageT* msg);

private:
  std::unordered_map<ReduceSeqLookupType,SequentialIDType> next_seq_for_tag_;
  GroupType group_ = default_group;
};

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_H*/
