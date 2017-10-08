
#if ! defined __RUNTIME_TRANSPORT_SEQUENCER_VIRTUAL_IMPL__
#define __RUNTIME_TRANSPORT_SEQUENCER_VIRTUAL_IMPL__

#include "config.h"
#include "sequencer.h"
#include "context/context_vrt.h"
#include "context/context_vrtmanager.h"

#include <cassert>
#include <cstdint>

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
/*virtual*/ typename TaggedSequencerVrt<SeqTag, SeqTrigger>::SeqType
TaggedSequencerVrt<SeqTag, SeqTrigger>::getNextID() {
  return this->seq_manager->nextSeqID(true);
}

template <typename SeqTag, template <typename> class SeqTrigger>
typename TaggedSequencerVrt<SeqTag, SeqTrigger>::SeqType
TaggedSequencerVrt<SeqTag, SeqTrigger>::createSeqVrtContext(
  VrtContext_ProxyType const& proxy
) {
  debug_print(
    sequence_vrt, node,
    "SequencerVirtual: createSeqVrtContextt\n"
  );

  auto vrt_context = theVrtCManager->getVrtContextByProxy(proxy);

  debug_print(
    sequence_vrt, node,
    "SequencerVirtual: lookup virtual context: vrt_context=%p\n",
    vrt_context
  );

  assert(
    vrt_context != nullptr and "Virtual context must exist locally"
  );

  SeqType const& next_seq = this->createSeq();

  debug_print(
    sequence_vrt, node,
    "SequencerVirtual: createSeqVrtContextt: new seq_id=%d\n", next_seq
  );

  auto seq_iter = seq_vrt_lookup_.find(next_seq);

  assert(
    seq_iter == seq_vrt_lookup_.end() and "Must not exist"
  );

  seq_vrt_lookup_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(next_seq),
    std::forward_as_tuple(VirtualInfoType{proxy})
  );

  return next_seq;
}

template <typename SeqTag, template <typename> class SeqTrigger>
VrtContext_ProxyType TaggedSequencerVrt<SeqTag, SeqTrigger>::getCurrentVrtProxy() {
  this->assertValidContext();

  auto cur_seq_id = this->context_->getSeq();

  debug_print(
    sequence_vrt, node,
    "SequencerVirtual: getCurrentVrtProxy: seq=%d\n", cur_seq_id
  );

  auto seq_iter = seq_vrt_lookup_.find(cur_seq_id);

  assert(
    seq_iter != seq_vrt_lookup_.end() and "Must exist"
  );

  return seq_iter->second.proxy;
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
void TaggedSequencerVrt<SeqTag, SeqTrigger>::sequenceVrtMsg(
  MsgT* msg, VcT* vrt
) {
  auto const& is_tag_type = envelopeIsTagType(msg->env);
  TagType const& msg_tag = is_tag_type ? envelopeGetTag(msg->env) : no_tag;

  debug_print(
    sequence, node,
    "sequenceVrtMsg: arrived: msg=%p, tag=%d\n", msg, msg_tag
  );

  bool const has_match =
    SeqStateMatcherType<VcT, MsgT, f>::hasMatchingAction(msg_tag);

  debug_print(
    sequence, node,
    "sequenceVrtMsg: arriving: msg=%p, has_match=%s, tag=%d\n",
    msg, print_bool(has_match), msg_tag
  );

  if (has_match) {
    auto action = SeqStateMatcherType<VcT, MsgT, f>::getMatchingAction(msg_tag);
    auto handle_msg_action = [this,action,msg,vrt]{
      this->lookupContextExecute(
        action.seq_id, action.generateCallable(msg, vrt)
      );
    };

    // skip the work queue and directly execute
    if (seq_skip_queue) {
      handle_msg_action();
    } else {
      theVrtSeq->enqueue(handle_msg_action);
    }
  } else {
    // nothing was found so the message must be buffered and wait an action
    // being posted

    // reference the arrival message to keep it alive past normal lifetime
    messageRef(msg);

    // buffer the unmatched messaged until a trigger is posted for it that
    // matches
    SeqStateMatcherType<VcT, MsgT, f>::bufferUnmatchedMessage(msg, msg_tag);
  }
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
void TaggedSequencerVrt<SeqTag, SeqTrigger>::wait(
  TagType const& tag, SeqTriggerType<MsgT, VcT> trigger
) {
  this->assertValidContext();
  return wait_on_trigger<VcT, MsgT, f>(
    tag, SeqActionType<MsgT, VcT>{this->getSeqID(),trigger}
  );
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
void TaggedSequencerVrt<SeqTag, SeqTrigger>::wait(
  SeqTriggerType<MsgT, VcT> trigger
) {
  return wait<VcT, MsgT, f>(no_tag, trigger);
}

template <typename SeqTag, template <typename> class SeqTrigger>
template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
void TaggedSequencerVrt<SeqTag, SeqTrigger>::wait_on_trigger(
  TagType const& tag, SeqActionType<MsgT, VcT> action
) {
  theTerm->produce();

  this->assertValidContext();

  SeqType const seq_id = this->getSeqID();
  SeqNodePtrType node = this->getNode(seq_id);

  assert(node != nullptr and "Must have node from context");

  bool const seq_ready = node->isReady();

  debug_print(
    sequence_vrt, node,
    "SequencerVrt: wait: tag=%d: context seq id=%d, node=%p, blocked=%s, "
    "ready=%s\n",
    tag, seq_id, PRINT_SEQ_NODE_PTR(node),
    print_bool(node->isBlockedNode()), print_bool(seq_ready)
  );

  auto deferred_wait_action = [tag,action,node,seq_id]() -> bool {
    bool const has_match = SeqStateMatcherType<VcT,MsgT,f>::hasMatchingMsg(tag);

    if (has_match) {
      auto msg = SeqStateMatcherType<VcT, MsgT, f>::getMatchingMsg(tag);

      auto const& cur_proxy = theVrtSeq->getCurrentVrtProxy();
      auto vrt_context = theVrtCManager->getVrtContextByProxy(cur_proxy);
      action.runAction(static_cast<VcT*>(vrt_context), msg);
      messageDeref(msg);
    }

    debug_print(
      sequence_vrt, node,
      "SequencerVrt: %s: tag=%d: node=%p, has_match=%s, "
      "is_blocked=%s\n",
      has_match ? "wait ran *immediately*" : "wait registered", tag,
      PRINT_SEQ_NODE_PTR(node), print_bool(has_match),
      print_bool(node->isBlockedNode())
    );

    bool const should_block = not has_match;

    node->setBlockedOnNode(eSeqConstructType::WaitConstruct, should_block);

    if (has_match) {
      debug_print(
        sequence_vrt, node,
        "SequencerVrt: activating next node: seq=%d, node=%p, blocked=%s\n",
        seq_id, PRINT_SEQ_NODE_PTR(node), print_bool(node->isBlockedNode())
      );

      node->activate();
    } else {
      // buffer the action to wait for a matching message

      auto msg_recv_trigger = [node,seq_id,action,tag](MsgT* msg, VcT* vc){
        debug_print(
          sequence_vrt, node,
          "SequencerVrt: msg_recv_trigger: seq=%d, tag=%d, node=%p, blocked=%s, "
          "msg=%p\n",
          seq_id, tag, PRINT_SEQ_NODE_PTR(node),
          print_bool(node->isBlockedNode()), msg
        );

        action.runAction(vc, msg);

        assert(node != nullptr and "node must not be nullptr");

        node->setBlockedOnNode(eSeqConstructType::WaitConstruct, false);
        node->activate();
      };

      // buffer the action because there is not a matching message to trigger
      // the message
      SeqStateMatcherType<VcT, MsgT, f>::bufferUnmatchedAction(
        msg_recv_trigger, seq_id, tag
      );
    }

    return should_block;
  };

  if (seq_ready) {
    // run it here, right now
    bool const has_match = not deferred_wait_action();

    debug_print(
      sequence_vrt, node,
      "SequencerVrt: executed wait: has_match=%s: seq_id=%d\n",
      print_bool(has_match), seq_id
    );
  } else {
    node->addSequencedClosure(deferred_wait_action);
  }
}

}} //end namespace vt::seq

#endif /*__RUNTIME_TRANSPORT_SEQUENCER_VIRTUAL_IMPL__*/
