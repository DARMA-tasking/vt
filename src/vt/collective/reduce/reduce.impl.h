/*
//@HEADER
// *****************************************************************************
//
//                                reduce.impl.h
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

#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H

#include "vt/config.h"
#include "vt/collective/collective_alg.h"
#include "vt/registry/registry.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"
#include "vt/group/group_headers.h"
#include "vt/messaging/message.h"

namespace vt { namespace collective { namespace reduce {

template <typename MessageT>
/*static*/ void Reduce::reduceUp(MessageT* msg) {
  auto const grp = envelopeGetGroup(msg->env);
  debug_print(
    reduce, node,
    "reduceUp: group={:x}, tag={}, seq={}, vrt={}, msg={}\n",
    grp, msg->reduce_tag_, msg->reduce_seq_, msg->reduce_proxy_, print_ptr(msg)
  );
  if (grp == default_group) {
    theCollective()->reduceAddMsg<MessageT>(msg,false);
    theCollective()->reduceNewMsg<MessageT>(msg);
  } else {
    theGroup()->groupReduce(grp)->template reduceAddMsg<MessageT>(msg,false);
    theGroup()->groupReduce(grp)->template reduceNewMsg<MessageT>(msg);
  }
}

template <typename MessageT>
/*static*/ void Reduce::reduceRootRecv(MessageT* msg) {
  auto const& handler = msg->combine_handler_;
  msg->next_ = nullptr;
  msg->count_ = 1;
  msg->is_root_ = true;
  auto const& from_node = theMsg()->getFromNodeCurrentHandler();
  runnable::Runnable<MessageT>::run(handler, nullptr, msg, from_node);
}

template <typename OpT, typename MsgT, ActiveTypedFnType<MsgT> *f>
SequentialIDType Reduce::reduce(
  NodeType const& root, MsgT* msg, Callback<MsgT> cb, TagType const& tag,
  SequentialIDType const& seq, ReduceNumType const& num_contrib,
  VirtualProxyType const& proxy
) {
  msg->setCallback(cb);
  return reduce<MsgT,f>(root,msg,tag,seq,num_contrib,proxy);
}

template <
  typename OpT, typename FunctorT, typename MsgT, ActiveTypedFnType<MsgT> *f
>
SequentialIDType Reduce::reduce(
  NodeType const& root, MsgT* msg, TagType const& tag,
  SequentialIDType const& seq, ReduceNumType const& num_contrib,
  VirtualProxyType const& proxy
) {
  return reduce<MsgT,f>(root,msg,tag,seq,num_contrib,proxy);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
SequentialIDType Reduce::reduce(
  NodeType root, MessageT* const msg, TagType tag, SequentialIDType seq,
  ReduceNumType num_contrib, VirtualProxyType proxy, ObjGroupProxyType objgroup
) {
  if (group_ != default_group) {
    envelopeSetGroup(msg->env, group_);
  }
  auto const han = auto_registry::makeAutoHandler<MessageT,f>(msg);
  msg->combine_handler_ = han;
  msg->reduce_tag_ = tag;
  msg->reduce_root_ = root;
  msg->reduce_proxy_ = proxy;
  msg->reduce_objgroup_ = objgroup;
  debug_print(
    reduce, node,
    "reduce: group={:x}, tag={}, seq={}, vrt={}, objgrp={}, contrib={}, "
    "msg={}, ref={}\n",
    group_, msg->reduce_tag_, msg->reduce_seq_, msg->reduce_proxy_,
    msg->reduce_objgroup_, num_contrib, print_ptr(msg), envelopeGetRef(msg->env)
  );
  if (seq == no_seq_id) {
    auto reduce_seq_lookup = std::make_tuple(proxy,tag,objgroup);
    auto iter = next_seq_for_tag_.find(reduce_seq_lookup);
    if (iter == next_seq_for_tag_.end()) {
      next_seq_for_tag_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(reduce_seq_lookup),
        std::forward_as_tuple(SequentialIDType{1})
      );
      iter = next_seq_for_tag_.find(reduce_seq_lookup);
    }
    vtAssert(iter != next_seq_for_tag_.end(), "Must exist now");
    msg->reduce_seq_ = iter->second++;
  } else {
    msg->reduce_seq_ = seq;
  }
  reduceAddMsg<MessageT>(msg,true,num_contrib);
  reduceNewMsg<MessageT>(msg);
  return msg->reduce_seq_;
}

template <typename MessageT>
void Reduce::reduceAddMsg(
  MessageT* msg, bool const local, ReduceNumType num_contrib
) {
  auto lookup = ReduceIdentifierType{
    msg->reduce_tag_,
    msg->reduce_seq_,
    msg->reduce_proxy_,
    msg->reduce_objgroup_
  };

  auto exists = ReduceStateHolder<MessageT>::exists(group_,lookup);
  if (not exists) {
    auto num_contrib_state = num_contrib == -1 ? 1 : num_contrib;
    ReduceState<MessageT> state(
      msg->reduce_tag_,msg->reduce_seq_,num_contrib_state
    );
    ReduceStateHolder<MessageT>::insert(group_,lookup,std::move(state));
  }

  auto& state = ReduceStateHolder<MessageT>::find(group_,lookup);
  auto msg_ptr = promoteMsg(msg);
  state.msgs.push_back(msg_ptr);
  if (num_contrib != -1) {
    state.num_contrib_ = num_contrib;
  }
  if (local) {
    state.num_local_contrib_++;
  }
  state.combine_handler_ = msg->combine_handler_;
  state.reduce_root_ = msg->reduce_root_;
  debug_print(
    reduce, node,
    "reduceAddMsg: group={:x}, msg={}, contrib={}, msgs.size()={}, ref={}\n",
    group_, print_ptr(msg), state.num_contrib_,
    state.msgs.size(), envelopeGetRef(msg->env)
  );
}

template <typename MessageT>
void Reduce::startReduce(
  TagType tag, SequentialIDType seq, VirtualProxyType proxy,
  ObjGroupProxyType objgroup, bool use_num_contrib
) {
  auto lookup = ReduceIdentifierType{tag,seq,proxy,objgroup};
  auto& state = ReduceStateHolder<MessageT>::find(group_,lookup);

  std::size_t nmsgs = state.msgs.size();
  auto const contrib =
    use_num_contrib ? state.num_contrib_ : state.num_local_contrib_;
  std::size_t total = getNumChildren() + contrib;
  bool ready = nmsgs == total;

  debug_print(
    reduce, node,
    "startReduce: group={:x}, tag={}, seq={}, vrt={}, msg={}, children={}, "
    "contrib_={}, local_contrib_={}, nmsgs={}, ready={}\n",
    group_, tag, seq, proxy, state.msgs.size(), getNumChildren(),
    state.num_contrib_, state.num_local_contrib_, nmsgs, ready
  );

  if (ready) {
    // Combine messages
    if (state.msgs.size() > 1) {
      auto size = state.msgs.size();
      for (decltype(size) i = 0; i < size; i++) {
        bool const has_next = i+1 < size;
        state.msgs[i]->next_ = has_next ? state.msgs[i+1].get() : nullptr;
        state.msgs[i]->count_ = size - i;
        state.msgs[i]->is_root_ = false;

        debug_print(
          reduce, node,
          "i={} next={} has_next={} count={} msgs.size()={}, ref={}\n",
          i, print_ptr(state.msgs[i]->next_), has_next, state.msgs[i]->count_,
          size, envelopeGetRef(state.msgs[i]->env)
        );
      }

      debug_print(
        reduce, node,
        "msgs.size()={}\n", size
      );

       /*
        *  Invoke user handler to run the functor that combines messages,
        *  applying the reduction operator
        */
      auto const& handler = state.combine_handler_;
      auto const& from_node = theMsg()->getFromNodeCurrentHandler();
      runnable::Runnable<MessageT>::run(
        handler,nullptr,static_cast<MessageT*>(state.msgs[0].get()),from_node
      );
    }

    // Send to parent
    auto msg = state.msgs[0];
    auto typed_msg = static_cast<MessageT*>(msg.get());
    ActionType cont = nullptr;

    state.msgs.clear();
    state.num_contrib_ = 1;

    if (isRoot()) {
      auto const& root = state.reduce_root_;
      auto const& this_node = theContext()->getNode();
      if (root != this_node) {
        debug_print(
          reduce, node,
          "reduce notify root (send): root={}, node={}\n", root, this_node
        );

        theMsg()->sendMsg<MessageT,reduceRootRecv<MessageT>>(root,typed_msg);
      } else {
        debug_print(
          reduce, node,
          "reduce notify root (deliver directly): root={}, node={}\n",
          root, this_node
        );
        reduceRootRecv(typed_msg);
      }
    } else {
      auto const& parent = getParent();
      debug_print(
        reduce, node,
        "reduce send to parent: parent={}\n", parent
      );
      theMsg()->sendMsg<MessageT,reduceUp<MessageT>>(parent,typed_msg);
    }
  }
}

template <typename MessageT>
void Reduce::reduceNewMsg(MessageT* msg) {
  return startReduce<MessageT>(
    msg->reduce_tag_,
    msg->reduce_seq_,
    msg->reduce_proxy_,
    msg->reduce_objgroup_
  );
}

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H*/
