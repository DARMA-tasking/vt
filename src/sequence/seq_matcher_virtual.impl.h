
#if ! defined __RUNTIME_TRANSPORT_SEQ_MATCHER_VIRTUAL_IMPL__
#define __RUNTIME_TRANSPORT_SEQ_MATCHER_VIRTUAL_IMPL__

#include "config.h"
#include "activefn/activefn.h"
#include "message.h"
#include "seq_common.h"
#include "seq_action_virtual.h"
#include "seq_state_virtual.h"
#include "context/context_vrtheaders.h"

#include <list>
#include <unordered_map>

namespace vt { namespace seq {

using namespace vrt;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ bool SeqMatcherVirtual<VcT, MsgT, f>::hasFirstElem(T& lst) {
  return lst.size() > 0;
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ auto SeqMatcherVirtual<VcT, MsgT, f>::getFirstElem(T& lst) {
  if (lst.size() > 0) {
    auto elm = lst.front();
    lst.pop_front();
    return elm;
  } else {
    assert(0 and "Must have element");
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ bool SeqMatcherVirtual<VcT, MsgT, f>::hasMatchingAnyNoTag(
  SeqStateContType<T>& lst
) {
  return hasFirstElem(lst);
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ auto SeqMatcherVirtual<VcT, MsgT, f>::getMatchingAnyNoTag(
  SeqStateContType<T>& lst
) {
  return getFirstElem(lst);
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ bool SeqMatcherVirtual<VcT, MsgT, f>::hasMatchingAnyTagged(
  SeqStateTaggedContType<T>& tagged_lst, TagType const& tag
) {
  auto iter = tagged_lst.find(tag);
  return iter != tagged_lst.end() ? hasFirstElem(iter->second) : false;
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename T>
/*static*/ auto SeqMatcherVirtual<VcT, MsgT, f>::getMatchingAnyTagged(
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

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
/*static*/ bool SeqMatcherVirtual<VcT, MsgT, f>::hasMatchingMsg(TagType const& tag) {
  if (tag == no_tag) {
    auto& lst = SeqStateVirtualType<VcT, MsgT, f>::seq_msg;
    return hasMatchingAnyNoTag(lst);
  } else {
    auto& tagged_lst = SeqStateVirtualType<VcT, MsgT, f>::seq_msg_tagged;
    return hasMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
/*static*/ MsgT* SeqMatcherVirtual<VcT, MsgT, f>::getMatchingMsg(TagType const& tag) {
  if (tag == no_tag) {
    auto& lst = SeqStateVirtualType<VcT, MsgT, f>::seq_msg;
    return getMatchingAnyNoTag(lst);
  } else {
    auto& tagged_lst = SeqStateVirtualType<VcT, MsgT, f>::seq_msg_tagged;
    return getMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
/*static*/ bool SeqMatcherVirtual<VcT, MsgT, f>::hasMatchingAction(TagType const& tag) {
  if (tag == no_tag) {
    auto& lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action;
    return hasMatchingAnyNoTag(lst);
  } else {
    auto& tagged_lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action_tagged;
    return hasMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
/*static*/ typename SeqMatcherVirtual<VcT, MsgT, f>::SeqActionType
SeqMatcherVirtual<VcT, MsgT, f>::getMatchingAction(TagType const& tag) {
  assert(hasMatchingAction(tag) and "Must have matching action");

  if (tag == no_tag) {
    auto& lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action;
    return getMatchingAnyNoTag(lst);
  } else {
    auto& tagged_lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action_tagged;
    return getMatchingAnyTagged(tagged_lst, tag);
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
/*static*/ void SeqMatcherVirtual<VcT, MsgT, f>::bufferUnmatchedMessage(
  MsgT* msg, TagType const& tag
) {
  if (tag == no_tag) {
    SeqStateVirtualType<VcT, MsgT, f>::seq_msg.push_back(msg);
  } else {
    SeqStateVirtualType<VcT, MsgT, f>::seq_msg_tagged[tag].push_back(msg);
  }
}

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
template <typename FnT>
/*static*/ void SeqMatcherVirtual<VcT, MsgT, f>::bufferUnmatchedAction(
  FnT action, SeqType const& seq_id, TagType const& tag
) {
  debug_print(
    sequence, node,
    "SeqMatcher: buffering action: seq=%d, tag=%d\n", seq_id, tag
  );

  if (tag == no_tag) {
    auto& lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action;
    lst.emplace_back(SeqActionType{seq_id,action});
  } else {
    auto& tagged_lst = SeqStateVirtualType<VcT, MsgT, f>::seq_action_tagged;
    tagged_lst[tag].emplace_back(SeqActionType{seq_id,action});
  }
}

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_MATCHER_VIRTUAL_IMPL__*/
