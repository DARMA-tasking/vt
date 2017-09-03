
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

template <typename SeqTag, template<class> class SeqTrigger>
struct TaggedSequencer {
  using seq_t = SeqTag;
  using seq_list_t = SeqList;

  template <typename MessageT>
  using seq_action_t = Action<MessageT>;

  template <typename MessageT>
  using seq_trigger_type_t = SeqTrigger<MessageT>;

  template <typename T>
  using seq_id_container = std::unordered_map<seq_t, T>;

  using seq_fun_t = seq_list_t::seq_fun_t;

  TaggedSequencer() = default;

  seq_t
  next_seq() {
    auto const cur_id = next_seq_id;

    auto seq_iter = seq_lookup.find(cur_id);

    assert(
      seq_iter == seq_lookup.end() and "New seq_id should not exist now"
    );

    seq_lookup.emplace(
      std::piecewise_construct,
      std::forward_as_tuple(cur_id),
      std::forward_as_tuple(seq_list_t{cur_id})
    );

    next_seq_id++;

    return cur_id;
  }

  void
  sequenced(seq_t const& seq_id, user_seq_fun_with_id_t const& fn) {
    return attach_next(seq_id,fn);
  }

  void
  sequenced(seq_t const& seq_id, user_seq_fun_t const& fn) {
    return attach_next(seq_id,fn);
  }

  seq_t
  get_current_seq() const {
    return stateful_seq;
  }

  void
  scheduler() {
    for (auto&& live_seq : seq_lookup) {
      live_seq.second.progress();
    }
  }

  template <
    typename MessageT,
    action_any_function_t<MessageT>* f,
    action_any_function_t<MessageT>* trigger
  >
  void wait(tag_t const& tag = no_tag) {
    /*
     * Overload for a migratable variant---for now just forward to
     * non-migratable
     */

    // auto const& local_han = auto_registry::make_auto_handler<
    //   MessageT, trigger
    // >(nullptr);

    return wait<MessageT,f>(tag,trigger);
  }

  template <typename MessageT, action_any_function_t<MessageT>* f>
  void
  wait(seq_trigger_type_t<MessageT> trigger) {
    return wait<MessageT, f>(no_tag, trigger);
  }

  template <typename MessageT, action_any_function_t<MessageT>* f>
  void
  wait(tag_t const& tag, seq_trigger_type_t<MessageT> trigger) {
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
      stateful_seq != no_seq and "Must have valid seq now"
    );

    seq_list_t& lst = get_seq_list(stateful_seq);
    seq_list_t* const lst_ptr = &lst;

    auto const cur_seq_id = stateful_seq;

    auto action = seq_action_t<MessageT>{cur_seq_id,trigger};

    auto deferred_wait_action = [tag,trigger,lst_ptr,cur_seq_id,action]() -> bool {
      debug_print(
        sequence, node,
        "wait registered for tag=%d\n", tag
      );

      bool found_matching = false;

      if (tag == no_tag) {
        auto& lst = seq_state_t<MessageT,f>::seq_msg;
        if (lst.size() > 0) {
          auto msg = lst.front();
          lst.pop_front();
          action.run_action(msg);
          message_deref(msg);
          found_matching = true;
        }
      } else {
        auto& tagged_lst = seq_state_t<MessageT, f>::seq_msg_tagged;
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
          auto& lst = seq_state_t<MessageT,f>::seq_action;
          lst.emplace_back(
            seq_action_t<MessageT>{cur_seq_id,ready_trigger}
          );
        } else {
          auto& tagged_lst = seq_state_t<MessageT,f>::seq_action_tagged;
          tagged_lst[tag].emplace_back(
            seq_action_t<MessageT>{cur_seq_id,ready_trigger}
          );
        }
      }

      bool const should_block = not found_matching;

      return should_block;
    };

    assert(
      stateful_seq != no_seq and "Must be in an active sequence context"
    );

    lst.add_action([=]() -> bool {
      bool const in_seq = true;
      return execute_in_context(stateful_seq, in_seq, deferred_wait_action);
    });
  }

  template <typename Callable>
  auto
  execute_in_context(
    seq_t const& context, bool const& in_sequence, Callable&& c
  ) {
    stateful_seq = context;
    auto const& ret = c();
    stateful_seq = no_seq;
    return ret;
  }

private:
  void
  attach_next(seq_t const& seq_id, user_seq_fun_with_id_t const& fn) {
    return attach_next(seq_id, [seq_id,fn]{
      return fn(seq_id);
    });
  }

  void
  attach_next(seq_t const& seq_id, user_seq_fun_t const& fn) {
    seq_list_t& lst = get_seq_list(seq_id);
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
  template <typename MessageT, action_any_function_t<MessageT>* f>
  void sequence_msg(MessageT* msg) {
    bool found_matching = false;

    auto const& is_tag_type = envelope_is_tag_type(msg->env);

    tag_t const& msg_tag = is_tag_type ? envelope_get_tag(msg->env) : no_tag;

    // try to find a matching action that is posted for this tag
    if (msg_tag == no_tag) {
      auto& lst = seq_state_t<MessageT, f>::seq_action;

      if (lst.size() > 0) {
        auto action = lst.front();
        execute_in_context(action.seq_id, false, action.generate_callable(msg));
        lst.pop_front();
        found_matching = true;
      }
    } else {
      auto& tagged_lst = seq_state_t<MessageT, f>::seq_action_tagged;

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
        seq_state_t<MessageT, f>::seq_msg.push_back(msg);
      } else {
        seq_state_t<MessageT, f>::seq_msg_tagged[msg_tag].push_back(msg);
      }
    }

    debug_print(
      sequence, node,
      "sequence_msg: arriving: msg=%p, found_matching=%s, tag=%d\n",
      msg, print_bool(found_matching), msg_tag
    );
  }

private:
  seq_list_t&
  get_seq_list(seq_t const& seq_id) {
    auto seq_iter = seq_lookup.find(seq_id);
    assert(
      seq_iter != seq_lookup.end() and "This seq_id must exit"
    );
    return seq_iter->second;
  }

private:
  seq_t stateful_seq = no_seq;

  seq_id_container<seq_list_t> seq_lookup;
};

using Sequencer = TaggedSequencer<seq_t, seq_migratable_trigger_t>;

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

