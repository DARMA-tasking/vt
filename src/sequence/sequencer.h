
#if ! defined __RUNTIME_TRANSPORT_SEQUENCE__
#define __RUNTIME_TRANSPORT_SEQUENCE__

#include "common.h"
#include "message.h"
#include "active.h"
#include "termination.h"

#include "seq_common.h"
#include "seq_list.h"
#include "seq_state.h"
#include "seq_action.h"

#include <unordered_map>
#include <list>
#include <cassert>

namespace runtime { namespace seq {

template <typename SeqTag, template <typename> class SeqTrigger>
struct TaggedSequencer {
  using SeqType = SeqTag;
  using SeqListType = SeqList;

  template <typename MessageT>
  using SeqActionType = Action<MessageT>;

  template <typename MessageT>
  using SeqTriggerType = SeqTrigger<MessageT>;

  template <typename T>
  using SeqIDContainerType = std::unordered_map<SeqType, T>;

  using SeqFunType = SeqListType::SeqFunType;

  TaggedSequencer() = default;

  SeqType next_seq() {
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

  void sequenced(SeqType const& seq_id, UserSeqFunWithIDType const& fn) {
    return attach_next(seq_id,fn);
  }

  void sequenced(SeqType const& seq_id, UserSeqFunType const& fn) {
    return attach_next(seq_id,fn);
  }

  void sequenced_block(UserSeqFunType const& fn) {
    assert(
      stateful_seq_ != no_seq and "Must be in a valid sequence"
    );
    return attach_next(stateful_seq_,fn);
  };

  SeqType get_current_seq() const {
    return stateful_seq_;
  }

  bool scheduler() {
    for (auto&& live_seq : seq_lookup_) {
      live_seq.second.progress();
    }
    return false;
  }

  bool is_local_term() {
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

    the_term->produce();

    debug_print(
      sequence, node,
      "wait called with tag=%d\n", tag
    );

    assert(
      stateful_seq_ != no_seq and "Must have valid seq now"
    );

    SeqListType& lst = get_seq_list(stateful_seq_);
    SeqListType* const lst_ptr = &lst;

    auto const cur_seq_id = stateful_seq_;

    auto action = SeqActionType<MessageT>{cur_seq_id,trigger};

    auto deferred_wait_action = [tag,trigger,lst_ptr,cur_seq_id,action]() -> bool {
      debug_print(
        sequence, node,
        "wait registered for tag=%d\n", tag
      );

      bool found_matching = false;

      if (tag == no_tag) {
        auto& lst = SeqStateType<MessageT,f>::seq_msg;
        if (lst.size() > 0) {
          auto msg = lst.front();
          lst.pop_front();
          action.run_action(msg);
          message_deref(msg);
          found_matching = true;
        }
      } else {
        auto& tagged_lst = SeqStateType<MessageT, f>::seq_msg_tagged;
        auto iter = tagged_lst.find(tag);
        if (iter != tagged_lst.end()) {
          auto msg = iter->second.front();
          action.run_action(msg);
          message_deref(msg);
          iter->second.pop_front();
          if (iter->second.size() == 0) {
            tagged_lst.erase(iter);
          }
          found_matching = true;
        }
      }

      if (not found_matching) {
        auto ready_trigger = [lst_ptr,trigger](MessageT* msg){
          trigger(msg);
          lst_ptr->make_ready();
        };

        if (tag == no_tag) {
          auto& lst = SeqStateType<MessageT,f>::seq_action;
          lst.emplace_back(
            SeqActionType<MessageT>{cur_seq_id,ready_trigger}
          );
        } else {
          auto& tagged_lst = SeqStateType<MessageT,f>::seq_action_tagged;
          tagged_lst[tag].emplace_back(
            SeqActionType<MessageT>{cur_seq_id,ready_trigger}
          );
        }
      }

      bool const should_block = not found_matching;

      return should_block;
    };

    assert(
      stateful_seq_ != no_seq and "Must be in an active sequence context"
    );

    lst.add_action([=]() -> bool {
      bool const in_seq = true;
      return execute_in_context(stateful_seq_, in_seq, deferred_wait_action);
    });
  }

  template <typename Callable>
  auto execute_in_context(
    SeqType const& context, bool const& in_sequence, Callable&& c
  ) {
    stateful_seq_ = context;
    auto const& ret = c();
    stateful_seq_ = no_seq;
    return ret;
  }

private:
  void attach_next(SeqType const& seq_id, UserSeqFunWithIDType const& fn) {
    return attach_next(seq_id, [seq_id,fn]{
      return fn(seq_id);
    });
  }

  void attach_next(SeqType const& seq_id, UserSeqFunType const& fn) {
    SeqListType& lst = get_seq_list(seq_id);
    auto fn_wrapper = [=]() -> bool {
      fn();
      return false;
    };
    lst.add_action([=]() -> bool {
      bool const in_seq = true;
      return execute_in_context(seq_id, in_seq, fn_wrapper);
    });
  }

public:
  template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
  void sequence_msg(MessageT* msg) {
    bool found_matching = false;

    auto const& is_tag_type = envelope_is_tag_type(msg->env);

    TagType const& msg_tag = is_tag_type ? envelope_get_tag(msg->env) : no_tag;

    // try to find a matching action that is posted for this tag
    if (msg_tag == no_tag) {
      auto& lst = SeqStateType<MessageT, f>::seq_action;

      if (lst.size() > 0) {
        auto action = lst.front();
        execute_in_context(action.seq_id, false, action.generate_callable(msg));
        lst.pop_front();
        found_matching = true;
      }
    } else {
      auto& tagged_lst = SeqStateType<MessageT, f>::seq_action_tagged;

      auto iter = tagged_lst.find(msg_tag);
      if (iter != tagged_lst.end()) {
        auto action = iter->second.front();
        execute_in_context(action.seq_id, false, action.generate_callable(msg));
        iter->second.pop_front();
        if (iter->second.size() == 0) {
          tagged_lst.erase(iter);
         }
        found_matching = true;
      }
    }

    // nothing was found so the message must be buffered and wait an action
    // being posted
    if (not found_matching) {
      // reference the arrival message to keep it alive past normal lifetime
      message_ref(msg);

      if (msg_tag == no_tag) {
        SeqStateType<MessageT, f>::seq_msg.push_back(msg);
      } else {
        SeqStateType<MessageT, f>::seq_msg_tagged[msg_tag].push_back(msg);
      }
    }

    debug_print(
      sequence, node,
      "sequence_msg: arriving: msg=%p, found_matching=%s, tag=%d\n",
      msg, print_bool(found_matching), msg_tag
    );
  }

private:
  SeqListType& get_seq_list(SeqType const& seq_id) {
    auto seq_iter = seq_lookup_.find(seq_id);
    assert(
      seq_iter != seq_lookup_.end() and "This seq_id must exit"
    );
    return seq_iter->second;
  }

private:
  SeqType stateful_seq_ = no_seq;

  SeqIDContainerType<SeqListType> seq_lookup_;
};

using Sequencer = TaggedSequencer<SeqType, SeqMigratableTriggerType>;

#define sequence_register_handler(message, handler)                     \
  static void handler(message* m) {                                     \
    the_seq->sequence_msg<message, handler>(m);                         \
  }

}} //end namespace runtime::seq

namespace runtime {

extern std::unique_ptr<seq::Sequencer> the_seq;

//using SequenceMessage = seq::SeqMsg;

} //end namespace runtime

#endif /*__RUNTIME_TRANSPORT_SEQUENCE__*/

