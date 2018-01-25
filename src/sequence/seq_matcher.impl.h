
#if !defined INCLUDED_SEQUENCE_SEQ_MATCHER_IMPL_H
#define INCLUDED_SEQUENCE_SEQ_MATCHER_IMPL_H

#include "config.h"
#include "activefn/activefn.h"
#include "messaging/message.h"
#include "seq_common.h"
#include "seq_action.h"
#include "seq_state.h"

#include <list>
#include <unordered_map>

namespace vt { namespace seq {

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
template <typename T>
/*static*/ bool SeqMatcher<MessageT, f>::hasFirstElem(T& lst) {
  return lst.size() > 0;
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
template <typename T>
/*static*/ auto SeqMatcher<MessageT, f>::getFirstElem(T& lst) {
  assert(lst.size > 0 and "Must have element");

  auto elm = lst.front();
  lst.pop_front();
  return elm;
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
template <typename T>
/*static*/ bool SeqMatcher<MessageT, f>::hasMatchingAnyNoTag(
  SeqStateContType<T>& lst
) {
  return hasFirstElem(lst);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
template <typename T>
/*static*/ auto SeqMatcher<MessageT, f>::getMatchingAnyNoTag(
  SeqStateContType<T>& lst
) {
  return getFirstElem(lst);
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
template <typename T>
/*static*/ bool SeqMatcher<MessageT, f>::hasMatchingAnyTagged(
  SeqStateTaggedContType<T>& tagged_lst, TagType const& tag
) {
  auto iter = tagged_lst.find(tag);
  return iter != tagged_lst.end() ? hasFirstElem(iter->second) : false;
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
template <typename T>
/*static*/ auto SeqMatcher<MessageT, f>::getMatchingAnyTagged(
  SeqStateTaggedContType<T>& tagged_lst, TagType const& tag
) {
  assert(hasMatchingAnyTagged(tagged_lst, tag) and "Must have matching elem");

  auto iter = tagged_lst.find(tag);
  auto elm = getFirstElem(iter->second);
  if (iter->second.size() == 0) {
    tagged_lst.erase(iter);
  }
  return elm;
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
/*static*/ bool SeqMatcher<MessageT, f>::hasMatchingMsg(TagType const& tag) {
  if (tag == no_tag) {
    auto& lst = SeqStateType<MessageT,f>::seq_msg;
    return hasMatchingAnyNoTag(lst);
  } else {
    auto& tagged_lst = SeqStateType<MessageT, f>::seq_msg_tagged;
    return hasMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
/*static*/ MessageT* SeqMatcher<MessageT, f>::getMatchingMsg(TagType const& tag) {
  if (tag == no_tag) {
    auto& lst = SeqStateType<MessageT, f>::seq_msg;
    return getMatchingAnyNoTag(lst);
  } else {
    auto& tagged_lst = SeqStateType<MessageT, f>::seq_msg_tagged;
    return getMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
/*static*/ bool SeqMatcher<MessageT, f>::hasMatchingAction(TagType const& tag) {
  if (tag == no_tag) {
    auto& lst = SeqStateType<MessageT, f>::seq_action;
    return hasMatchingAnyNoTag(lst);
  } else {
    auto& tagged_lst = SeqStateType<MessageT, f>::seq_action_tagged;
    return hasMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
/*static*/ typename SeqMatcher<MessageT, f>::SeqActionType
SeqMatcher<MessageT, f>::getMatchingAction(TagType const& tag) {
  assert(hasMatchingAction(tag) and "Must have matching action");

  if (tag == no_tag) {
    auto& lst = SeqStateType<MessageT, f>::seq_action;
    return getMatchingAnyNoTag(lst);
  } else {
    auto& tagged_lst = SeqStateType<MessageT, f>::seq_action_tagged;
    return getMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
/*static*/ void SeqMatcher<MessageT, f>::bufferUnmatchedMessage(
  MessageT* msg, TagType const& tag
) {
  if (tag == no_tag) {
    SeqStateType<MessageT, f>::seq_msg.push_back(msg);
  } else {
    SeqStateType<MessageT, f>::seq_msg_tagged[tag].push_back(msg);
  }
}

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
template <typename FnT>
/*static*/ void SeqMatcher<MessageT, f>::bufferUnmatchedAction(
  FnT action, SeqType const& seq_id, TagType const& tag
) {
  debug_print(
    sequence, node,
    "SeqMatcher: buffering action: seq=%d, tag=%d\n", seq_id, tag
  );

  if (tag == no_tag) {
    auto& lst = SeqStateType<MessageT,f>::seq_action;
    lst.emplace_back(SeqActionType{seq_id,action});
  } else {
    auto& tagged_lst = SeqStateType<MessageT,f>::seq_action_tagged;
    tagged_lst[tag].emplace_back(SeqActionType{seq_id,action});
  }
}

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_MATCHER_IMPL_H*/
