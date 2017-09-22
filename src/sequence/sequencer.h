
#if ! defined __RUNTIME_TRANSPORT_SEQUENCE__
#define __RUNTIME_TRANSPORT_SEQUENCE__

#include "config.h"
#include "message.h"
#include "active.h"
#include "termination.h"
#include "concurrent_deque.h"

#include "seq_common.h"
#include "seq_context.h"
#include "seq_node.h"
#include "seq_list.h"
#include "seq_state.h"
#include "seq_matcher.h"
#include "seq_action.h"
#include "seq_parallel.h"

#include <unordered_map>
#include <list>
#include <cassert>

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
struct TaggedSequencer;

} // end namespace seq

extern std::unique_ptr<seq::TaggedSequencer<SeqType, seq::SeqMigratableTriggerType>> theSeq;

}  // end namespace vt

namespace vt { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
struct TaggedSequencer {
  using SeqType = SeqTag;
  using SeqListType = SeqList;
  using SeqContextType = SeqContext;
  using SeqParallelType = SeqParallel;
  using SeqFunType = SeqListType::SeqFunType;
  using SeqContextPtrType = SeqContextType*;
  using SeqContextContainerType = std::unordered_map<SeqType, SeqContextPtrType>;

  template <typename MessageT>
  using SeqActionType = Action<MessageT>;

  template <typename MessageT>
  using SeqTriggerType = SeqTrigger<MessageT>;

  template <typename T>
  using SeqIDContainerType = std::unordered_map<SeqType, T>;

  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  using SeqStateMatcherType = SeqMatcher<MessageT, f>;

  TaggedSequencer() = default;

  SeqType nextSeq() {
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

  static SeqFunType convertSeqFun(SeqType const& id, UserSeqFunType fn)  {
    return [=]() -> bool {
      return theSeq->executeInContext(id, true, fn);
    };
  }

  void assertValidContext() const {
    assert(context_ != nullptr and "Must be in a valid sequence");
  }

  void sequencedBlock(UserSeqFunType const& fn) {
    assertValidContext();
    return attachNext(context_->getSeq(), fn);
  };

  void sequenced(SeqType const& seq_id, UserSeqFunWithIDType const& fn) {
    debug_print(
      sequence, node,
      "sequenced (UserSeqFunWithIDType) seq_id=%d\n", seq_id
    );

    auto sfn = [=]{ fn(seq_id); };
    return sequenced(seq_id, sfn);
  }

  void sequenced(SeqType const& seq_id, UserSeqFunType const& fn) {
    debug_print(
      sequence, node,
      "sequenced (UserSeqFunType) seq_id=%d\n", seq_id
    );

    SeqListType& lst = getSeqList(seq_id);
    lst.addAction(convertSeqFun(seq_id, fn));
    checkReadySeqList(seq_id);
  }

  void parallel(
    SeqType const& seq_id, UserSeqFunType const& fn1, UserSeqFunType const& fn2
  ) {
    debug_print(
      sequence, node,
      "parallel seq_id=%d\n", seq_id
    );

    SeqListType& lst = getSeqList(seq_id);
    lst.addNode(
      SeqNode::makeParallelNode(
        seq_id, convertSeqFun(seq_id, fn2), convertSeqFun(seq_id, fn2)
      )
    );
    checkReadySeqList(seq_id);
  }

  void checkReadySeqList(SeqType const& seq_id) {
    SeqListType& lst = getSeqList(seq_id);
    if (lst.isReady()) {
      lst.expandNextNode();
    }
  }

  void activeNextSeq(SeqType const& seq_id) {
    // SeqListType& lst = getSeqList(seq_id);
    // lst.expandNextNode();
  }

  SeqType getCurrentSeq() const {
    assertValidContext();
    return context_->getSeq();
  }

  bool scheduler() {
    // for (auto&& live_seq : seq_lookup_) {
    //   live_seq.second.progress();
    // }
    return false;
  }

  bool isLocalTerm() {
    for (auto&& live_seq : seq_lookup_) {
      if (live_seq.second.lst.size() > 0) {
        return false;
      }
    }
    return true;
  }

  void setNode(SeqType const& id, SeqNodePtrType node) {
    auto iter = context_lookup_.find(id);
    if (iter == context_lookup_.end()) {
      context_lookup_[id] = new SeqContext(id);
      iter = context_lookup_.find(id);
    }
    iter->second->setNode(node);
  }

  SeqNodePtrType getNode(SeqType const& id) const {
    auto iter = context_lookup_.find(id);
    return iter == context_lookup_.end() ? nullptr : iter->second->getNode();
  }

  bool getSeqReady(SeqType const& id) {
    auto const& node = getNode(id);
    return node ? node->isReady() : (assert(0), false);
  }

  void setSeqReady(SeqType const& id, bool const& ready) {
    auto const& node = getNode(id);
    assert(node != nullptr and "Node must exist");
    node->setReady(ready);
  }

  template <
    typename MessageT,
    ActiveAnyFunctionType<MessageT>* f,
    ActiveAnyFunctionType<MessageT>* trigger
  >
  void wait(TagType const& tag = no_tag) {
    /*
     * Overload for a migratable variant---for now just forward to
     * non-migratable
     */

    // auto const& local_han = auto_registry::make_auto_handler<
    //   MessageT, trigger
    // >(nullptr);

    return wait<MessageT,f>(tag,trigger);
  }

  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void wait(SeqTriggerType<MessageT> trigger) {
    return wait<MessageT, f>(no_tag, trigger);
  }

  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void wait(TagType const& tag, SeqTriggerType<MessageT> trigger) {
    /*
     * Migratablity---this wait variant is not migratable, due to the
     * non-registration of trigger
     */

    theTerm->produce();

    assertValidContext();

    SeqType const seq_id = context_->getSeq();
    SeqNodePtrType node = getNode(seq_id);
    bool const seq_ready = getSeqReady(seq_id);
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
        auto ready_trigger = [node,seq_id,trigger](MessageT* msg){
          auto is_blocked = node->isBlockedNode();

          debug_print(
            sequence, node,
            "Sequencer: ready trigger: seq_id=%d, node=%p, is_blocked=%s\n",
            seq_id, PRINT_SEQ_NODE_PTR(node), print_bool(is_blocked)
          );

          trigger(msg);

          assert(node != nullptr and "node must not be nullptr");

          node->setBlockedOnNode(eSeqConstructType::WaitConstruct, false);
          node->activate();
        };

        // buffer the action because there is not a matching message to trigger
        // the message
        SeqStateMatcherType<MessageT, f>::bufferUnmatchedAction(
          ready_trigger, seq_id, tag
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
      node->addSequencedWait(deferred_wait_action);
    }
  }

  // should be made thread-safe and thread-local
  template <typename Callable>
  bool executeInContext(
    SeqType const& id, bool const& in_sequence, Callable&& c
  ) {
    SeqContextType local_context(id);
    context_ = &local_context;
    c();
    context_ = nullptr;
    return true;
  }

private:
  void attachNext(SeqType const& seq_id, UserSeqFunWithIDType const& fn) {
    return attachNext(seq_id, [seq_id,fn]{
      return fn(seq_id);
    });
  }

  void attachNext(SeqType const& seq_id, UserSeqFunType const& fn) {
    SeqListType& lst = getSeqList(seq_id);
    auto fn_wrapper = [=]() -> bool {
      fn();
      return false;
    };
    lst.addAction([=]() -> bool {
      bool const in_seq = true;
      return executeInContext(seq_id, in_seq, fn_wrapper);
    });
  }

public:
  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void sequenceMsg(MessageT* msg) {
    auto const& is_tag_type = envelopeIsTagType(msg->env);

    TagType const& msg_tag = is_tag_type ? envelopeGetTag(msg->env) : no_tag;

    auto apply_func = [=](Action<MessageT>& action){
      executeInContext(action.seq_id, false, action.generateCallable(msg));
    };

    bool const has_match =
      SeqStateMatcherType<MessageT, f>::findMatchingAction(apply_func, msg_tag);

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
    } else {
      // trigger the next continuation
    }
  }

private:
  SeqListType& getSeqList(SeqType const& seq_id) {
    auto seq_iter = seq_lookup_.find(seq_id);
    assert(
      seq_iter != seq_lookup_.end() and "This seq_id must exit"
    );
    return seq_iter->second;
  }

private:
  SeqContextContainerType context_lookup_;

  SeqContext* context_ = nullptr;

  SeqIDContainerType<SeqListType> seq_lookup_;

  util::container::ConcurrentDeque<ActionType> work_deque_;
};


inline void setNode(SeqType const& id, SeqNodePtrType node) {
  theSeq->setNode(id, node);
}

inline void seqProgress(SeqType const& id) {
  theSeq->checkReadySeqList(id);
}

inline void setSeqReady(SeqType const& id, bool const& ready) {
  theSeq->setSeqReady(id, ready);
}

using Sequencer = TaggedSequencer<SeqType, SeqMigratableTriggerType>;

#define SEQUENCE_REGISTER_HANDLER(message, handler)                     \
  static void handler(message* m) {                                     \
    theSeq->sequenceMsg<message, handler>(m);                           \
  }

}} //end namespace vt::seq

namespace vt {

extern std::unique_ptr<seq::Sequencer> theSeq;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_SEQUENCE__*/

