
#if !defined INCLUDED_SEQUENCE_SEQ_MATCHER_H
#define INCLUDED_SEQUENCE_SEQ_MATCHER_H

#include "vt/config.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/message.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_action.h"
#include "vt/sequence/seq_state.h"
#include "vt/sequence/seq_action.h"

#include <list>
#include <unordered_map>

namespace vt { namespace seq {

template <typename MessageT, ActiveTypedFnType<MessageT>* f>
struct SeqMatcher {
  using SeqActionType = Action<MessageT>;
  using MatchFuncType = ActiveTypedFnType<MessageT>;
  using SeqMsgStateType = SeqMsgState<MessageT, f>;

  template <typename T>
  using SeqStateContType = typename SeqMsgStateType::template ContainerType<T>;

  template <typename T>
  using SeqStateTaggedContType = typename SeqMsgStateType::template TagContainerType<T>;

  template <typename T>
  static bool hasFirstElem(T& lst);
  template <typename T>
  static auto getFirstElem(T& lst);

  template <typename T>
  static bool hasMatchingAnyNoTag(SeqStateContType<T>& lst);
  template <typename T>
  static auto getMatchingAnyNoTag(SeqStateContType<T>& lst);

  template <typename T>
  static bool hasMatchingAnyTagged(
    SeqStateTaggedContType<T>& tagged_lst, TagType const& tag
  );
  template <typename T>
  static auto getMatchingAnyTagged(
    SeqStateTaggedContType<T>& tagged_lst, TagType const& tag
  );

  static bool hasMatchingAction(TagType const& tag);
  static SeqActionType getMatchingAction(TagType const& tag);

  static bool hasMatchingMsg(TagType const& tag);
  static MessageT* getMatchingMsg(TagType const& tag);

  // Buffer messages and actions that do not match
  static void bufferUnmatchedMessage(MessageT* msg, TagType const& tag);
  template <typename FnT>
  static void bufferUnmatchedAction(
    FnT action, SeqType const& seq_id, TagType const& tag
  );
};

}} //end namespace vt::seq

#include "vt/sequence/seq_matcher.impl.h"

#endif /* INCLUDED_SEQUENCE_SEQ_MATCHER_H*/
