
#if ! defined __RUNTIME_TRANSPORT_SEQUENCE__
#define __RUNTIME_TRANSPORT_SEQUENCE__

#include "config.h"
#include "message.h"
#include "active.h"
#include "termination.h"

#include "seq_common.h"
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
struct TaggedSequencer {
  using SeqType = SeqTag;
  using SeqListType = SeqList;
  using SeqParallelType = SeqParallel;
  template <typename MessageT>
  using SeqActionType = Action<MessageT>;
  template <typename MessageT>
  using SeqTriggerType = SeqTrigger<MessageT>;
  template <typename T>
  using SeqIDContainerType = std::unordered_map<SeqType, T>;
  using SeqFunType = SeqListType::SeqFunType;
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

  static SeqFunType convertSeqFun(UserSeqFunType const& fn) {
    return [=]() -> bool { return fn(), true; };
  }

  void sequencedBlock(UserSeqFunType const& fn) {
    assert(
      stateful_seq_ != no_seq and "Must be in a valid sequence"
    );
    return attachNext(stateful_seq_,fn);
  };

  void sequenced(SeqType const& seq_id, UserSeqFunWithIDType const& fn) {
    auto sfn = [=]{ fn(seq_id); };
    return sequenced(seq_id, fn);
  }

  void sequenced(SeqType const& seq_id, UserSeqFunType const& fn) {
    SeqListType& lst = getSeqList(seq_id);
    lst.addAction(convertSeqFun(fn));
  }

  void parallel(
    SeqType const& seq_id, UserSeqFunType const& fn1, UserSeqFunType const& fn2
  ) {
    SeqListType& lst = getSeqList(seq_id);
    lst.addNode(
      SeqNode::makeParallelNode(convertSeqFun(fn2), convertSeqFun(fn2))
    );

    // auto fn_wrapper = [=]() -> bool {
    //   SeqParallelType seq_par(nullptr, fn1, fn2);
    //   seq_par.unravelParallelRegion();
    //   return false;
    // };

    // SeqListType& lst = getSeqList(seq_id);

    // lst.addAction([=]() -> bool {
    //   return executeInContext(seq_id, false, fn_wrapper);
    // });
  }

  SeqType get_current_seq() const {
    return stateful_seq_;
  }

  bool scheduler() {
    for (auto&& live_seq : seq_lookup_) {
      live_seq.second.progress();
    }
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

    debug_print(
      sequence, node,
      "wait called with tag=%d: is_expanding_sequenced_=%s\n",
      tag, print_bool(is_expanding_sequenced_)
    );

    assert(
      stateful_seq_ != no_seq and "Must have valid seq now"
    );

    SeqListType& lst = getSeqList(stateful_seq_);
    SeqListType* const lst_ptr = &lst;

    auto const cur_seq_id = stateful_seq_;

    auto action = SeqActionType<MessageT>{cur_seq_id,trigger};

    bool const cur_seq = is_expanding_sequenced_;

    auto deferred_wait_action =
      [tag,trigger,lst_ptr,cur_seq_id,action,cur_seq]() -> bool {

      debug_print(
        sequence, node,
        "wait registered for tag=%d: cur_seq=%s\n", tag, print_bool(cur_seq)
      );

      auto apply_func = [=](MessageT* msg){
        action.runAction(msg);
        messageDeref(msg);
      };

      bool const has_match =
        SeqStateMatcherType<MessageT, f>::findMatchingMsg(apply_func, tag);

      if (not has_match) {
        auto ready_trigger = [lst_ptr,trigger](MessageT* msg){
          trigger(msg);
          lst_ptr->makeReady();
        };

        // buffer the action because there is not a matching message to trigger
        // the message
        SeqStateMatcherType<MessageT, f>::bufferUnmatchedAction(
          ready_trigger, cur_seq_id, tag
        );
      }

      bool const should_block = not has_match and cur_seq;

      return should_block;
    };

    assert(
      stateful_seq_ != no_seq and "Must be in an active sequence context"
    );

    lst.addAction([=]() -> bool {
      return executeInContext(stateful_seq_, cur_seq, deferred_wait_action);
    });
  }

  template <typename Callable>
  auto executeInContext(
    SeqType const& context, bool const& in_sequence, Callable&& c
  ) {
    bool prev_val = is_expanding_sequenced_;
    // set up context for execution
    is_expanding_sequenced_ = in_sequence;
    stateful_seq_ = context;

    auto const& ret = c();

    // return context back to previous form
    stateful_seq_ = no_seq;
    is_expanding_sequenced_ = prev_val;
    return ret;
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

    // nothing was found so the message must be buffered and wait an action
    // being posted
    if (not has_match) {
      // reference the arrival message to keep it alive past normal lifetime
      messageRef(msg);

      // buffer the unmatched messaged until a trigger is posted for it that
      // matches
      SeqStateMatcherType<MessageT, f>::bufferUnmatchedMessage(msg, msg_tag);
    }

    debug_print(
      sequence, node,
      "sequenceMsg: arriving: msg=%p, has_match=%s, tag=%d\n",
      msg, print_bool(has_match), msg_tag
    );
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
  bool is_expanding_sequenced_ = true;

  SeqType stateful_seq_ = no_seq;

  SeqIDContainerType<SeqListType> seq_lookup_;
};

using Sequencer = TaggedSequencer<SeqType, SeqMigratableTriggerType>;

#define SEQUENCE_REGISTER_HANDLER(message, handler)                     \
  static void handler(message* m) {                                     \
    theSeq->sequenceMsg<message, handler>(m);                           \
  }

}} //end namespace vt::seq

namespace vt {

extern std::unique_ptr<seq::Sequencer> theSeq;

//using SequenceMessage = seq::SeqMsg;

} //end namespace vt

#endif /*__RUNTIME_TRANSPORT_SEQUENCE__*/

