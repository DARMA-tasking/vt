/*
//@HEADER
// *****************************************************************************
//
//                               sequencer.impl.h
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

#if !defined INCLUDED_SEQUENCE_SEQUENCER_IMPL_H
#define INCLUDED_SEQUENCE_SEQUENCER_IMPL_H

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/sequencer_headers.h"
#include "vt/scheduler/scheduler.h"

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
typename TaggedSequencer<SeqTag, SeqTrigger>::SeqType
TaggedSequencer<SeqTag, SeqTrigger>::createSeq() {
  return nextSeq();
}

template <typename SeqTag, template <typename> class SeqTrigger>
/*virtual*/ typename TaggedSequencer<SeqTag, SeqTrigger>::SeqType
TaggedSequencer<SeqTag, SeqTrigger>::getNextID() {
  return seq_manager->nextSeqID(false);
}

template <typename SeqTag, template <typename> class SeqTrigger>
typename TaggedSequencer<SeqTag, SeqTrigger>::SeqType
TaggedSequencer<SeqTag, SeqTrigger>::nextSeq() {
  auto const cur_id = getNextID();

  auto seq_iter = seq_lookup_.find(cur_id);

  vtAssert(
    seq_iter == seq_lookup_.end(), "New seq_id should not exist now"
  );

  seq_lookup_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(cur_id),
    std::forward_as_tuple(SeqListType{cur_id})
  );

  return cur_id;
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::assertValidContext() const {
  vtAssert(hasContext(), "Must be in a valid sequence");
}

template <typename SeqTag, template <typename> class SeqTrigger>
bool TaggedSequencer<SeqTag, SeqTrigger>::hasContext() const {
  return context_ != nullptr;
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::for_loop(
  ForIndex const& begin, ForIndex const& end, ForIndex const& stride,
  FuncIndexType fn
) {
  if (begin < end) {
    sequenced([=]{
      fn(begin);
      sequenced([=]{
        for_loop(begin+stride, end, stride, fn);
      });
    });
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::sequenced(UserSeqFunType const& fn) {
  assertValidContext();

  vt_debug_print(
    sequence, node,
    "Sequencer: sequenced: fn: context_={}\n",
    print_ptr(context_)
  );

  return sequenced(context_->getSeq(), fn);
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::sequenced(
  SeqType const& seq_id, UserSeqFunWithIDType const& fn
) {
  vt_debug_print(
    sequence, node,
    "Sequencer: sequenced (UserSeqFunWithIDType) seq_id={}\n", seq_id
  );

  auto sfn = [=]{ fn(seq_id); };
  return sequenced(seq_id, sfn);
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::sequenced(
  SeqType const& seq_id, UserSeqFunType const& fn
) {
  bool const has_context = hasContext();

  vt_debug_print(
    sequence, node,
    "Sequencer: sequenced (UserSeqFunType) seq_id={}: has_context={}\n",
    seq_id, print_bool(has_context)
  );

  auto converted_fn = convertSeqFun(seq_id, fn);

  if (has_context) {
    // add to current inner node context container for sequence
    SeqNodePtrType node = getNode(seq_id);
    node->addSequencedClosure(converted_fn, false);
  } else {
    // add to outer scope container for sequence
    SeqListType& lst = getSeqList(seq_id);
    lst.addAction(converted_fn);
    enqueueSeqList(seq_id);
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::parallel_lst(
  SeqFuncContainerType const& fn_list
) {
  assertValidContext();
  return parallel_lst(context_->getSeq(), fn_list);
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::parallel_lst(
  SeqType const& seq_id, SeqFuncContainerType const& fn_list
) {
  bool const has_context = hasContext();

  vt_debug_print(
    sequence, node,
    "Sequencer: parallel: seq_id={}, has_context={}, num fns={}\n",
    seq_id, print_bool(has_context), fn_list.size()
  );

  SeqFuncContainerType new_fn_list;

  for (auto&& elm : fn_list) {
    new_fn_list.emplace_back(convertSeqFun(seq_id, elm));
  }

  SeqNodePtrType par_node = SeqNode::makeParallelNode(seq_id, new_fn_list);

  return dispatch_parallel(has_context, seq_id, par_node);
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename... FnT>
void TaggedSequencer<SeqTag, SeqTrigger>::parallel(FnT&&... fns) {
  assertValidContext();

  vt_debug_print(
    sequence, node,
    "Sequencer: parallel: fn: context_={}: num fns={}\n",
    context_, sizeof...(fns)
  );

  return parallel(context_->getSeq(), std::forward<FnT>(fns)...);
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename... FnT>
void TaggedSequencer<SeqTag, SeqTrigger>::parallel(
  SeqType const& seq_id, FnT&&... fns
) {
  bool const has_context = hasContext();

  vt_debug_print(
    sequence, node,
    "Sequencer: parallel: seq_id={}, has_context={}, num fns={}\n",
    seq_id, print_bool(has_context), sizeof...(fns)
  );

  SeqNodePtrType par_node = SeqNode::makeParallelNode(
    seq_id, convertSeqFun(seq_id, fns)...
  );

  return dispatch_parallel(has_context, seq_id, par_node);
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::dispatch_parallel(
  bool const& has_context, SeqType const& seq_id, SeqNodePtrType par_node
) {
  if (has_context) {
    // add to current inner node context container for sequence
    SeqNodePtrType node = getNode(seq_id);
    par_node->setParent(node);
    node->addSequencedParallelClosure(par_node);
  } else {
    // add to outer scope container for sequence
    SeqListType& lst = getSeqList(seq_id);
    lst.addNode(par_node);
    enqueueSeqList(seq_id);
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::enqueueSeqList(SeqType const& seq_id) {
  SeqListType& lst = getSeqList(seq_id);
  if (lst.isReady()) {
    auto run_list = [this,seq_id]{
      SeqListType& mylst = getSeqList(seq_id);
      if (mylst.isReady()) {
        mylst.expandNextNode();
      }
    };
    enqueue(run_list);
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
typename TaggedSequencer<SeqTag, SeqTrigger>::SeqType
TaggedSequencer<SeqTag, SeqTrigger>::getCurrentSeq() const {
  assertValidContext();
  return context_->getSeq();
}

template <typename SeqTag, template <typename> class SeqTrigger>
bool TaggedSequencer<SeqTag, SeqTrigger>::isLocalTerm() {
  for (auto&& live_seq : seq_lookup_) {
    if (live_seq.second.lst.size() > 0) {
      return false;
    }
  }
  return true;
}

template <typename SeqTag, template <typename> class SeqTrigger>
SeqNodePtrType TaggedSequencer<SeqTag, SeqTrigger>::getNode(
  SeqType const& id
) const {
  assertValidContext();
  return context_->getNode();
}

template <typename SeqTag, template <typename> class SeqTrigger>
typename TaggedSequencer<SeqTag, SeqTrigger>::SeqType
TaggedSequencer<SeqTag, SeqTrigger>::getSeqID() const {
  assertValidContext();
  return context_->getNode()->getSeqID();
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void TaggedSequencer<SeqTag, SeqTrigger>::wait_closure(
  SeqNonMigratableTriggerType<MessageT> trigger
) {
  return wait_closure(no_tag, trigger);
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void TaggedSequencer<SeqTag, SeqTrigger>::wait_closure(
  TagType const& tag, SeqNonMigratableTriggerType<MessageT> trigger
) {
  assertValidContext();
  return wait_on_trigger<MessageT, f>(
    tag, SeqActionType<MessageT>{getSeqID(),trigger}
  );
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void TaggedSequencer<SeqTag, SeqTrigger>::wait(SeqTriggerType<MessageT> trigger) {
  return wait<MessageT, f>(no_tag, trigger);
}


template <typename SeqTag, template <typename> class SeqTrigger>
template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void TaggedSequencer<SeqTag, SeqTrigger>::wait(
  TagType const& tag, SeqTriggerType<MessageT> trigger
) {
  assertValidContext();
  return wait_on_trigger<MessageT, f>(
    tag, SeqActionType<MessageT>{getSeqID(),trigger}
  );
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename MessageT, ActiveTypedFnType<MessageT>* f>
void TaggedSequencer<SeqTag, SeqTrigger>::wait_on_trigger(
  TagType const& tag, SeqActionType<MessageT> action
) {
  theTerm()->produce();

  assertValidContext();

  SeqType const seq_id = getSeqID();
  SeqNodePtrType node = getNode(seq_id);

  vtAssert(node != nullptr, "Must have node from context");

  bool const seq_ready = node->isReady();

  vt_debug_print(
    sequence, node,
    "Sequencer: wait: tag={}: context seq id={}, node={}, blocked={}, "
    "ready={}\n",
    tag, seq_id, PRINT_SEQ_NODE_PTR(node),
    print_bool(node->isBlockedNode()), print_bool(seq_ready)
  );

  auto deferred_wait_action = [tag,action,node,seq_id]() -> bool {
    bool const has_match = SeqStateMatcherType<MessageT,f>::hasMatchingMsg(tag);

    if (has_match) {
      auto msg = SeqStateMatcherType<MessageT, f>::getMatchingMsg(tag);
      action.runAction(msg);
    }

    vt_debug_print(
      sequence, node,
      "Sequencer: {}: tag={}: node={}, has_match={}, "
      "is_blocked={}\n",
      has_match ? "wait ran *immediately*" : "wait registered", tag,
      PRINT_SEQ_NODE_PTR(node), print_bool(has_match),
      print_bool(node->isBlockedNode())
    );

    bool const should_block = not has_match;

    node->setBlockedOnNode(eSeqConstructType::WaitConstruct, should_block);

    if (has_match) {
      vt_debug_print(
        sequence, node,
        "Sequencer: activating next node: seq={}, node={}, blocked={}\n",
        seq_id, PRINT_SEQ_NODE_PTR(node), print_bool(node->isBlockedNode())
      );

      if (node->isParallel()) {
        node->activate();
      }
    } else {
      // buffer the action to wait for a matching message

      auto msg_recv_trigger = [node,seq_id,action,tag](MessageT* msg){
        vt_debug_print(
          sequence, node,
          "Sequencer: msg_recv_trigger: seq={}, tag={}, node={}, blocked={}, "
          "msg={}\n",
          seq_id, tag, PRINT_SEQ_NODE_PTR(node),
          print_bool(node->isBlockedNode()), print_ptr(msg)
        );

        auto pmsg = promoteMsg(msg);

        action.runAction(pmsg, false);

        vtAssert(node != nullptr, "node must not be nullptr");

        node->setBlockedOnNode(eSeqConstructType::WaitConstruct, false);
        node->activate();
      };

      // buffer the action because there is not a matching message to trigger
      // the message
      SeqStateMatcherType<MessageT, f>::bufferUnmatchedAction(
        msg_recv_trigger, seq_id, tag
      );
    }

    return should_block;
  };

  bool should_suspend = false;

  if (seq_ready) {
    // run it here, right now
    bool const has_match = not deferred_wait_action();

    vt_debug_print(
      sequence, node,
      "Sequencer: executed wait: has_match={}: seq_id={}\n",
      print_bool(has_match), seq_id
    );
    should_suspend = not has_match;
  } else {
    should_suspend = true;

    vt_debug_print(
      sequence, node,
      "Sequencer: deferring wait: seq_id={}\n", seq_id
    );

    node->addSequencedClosure(deferred_wait_action);
  }

  if (should_suspend and context_->isSuspendable()) {
    vt_debug_print(
      sequence, node,
      "Sequencer: should suspend: seq_id={}, context suspendable={}\n",
      seq_id, print_bool(context_->isSuspendable())
    );

    context_->suspend();
  }
}

// should be made thread-safe and thread-local
template <typename SeqTag, template <typename> class SeqTrigger>
bool TaggedSequencer<SeqTag, SeqTrigger>::lookupContextExecute(
  SeqType const& id, SeqCtxFunctionType c
) {
  auto node_iter = node_lookup_.find(id);
  vtAssert(node_iter != node_lookup_.end(), "Must have valid node context");

  auto seq_node = node_iter->second;

  vt_debug_print(
    sequence, node,
    "Sequencer: lookupContextExecute (start): id={}: context={}, node={}\n",
    id, print_ptr(context_), PRINT_SEQ_NODE_PTR(seq_node)
  );

  bool const blocked = executeInNodeContext(id, seq_node, c);

  vt_debug_print(
    sequence, node,
    "Sequencer: lookupContextExecute (end): id={}: context={}, node={}, "
    "blocked={}\n",
    id, print_ptr(context_), PRINT_SEQ_NODE_PTR(seq_node), print_bool(blocked)
  );

  return blocked;
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::storeNodeContext(
  SeqType const& id, SeqNodePtrType node
) {
  auto iter = node_lookup_.find(id);
  if (iter == node_lookup_.end()) {
    node_lookup_[id] = node;
  } else {
    iter->second = node;
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
bool TaggedSequencer<SeqTag, SeqTrigger>::executeSuspendableContext(
  SeqType const& id, SeqNodePtrType node, SeqCtxFunctionType fn
) {
  return executeInNodeContext(id, node, fn, true);
}

template <typename SeqTag, template <typename> class SeqTrigger>
bool TaggedSequencer<SeqTag, SeqTrigger>::executeInNodeContext(
  SeqType const& id, SeqNodePtrType node, SeqCtxFunctionType c,
  bool const suspendable
) {
  SeqContextType* prev_context = context_;

  vt_debug_print(
    sequence, node,
    "Sequencer: executeInNodeContext (start): id={}: node={}\n",
    id, print_ptr(node.get())
  );

  // save this node related to `id' for later execution in this context
  storeNodeContext(id, node);

  SeqContextType new_context(id, node, suspendable);
  context_ = &new_context;
  if (suspendable) {
#if vt_check_enabled(fcontext)
    new_context.seq_ult->initialize(c);
    new_context.seq_ult->start();
#else
    vtAbort("Trying to use suspendable context without fcontext enabled!");
#endif
  } else {
    c();
  }
  context_ = prev_context;

  vt_debug_print(
    sequence, node,
    "Sequencer: executeInNodeContext (end): id={}: node={}\n",
    id, print_ptr(node.get())
  );

  return node->isBlockedNode();
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::enqueue(ActionType const& action) {
  vt_debug_print(
    sequence, node, "Sequencer: enqueue item\n"
  );

  theSched()->enqueue(action);
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename MessageT, ActiveTypedFnType<MessageT>* f>
/*static*/ void TaggedSequencer<SeqTag, SeqTrigger>::sequenceMsg(MessageT* msg) {
  auto const& is_tag_type = envelopeIsTagType(msg->env);
  TagType const& msg_tag = is_tag_type ? envelopeGetTag(msg->env) : no_tag;

  vt_debug_print(
    sequence, node,
    "sequenceMsg: arrived: msg={}, tag={}\n",
    print_ptr(msg), msg_tag
  );

  bool const has_match =
    SeqStateMatcherType<MessageT, f>::hasMatchingAction(msg_tag);

  vt_debug_print(
    sequence, node,
    "sequenceMsg: arriving: msg={}, has_match={}, tag={}\n",
    print_ptr(msg), print_bool(has_match), msg_tag
  );

  // reference the arrival message to keep it alive past normal lifetime, in
  // case it's buffered or put into the scheduler
  auto pmsg = promoteMsg(msg);

  if (has_match) {
    auto action = SeqStateMatcherType<MessageT, f>::getMatchingAction(msg_tag);
    auto handle_msg_action = [this,action,pmsg]{
      lookupContextExecute(action.seq_id, action.generateCallable(pmsg));
    };

    // skip the work queue and directly execute
    if (seq_skip_queue) {
      handle_msg_action();
    } else {
      theSeq()->enqueue(handle_msg_action);
    }
  } else {
    // nothing was found so the message must be buffered and wait an action
    // being posted

    // buffer the unmatched messaged until a trigger is posted for it that
    // matches
    SeqStateMatcherType<MessageT, f>::bufferUnmatchedMessage(pmsg, msg_tag);
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
typename TaggedSequencer<SeqTag, SeqTrigger>::SeqListType&
TaggedSequencer<SeqTag, SeqTrigger>::getSeqList(SeqType const& seq_id) {
  auto seq_iter = seq_lookup_.find(seq_id);
  vtAssert(
    seq_iter != seq_lookup_.end(), "This seq_id must exit"
  );
  return seq_iter->second;
}

// Give the static manager declaration linkage
template <typename SeqTag, template <typename> class SeqTrigger>
/*static*/ std::unique_ptr<typename TaggedSequencer<SeqTag, SeqTrigger>::SeqManagerType>
  TaggedSequencer<SeqTag, SeqTrigger>::seq_manager =
    std::make_unique<typename TaggedSequencer<SeqTag, SeqTrigger>::SeqManagerType>();

}} //end namespace vt::seq

#endif /*INCLUDED_SEQUENCE_SEQUENCER_IMPL_H*/

