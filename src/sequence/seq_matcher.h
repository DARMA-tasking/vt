
#if ! defined __RUNTIME_TRANSPORT_SEQ_MATCHER__
#define __RUNTIME_TRANSPORT_SEQ_MATCHER__

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
struct SeqMatcher {
  using SeqActionType = Action<MessageT>;
  using MatchFuncType = ActiveAnyFunctionType<MessageT>;
  using SeqMsgStateType = SeqMsgState<MessageT, f>;

  template <typename T>
  using SeqStateContType = typename SeqMsgStateType::template ContainerType<T>;

  template <typename T>
  using SeqStateTaggedContType = typename SeqMsgStateType::template TagContainerType<T>;

  template <typename T, typename FnT>
  static bool findMatchingNoTag(SeqStateContType<T>& lst, FnT func);

  template <typename T, typename FnT>
  static bool findMatchingTagged(
    SeqStateTaggedContType<T>& tagged_lst, FnT func, TagType const& tag
  );

  template <typename FnT>
  static bool findMatchingMsg(FnT func, TagType const& tag);

  template <typename FnT>
  static bool findMatchingAction(FnT func, TagType const& tag);

  static void bufferUnmatchedMessage(MessageT* msg, TagType const& tag);

  template <typename FnT>
  static void bufferUnmatchedAction(
    FnT action, SeqType const& seq_id, TagType const& tag
  );
};

}} //end namespace vt::seq

#include "seq_matcher.impl.h"

#endif /* __RUNTIME_TRANSPORT_SEQ_MATCHER__*/
