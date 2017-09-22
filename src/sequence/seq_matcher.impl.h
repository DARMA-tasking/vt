
#if ! defined __RUNTIME_TRANSPORT_SEQ_MATCHER_IMPL__
#define __RUNTIME_TRANSPORT_SEQ_MATCHER_IMPL__

#include "config.h"
#include "function.h"
#include "message.h"
#include "seq_common.h"
#include "seq_action.h"
#include "seq_state.h"
#include "seq_action.h"

#include <list>
#include <unordered_map>

namespace vt { namespace seq {

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
template <typename T, typename FnT>
/*static*/ bool SeqMatcher<MessageT, f>::applyActionFirstElem(T& lst, FnT func) {
  bool applied_action = false;
  if (lst.size() > 0) {
    auto elm = lst.front();
    lst.pop_front();
    func(elm);
    applied_action = true;
  }
  return applied_action;
}

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
template <typename T, typename FnT>
/*static*/ bool SeqMatcher<MessageT, f>::findMatchingNoTag(
  SeqStateContType<T>& lst, FnT func
) {
  return applyActionFirstElem(lst, func);
}

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
template <typename T, typename FnT>
/*static*/ bool SeqMatcher<MessageT, f>::findMatchingTagged(
  SeqStateTaggedContType<T>& tagged_lst, FnT func, TagType const& tag
) {
  bool applied_action = false;
  auto iter = tagged_lst.find(tag);
  if (iter != tagged_lst.end()) {
    applied_action = applyActionFirstElem(iter->second, func);
    if (iter->second.size() == 0) {
      tagged_lst.erase(iter);
    }
  }
  return applied_action;
}

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
template <typename FnT>
/*static*/ bool SeqMatcher<MessageT, f>::findMatchingMsg(FnT func, TagType const& tag) {
  if (tag == no_tag) {
    auto& lst = SeqStateType<MessageT,f>::seq_msg;
    return findMatchingNoTag(lst, func);
  } else {
    auto& tagged_lst = SeqStateType<MessageT, f>::seq_msg_tagged;
    return findMatchingTagged(tagged_lst, func, tag);
  }
}

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
template <typename FnT>
/*static*/ bool SeqMatcher<MessageT, f>::findMatchingAction(FnT func, TagType const& tag) {
  if (tag == no_tag) {
    auto& lst = SeqStateType<MessageT, f>::seq_action;
    return findMatchingNoTag(lst, func);
  } else {
    auto& tagged_lst = SeqStateType<MessageT, f>::seq_action_tagged;
    return findMatchingTagged(tagged_lst, func, tag);
  }
}

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
/*static*/ void SeqMatcher<MessageT, f>::bufferUnmatchedMessage(
  MessageT* msg, TagType const& tag
) {
  if (tag == no_tag) {
    SeqStateType<MessageT, f>::seq_msg.push_back(msg);
  } else {
    SeqStateType<MessageT, f>::seq_msg_tagged[tag].push_back(msg);
  }
}

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
template <typename FnT>
/*static*/ void SeqMatcher<MessageT, f>::bufferUnmatchedAction(
  FnT action, SeqType const& seq_id, TagType const& tag
) {
  if (tag == no_tag) {
    auto& lst = SeqStateType<MessageT,f>::seq_action;
    lst.emplace_back(SeqActionType{seq_id,action});
  } else {
    auto& tagged_lst = SeqStateType<MessageT,f>::seq_action_tagged;
    tagged_lst[tag].emplace_back(SeqActionType{seq_id,action});
  }
}

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_MATCHER_IMPL__*/
