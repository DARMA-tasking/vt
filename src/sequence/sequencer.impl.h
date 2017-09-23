
#if ! defined __RUNTIME_TRANSPORT_SEQUENCE_IMPL__
#define __RUNTIME_TRANSPORT_SEQUENCE_IMPL__

#include "config.h"
#include "seq_common.h"
#include "sequencer.h"

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
typename TaggedSequencer<SeqTag, SeqTrigger>::SeqType
TaggedSequencer<SeqTag, SeqTrigger>::nextSeq() {
  auto const cur_id = next_seq_id;

  auto seq_iter = seq_lookup_.find(cur_id);

  assert(
    seq_iter == seq_lookup_.end() and "New seq_id should not exist now"
  );

  seq_lookup_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(cur_id),
    std::forward_as_tuple(SeqListType{cur_id})
  );

  next_seq_id++;

  return cur_id;
}

template <typename SeqTag, template <typename> class SeqTrigger>
/*static*/
typename TaggedSequencer<SeqTag, SeqTrigger>::SeqFunType
TaggedSequencer<SeqTag, SeqTrigger>::convertSeqFun(
  SeqType const& id, UserSeqFunType fn
)  {
  return [=]() -> bool {
    return theSeq->lookupContextExecute(id, fn);
  };
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::assertValidContext() const {
  assert(hasContext() and "Must be in a valid sequence");
}

template <typename SeqTag, template <typename> class SeqTrigger>
bool TaggedSequencer<SeqTag, SeqTrigger>::hasContext() const {
  return context_ != nullptr;
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::sequenced(UserSeqFunType const& fn) {
  assertValidContext();

  debug_print(
    sequence, node,
    "Sequencer: sequenced: fn: context_=%p\n", context_
  );

  return sequenced(context_->getSeq(), fn);
};

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::sequenced(
  SeqType const& seq_id, UserSeqFunWithIDType const& fn
) {
  debug_print(
    sequence, node,
    "Sequencer: sequenced (UserSeqFunWithIDType) seq_id=%d\n", seq_id
  );

  auto sfn = [=]{ fn(seq_id); };
  return sequenced(seq_id, sfn);
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::sequenced(
  SeqType const& seq_id, UserSeqFunType const& fn
) {
  bool const has_context = hasContext();

  debug_print(
    sequence, node,
    "Sequencer: sequenced (UserSeqFunType) seq_id=%d: has_context=%s\n",
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
    checkReadySeqList(seq_id);
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::parallel(
  SeqType const& seq_id, UserSeqFunType const& fn1, UserSeqFunType const& fn2
) {
  debug_print(
    sequence, node,
    "Sequencer: parallel seq_id=%d\n", seq_id
  );

  SeqListType& lst = getSeqList(seq_id);
  lst.addNode(
    SeqNode::makeParallelNode(
      seq_id, convertSeqFun(seq_id, fn2), convertSeqFun(seq_id, fn2)
    )
  );
  checkReadySeqList(seq_id);
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::checkReadySeqList(SeqType const& seq_id) {
  SeqListType& lst = getSeqList(seq_id);
  if (lst.isReady()) {
    lst.expandNextNode();
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
typename TaggedSequencer<SeqTag, SeqTrigger>::SeqType
TaggedSequencer<SeqTag, SeqTrigger>::getCurrentSeq() const {
  assertValidContext();
  return context_->getSeq();
}

template <typename SeqTag, template <typename> class SeqTrigger>
bool TaggedSequencer<SeqTag, SeqTrigger>::scheduler() {
  // for (auto&& live_seq : seq_lookup_) {
  //   live_seq.second.progress();
  // }
  return false;
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
template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
void TaggedSequencer<SeqTag, SeqTrigger>::wait(SeqTriggerType<MessageT> trigger) {
  return wait<MessageT, f>(no_tag, trigger);
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
void TaggedSequencer<SeqTag, SeqTrigger>::wait(
  TagType const& tag, SeqTriggerType<MessageT> trigger
) {
  /*
   * Migratablity---this wait variant is not migratable, due to the
   * non-registration of trigger
   */

  theTerm->produce();

  assertValidContext();

  SeqType const seq_id = getSeqID();
  SeqNodePtrType node = getNode(seq_id);

  assert(node != nullptr and "Must have node from context");

  bool const seq_ready = node->isReady();
  bool const cur_seq = true;

  debug_print(
    sequence, node,
    "Sequencer: wait: f=%p tag=%d: context seq id=%d, node=%p, blocked=%s, "
    "ready=%s\n",
    f, tag, seq_id, PRINT_SEQ_NODE_PTR(node),
    print_bool(node->isBlockedNode()), print_bool(seq_ready)
  );

  auto action = SeqActionType<MessageT>{seq_id,trigger};

  auto deferred_wait_action = [tag,trigger,node,seq_id,action,cur_seq]() -> bool {
    auto apply_func = [=](MessageT* msg){
      action.runAction(msg);
      messageDeref(msg);
    };

    bool const has_match =
    SeqStateMatcherType<MessageT, f>::findMatchingMsg(apply_func, tag);

    auto const is_blocked = node->isBlockedNode();

    debug_print(
      sequence, node,
      "Sequencer: wait registered: tag=%d: cur_seq=%s, node=%p, "
      "has_match=%s, is_blocked=%s\n",
      tag, print_bool(cur_seq), PRINT_SEQ_NODE_PTR(node),
      print_bool(has_match), print_bool(is_blocked)
    );

    if (not has_match) {
      auto msg_recv_trigger = [node,seq_id,trigger](MessageT* msg){
        auto is_blocked = node->isBlockedNode();

        debug_print(
          sequence, node,
          "Sequencer: msg_recv_trigger: seq=%d, node=%p, blocked=%s, msg=%p\n",
          seq_id, PRINT_SEQ_NODE_PTR(node), print_bool(is_blocked), msg
        );

        trigger(msg);

        assert(node != nullptr and "node must not be nullptr");

        node->setBlockedOnNode(eSeqConstructType::WaitConstruct, false);
        node->activate();
      };

      // buffer the action because there is not a matching message to trigger
      // the message
      SeqStateMatcherType<MessageT, f>::bufferUnmatchedAction(
        msg_recv_trigger, seq_id, tag
      );
    }

    bool const should_block = not has_match and cur_seq;

    node->setBlockedOnNode(eSeqConstructType::WaitConstruct, should_block);

    return should_block;
  };

  if (seq_ready) {
    // run it here, right now
    bool const has_match = not deferred_wait_action();

    debug_print(
      sequence, node,
      "Sequencer: executed wait: has_match=%s: seq_id=%d\n",
      print_bool(has_match), seq_id
    );
  } else {
    node->addSequencedClosure(deferred_wait_action);
  }
}

// should be made thread-safe and thread-local
template <typename SeqTag, template <typename> class SeqTrigger>
template <typename Callable>
bool TaggedSequencer<SeqTag, SeqTrigger>::lookupContextExecute(
  SeqType const& id, Callable&& c
) {
  auto node_iter = node_lookup_.find(id);
  assert(node_iter != node_lookup_.end() and "Must have valid node context");

  auto seq_node = node_iter->second;

  debug_print(
    sequence, node,
    "Sequencer: lookupContextExecute (start): id=%d: context=%p, node=%p\n",
    id, context_, PRINT_SEQ_NODE_PTR(seq_node)
  );

  bool const blocked = executeInNodeContext(id, seq_node, c);

  debug_print(
    sequence, node,
    "Sequencer: lookupContextExecute (end): id=%d: context=%p, node=%p, "
    "blocked=%s\n",
    id, context_, PRINT_SEQ_NODE_PTR(seq_node), print_bool(blocked)
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
template <typename Fn>
bool TaggedSequencer<SeqTag, SeqTrigger>::executeInNodeContext(
  SeqType const& id, SeqNodePtrType node, Fn&& c
) {
  SeqContextType* prev_context = context_;

  debug_print(
    sequence, node,
    "Sequencer: executeInNodeContext (start): id=%d: node=%p\n", id, node.get()
  );

  // save this node related to `id' for later execution in this context
  storeNodeContext(id, node);

  SeqContextType new_context(id, node);
  context_ = &new_context;
  c();
  context_ = prev_context;

  debug_print(
    sequence, node,
    "Sequencer: executeInNodeContext (end): id=%d: node=%p\n", id, node.get()
  );

  return node->isBlockedNode();
}

template <typename SeqTag, template <typename> class SeqTrigger>
void TaggedSequencer<SeqTag, SeqTrigger>::enqueue(ActionType const& action) {
  work_deque_.pushBack(action);
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
/*static*/ void TaggedSequencer<SeqTag, SeqTrigger>::sequenceMsg(MessageT* msg) {
  auto const& is_tag_type = envelopeIsTagType(msg->env);
  TagType const& msg_tag = is_tag_type ? envelopeGetTag(msg->env) : no_tag;

  debug_print(
    sequence, node,
    "sequenceMsg: arrived: msg=%p, tag=%d\n", msg, msg_tag
  );

  auto apply_func = [=](Action<MessageT>& action){
    lookupContextExecute(action.seq_id, action.generateCallable(msg));
  };

  bool const apply_action_immediately = true;
  bool has_match = false;

  // skip the work queue and directly execute
  if (apply_action_immediately) {
    has_match = SeqStateMatcherType<MessageT, f>::findMatchingAction(
      apply_func, msg_tag
    );
  } else {
    has_match = SeqStateMatcherType<MessageT, f>::hasMatchingAction(msg_tag);

    if (has_match) {
      auto match = SeqStateMatcherType<MessageT, f>::getMatchingAction(msg_tag);
      auto handle_msg_action = [=]{
        lookupContextExecute(match.seq_id, match.generateCallable(msg));
      };
      theSeq->enqueue(handle_msg_action);
    }
  }

  debug_print(
    sequence, node,
    "sequenceMsg: arriving: msg=%p, has_match=%s, tag=%d\n",
    msg, print_bool(has_match), msg_tag
  );

  // nothing was found so the message must be buffered and wait an action
  // being posted
  if (not has_match) {
    // reference the arrival message to keep it alive past normal lifetime
    messageRef(msg);

    // buffer the unmatched messaged until a trigger is posted for it that
    // matches
    SeqStateMatcherType<MessageT, f>::bufferUnmatchedMessage(msg, msg_tag);
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
typename TaggedSequencer<SeqTag, SeqTrigger>::SeqListType&
TaggedSequencer<SeqTag, SeqTrigger>::getSeqList(SeqType const& seq_id) {
  auto seq_iter = seq_lookup_.find(seq_id);
  assert(
    seq_iter != seq_lookup_.end() and "This seq_id must exit"
  );
  return seq_iter->second;
}

template <typename Fn>
bool executeSeqExpandContext(SeqType const& id, SeqNodePtrType node, Fn&& fn) {
  return theSeq->executeInNodeContext(id, node, fn);
}

}} //end namespace vt::seq

#endif /*__RUNTIME_TRANSPORT_SEQUENCE_IMPL_*/

