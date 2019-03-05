/*
//@HEADER
// ************************************************************************
//
//                          reduce.impl.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H
#define INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H

#include "vt/config.h"
#include "vt/collective/collective_alg.h"
#include "vt/registry/registry.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/serialization/auto_dispatch/dispatch.h"
#include "vt/messaging/active.h"
#include "vt/runnable/general.h"
#include "vt/group/group_headers.h"
#include "vt/messaging/message.h"

namespace vt { namespace collective { namespace reduce {

template <typename MessageT>
/*static*/ void Reduce::reduceUp(MessageT* msg) {
  debug_print(
    reduce, node,
    "reduceUp: tag={}, epoch={}, vrt={}, msg={}\n",
    msg->reduce_tag_, msg->reduce_epoch_, msg->reduce_proxy_, print_ptr(msg)
  );
  auto const grp = envelopeGetGroup(msg->env);
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
  auto active_fun = auto_registry::getAutoHandler(handler);
  msg->next_ = nullptr;
  msg->count_ = 1;
  msg->is_root_ = true;
  auto const& from_node = theMsg()->getFromNodeCurrentHandler();
  runnable::Runnable<MessageT>::run(handler, nullptr, msg, from_node);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
EpochType Reduce::reduce(
  NodeType root, MessageT* const msg, TagType tag, EpochType epoch,
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
    "reduce: tag={}, epoch={}, vrt={}, objgrp={}, contrib={}, msg={}, ref={}\n",
    msg->reduce_tag_, msg->reduce_epoch_, msg->reduce_proxy_,
    msg->reduce_objgroup_, num_contrib, print_ptr(msg), envelopeGetRef(msg->env)
  );
  if (epoch == no_epoch) {
    auto reduce_epoch_lookup = std::make_tuple(proxy,tag,objgroup);
    auto iter = next_epoch_for_tag_.find(reduce_epoch_lookup);
    if (iter == next_epoch_for_tag_.end()) {
      next_epoch_for_tag_.emplace(
        std::piecewise_construct,
        std::forward_as_tuple(reduce_epoch_lookup),
        std::forward_as_tuple(EpochType{1})
      );
      iter = next_epoch_for_tag_.find(reduce_epoch_lookup);
    }
    vtAssert(iter != next_epoch_for_tag_.end(), "Must exist now");
    msg->reduce_epoch_ = iter->second++;
  } else {
    msg->reduce_epoch_ = epoch;
  }
  reduceAddMsg<MessageT>(msg,true,num_contrib);
  reduceNewMsg<MessageT>(msg);
  return msg->reduce_epoch_;
}

template <typename MessageT>
void Reduce::reduceAddMsg(
  MessageT* msg, bool const local, ReduceNumType num_contrib
) {
  auto lookup = ReduceIdentifierType{
    msg->reduce_tag_,
    msg->reduce_epoch_,
    msg->reduce_proxy_,
    msg->reduce_objgroup_
  };
  auto live_iter = live_reductions_.find(lookup);
  if (live_iter == live_reductions_.end()) {
    auto num_contrib_state = num_contrib == -1 ? 1 : num_contrib;
    live_reductions_.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(lookup),
      std::forward_as_tuple(ReduceState{
        msg->reduce_tag_,msg->reduce_epoch_,num_contrib_state
      })
    );
    live_iter = live_reductions_.find(lookup);
    vtAssertExpr(live_iter != live_reductions_.end());
  }
  auto& state = live_iter->second;
  auto msg_ptr = promoteMsg(msg).template to<ReduceMsg>();;
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
    "reduceAddMsg: msg={}, contrib={}, msgs.size()={}, ref={}\n",
    print_ptr(msg), state.num_contrib_,
    state.msgs.size(), envelopeGetRef(msg->env)
  );
}

template <typename MessageT>
void Reduce::startReduce(
  TagType tag, EpochType epoch, VirtualProxyType proxy,
  ObjGroupProxyType objgroup, bool use_num_contrib
) {
  auto lookup = ReduceIdentifierType{tag,epoch,proxy,objgroup};
  auto live_iter = live_reductions_.find(lookup);
  auto& state = live_iter->second;

  auto const& nmsgs = state.msgs.size();
  bool ready = false;

  debug_print(
    reduce, node,
    "startReduce: tag={}, epoch={}, vrt={}, msg={}, children={}, contrib_={}\n",
    tag, epoch, proxy, state.msgs.size(), getNumChildren(), state.num_contrib_
  );

  if (use_num_contrib) {
    ready = nmsgs == getNumChildren() + state.num_contrib_;
  } else {
    ready = nmsgs == getNumChildren() + state.num_local_contrib_;
  }

  if (ready) {
    // Combine messages
    if (state.msgs.size() > 1) {
      for (int i = 0; i < state.msgs.size(); i++) {
        bool const has_next = i+1 < state.msgs.size();
        state.msgs[i]->next_ = has_next ? state.msgs[i+1].get() : nullptr;
        state.msgs[i]->count_ = state.msgs.size() - i;
        state.msgs[i]->is_root_ = false;

        debug_print(
          reduce, node,
          "i={} next={} has_next={} count={} msgs.size()={}, ref={}\n",
          i, print_ptr(state.msgs[i]->next_), has_next, state.msgs[i]->count_,
          state.msgs.size(), envelopeGetRef(state.msgs[i]->env)
        );
      }

      debug_print(
        reduce, node,
        "msgs.size()={}\n", state.msgs.size()
      );

       /*
        *  Invoke user handler to run the functor that combines messages,
        *  applying the reduction operator
        */
      auto const& handler = state.combine_handler_;
      auto active_fun = auto_registry::getAutoHandler(handler);
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

        using SendDispatch =
          serialization::auto_dispatch::RequiredSerialization<
            MessageT, reduceRootRecv<MessageT>
          >;
        SendDispatch::sendMsg(root,typed_msg);
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
      using SendDispatch =
        serialization::auto_dispatch::RequiredSerialization<
          MessageT, reduceUp<MessageT>
        >;
      SendDispatch::sendMsg(parent,typed_msg);
    }
  }
}

template <typename MessageT>
void Reduce::reduceNewMsg(MessageT* msg) {
  return startReduce<MessageT>(
    msg->reduce_tag_,
    msg->reduce_epoch_,
    msg->reduce_proxy_,
    msg->reduce_objgroup_
  );
}

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_COLLECTIVE_REDUCE_REDUCE_IMPL_H*/
