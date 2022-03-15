/*
//@HEADER
// *****************************************************************************
//
//                                reduce.impl.h
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

#if !defined INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_IMPL_H
#define INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_IMPL_H

#include "vt/config.h"
#include "vt/collective/collective_alg.h"
#include "vt/registry/registry.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/messaging/active.h"
#include "vt/messaging/message.h"
#include "vt/runnable/make_runnable.h"

namespace vt { namespace collective { namespace reduce {

template <typename MsgT>
void Reduce::reduceUpHan(MsgT* msg) {
  envelopeUnlockForForwarding(msg->env);

  vtAssert(msg->scope() == scope_, "Must match correct scope");

  vt_debug_print(
    verbose, reduce,
    "reduceUpHan: scope={}, stamp={}, msg={}\n",
    msg->scope().str(), detail::stringizeStamp(msg->stamp()), print_ptr(msg)
  );

  reduceAddMsg<MsgT>(msg,false);
  reduceNewMsg<MsgT>(msg);
}

template <typename MsgT>
void Reduce::reduceRootRecv(MsgT* msg) {
  auto const handler = msg->combine_handler_;
  msg->next_ = nullptr;
  msg->count_ = 1;
  msg->is_root_ = true;

  auto const from_node = theContext()->getFromNodeCurrentTask();
  auto m = promoteMsg(msg);
  runnable::makeRunnable(m, false, handler, from_node)
    .withTDEpochFromMsg()
    .run();
}

template <typename OpT, typename MsgT, ActiveTypedFnType<MsgT> *f>
Reduce::PendingSendType Reduce::reduce(
  NodeType const& root, MsgT* msg, Callback<MsgT> cb, detail::ReduceStamp id,
  ReduceNumType const& num_contrib
) {
  msg->setCallback(cb);
  return reduce<MsgT,f>(root,msg,id,num_contrib);
}

template <typename OpT, typename MsgT, ActiveTypedFnType<MsgT> *f>
detail::ReduceStamp Reduce::reduceImmediate(
  NodeType const& root, MsgT* msg, Callback<MsgT> cb, detail::ReduceStamp id,
  ReduceNumType const& num_contrib
) {
  msg->setCallback(cb);
  return reduceImmediate<MsgT,f>(root,msg,id,num_contrib);
}

template <
  typename OpT, typename FunctorT, typename MsgT, ActiveTypedFnType<MsgT> *f
>
Reduce::PendingSendType Reduce::reduce(
  NodeType const& root, MsgT* msg, detail::ReduceStamp id,
  ReduceNumType const& num_contrib
) {
  return reduce<MsgT,f>(root,msg,id,num_contrib);
}

template <
  typename OpT, typename FunctorT, typename MsgT, ActiveTypedFnType<MsgT> *f
>
detail::ReduceStamp Reduce::reduceImmediate(
  NodeType const& root, MsgT* msg, detail::ReduceStamp id,
  ReduceNumType const& num_contrib
) {
  return reduceImmediate<MsgT,f>(root,msg,id,num_contrib);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
Reduce::PendingSendType Reduce::reduce(
  NodeType root, MsgT* const msg, detail::ReduceStamp id,
  ReduceNumType num_contrib
) {
  auto msg_ptr = promoteMsg(msg);
  return PendingSendType{theMsg()->getEpochContextMsg(msg_ptr), [=](){
                           reduceImmediate<MsgT, f>(root, msg_ptr.get(), id, num_contrib);
                         } };
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
detail::ReduceStamp Reduce::reduceImmediate(
  NodeType root, MsgT* const msg, detail::ReduceStamp id,
  ReduceNumType num_contrib
) {
  if (scope_.get().is<detail::StrongGroup>()) {
    envelopeSetGroup(msg->env, scope_.get().get<detail::StrongGroup>().get());
  }
  auto cur_id = id == detail::ReduceStamp{} ? generateNextID() : id;

  auto const han = auto_registry::makeAutoHandler<MsgT,f>();
  msg->combine_handler_ = han;
  msg->reduce_root_ = root;
  msg->reduce_id_ = detail::ReduceIDImpl{cur_id, scope_};

  vt_debug_print(
    terse, reduce,
    "reduce: scope={}, stamp={}, contrib={}, msg={}, ref={}\n",
    scope_.str(), detail::stringizeStamp(id), num_contrib, print_ptr(msg),
    envelopeGetRef(msg->env)
  );

  reduceAddMsg<MsgT>(msg,true,num_contrib);
  reduceNewMsg<MsgT>(msg);

  return cur_id;
}

template <typename MsgT>
void Reduce::reduceAddMsg(
  MsgT* msg, bool const local, ReduceNumType num_contrib
) {
  auto lookup = msg->stamp();

  auto exists = state_.exists(lookup);

  if (not exists) {
    auto num_contrib_state = num_contrib == -1 ? 1 : num_contrib;
    ReduceState state(num_contrib_state);
    state_.insert(lookup,std::move(state));
  }

  auto& state = state_.find(lookup);
  auto msg_ptr = promoteMsg(msg);

  // Run-time cast with lasting type info to ReduceMsg for holder
  state.msgs.push_back(msg_ptr.template to<ReduceMsg>());
  if (num_contrib != -1) {
    state.num_contrib_ = num_contrib;
  }
  if (local) {
    state.num_local_contrib_++;
  }
  state.combine_handler_ = msg->combine_handler_;
  state.reduce_root_ = msg->reduce_root_;
  vt_debug_print(
    verbose, reduce,
    "reduceAddMsg: scope={}, stamp={}, msg={}, contrib={}, msgs.size()={}, "
    "ref={}\n",
    scope_.str(), detail::stringizeStamp(lookup), print_ptr(msg),
    state.num_contrib_, state.msgs.size(), envelopeGetRef(msg->env)
  );
}

template <typename MsgT>
void Reduce::startReduce(detail::ReduceStamp id, bool use_num_contrib) {
  auto lookup = id;
  auto& state = state_.find(lookup);

  std::size_t nmsgs = state.msgs.size();
  auto const contrib =
    use_num_contrib ? state.num_contrib_ : state.num_local_contrib_;
  std::size_t total = getNumChildren() + contrib;
  bool ready = nmsgs == total;

  vt_debug_print(
    normal, reduce,
    "startReduce: scope={}, stamp={}, msg={}, children={}, "
    "contrib_={}, local_contrib_={}, nmsgs={}, ready={}\n",
    scope_.str(), detail::stringizeStamp(id), state.msgs.size(),
    getNumChildren(), state.num_contrib_, state.num_local_contrib_, nmsgs, ready
  );

  if (ready) {
    // Combine messages
    if (state.msgs.size() > 1) {
      auto size = state.msgs.size();
      for (decltype(size) i = 0; i < size; i++) {
        bool const has_next = i+1 < size;
        // Collection is of MsgPtr<ReduceMsg>
        auto typed_msg = static_cast<MsgT*>(state.msgs[i].get());
        if (has_next) {
          typed_msg->next_ = static_cast<MsgT*>(state.msgs[i+1].get());
        } else {
          typed_msg->next_ = nullptr;
        }
        typed_msg->count_ = size - i;
        typed_msg->is_root_ = false;

        vt_debug_print(
          verbose, reduce,
          "scope={}, stamp={}: i={} next={} has_next={} count={} msgs.size()={} "
          "ref={}\n",
          scope_.str(), detail::stringizeStamp(id),
          i, print_ptr(typed_msg->next_), has_next, typed_msg->count_,
          size, envelopeGetRef(typed_msg->env)
        );
      }

      vt_debug_print(
        verbose, reduce,
        "scope={}, stamp={}, msgs.size()={}\n",
        scope_.str(), detail::stringizeStamp(id), size
      );

      /*
       *  Invoke user handler to run the functor that combines messages,
       *  applying the reduction operator
       */
      auto const handler = state.combine_handler_;
      auto const from_node = theContext()->getFromNodeCurrentTask();

      // this needs to run inline.. threaded not allowed for reduction
      // combination
      runnable::makeRunnable(state.msgs[0], false, handler, from_node)
        .withTDEpochFromMsg()
        .run();
    }

    // Send to parent
    // Collection is of MsgPtr<ReduceMsg>, re-type and drop collection owner.
    auto msg = state.msgs[0];
    MsgPtr<MsgT> typed_msg = msg.template to<MsgT>();
    state.msgs.clear();
    state.num_contrib_ = 1;
    NodeType const root = state.reduce_root_;

    // Must erase this before invoking the root function/callback because it
    // might be re-entrant
    state_.erase(lookup);

    if (isRoot()) {
      auto const& this_node = theContext()->getNode();
      if (root != this_node) {
        vt_debug_print(
          normal, reduce,
          "reduce notify root (send): scope={}, stamp={}, root={}, node={}\n",
          scope_.str(), detail::stringizeStamp(id), root, this_node
        );

        theMsg()->sendMsg<MsgT,ReduceManager::reduceRootRecv<MsgT>>(root, typed_msg);
      } else {
        vt_debug_print(
          normal, reduce,
          "reduce notify root (direct): scope={}, stamp={}, root={}, node={}\n",
          scope_.str(), detail::stringizeStamp(id), root, this_node
        );
        theMsg()->setupEpochMsg(typed_msg.get());
        reduceRootRecv(typed_msg.get());
      }
    } else {
      auto const& parent = getParent();
      vt_debug_print(
        normal, reduce,
        "reduce send to parent: scope={}, stamp={}, parent={}\n",
        scope_.str(), detail::stringizeStamp(id), parent
      );

      theMsg()->sendMsg<MsgT,ReduceManager::reduceUpHan<MsgT>>(parent, typed_msg);
    }
  }
}

template <typename MsgT>
void Reduce::reduceNewMsg(MsgT* msg) {
  vtAssert(msg->scope() == scope_, "Must match correct scope");
  return startReduce<MsgT>(msg->stamp());
}

}}} /* end namespace vt::collective::reduce */

#endif /*INCLUDED_VT_COLLECTIVE_REDUCE_REDUCE_IMPL_H*/
