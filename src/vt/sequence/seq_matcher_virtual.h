
#if !defined INCLUDED_SEQUENCE_SEQ_MATCHER_VIRTUAL_H
#define INCLUDED_SEQUENCE_SEQ_MATCHER_VIRTUAL_H

#include "config.h"
#include "activefn/activefn.h"
#include "messaging/message.h"
#include "seq_common.h"
#include "seq_action.h"
#include "seq_state_virtual.h"
#include "seq_action_virtual.h"
#include "vrt/context/context_vrtheaders.h"

#include <list>
#include <unordered_map>

namespace vt { namespace seq {

using namespace vrt;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
struct SeqMatcherVirtual {
  using SeqActionType = ActionVirtual<MsgT, VcT>;
  using MatchFuncType = ActiveVrtTypedFnType<MsgT, VcT>;
  using SeqMsgStateType = SeqMsgStateVirtual<VcT, MsgT, f>;

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
  static MsgT* getMatchingMsg(TagType const& tag);

  // Buffer messages and actions that do not match
  static void bufferUnmatchedMessage(MsgT* msg, TagType const& tag);
  template <typename FnT>
  static void bufferUnmatchedAction(
    FnT action, SeqType const& seq_id, TagType const& tag
  );
};

}} //end namespace vt::seq

#include "seq_matcher_virtual.impl.h"

#endif /* INCLUDED_SEQUENCE_SEQ_MATCHER_VIRTUAL_H*/
