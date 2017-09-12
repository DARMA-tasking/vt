
#if ! defined __RUNTIME_TRANSPORT_SEQ_STATE__
#define __RUNTIME_TRANSPORT_SEQ_STATE__

#include "seq_common.h"
#include "seq_action.h"

namespace runtime { namespace seq {

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
struct SeqMsgState {
  using ActionType = Action<MessageT>;

  template <typename T>
  using TagContainerType = std::unordered_map<TagType, T>;

  using ActionContainerType = std::list<ActionType>;
  using TaggedActionContainerType = TagContainerType<ActionContainerType>;

  using MsgContainerType = std::list<MessageT*>;
  using tagged_MsgContainerType = TagContainerType<std::list<MessageT*>>;

  // waiting actions on matching message arrival
  static ActionContainerType seq_action;
  static TaggedActionContainerType seq_action_tagged;

  // waiting messages on matching action arrival
  static MsgContainerType seq_msg;
  static tagged_MsgContainerType seq_msg_tagged;
};

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
using SeqStateType = SeqMsgState<MessageT, f>;

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
typename SeqStateType<MessageT, f>::ActionContainerType SeqMsgState<MessageT, f>::seq_action;

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
typename SeqStateType<MessageT, f>::TaggedActionContainerType SeqMsgState<MessageT, f>::seq_action_tagged;

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
typename SeqStateType<MessageT, f>::MsgContainerType SeqMsgState<MessageT, f>::seq_msg;

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
typename SeqStateType<MessageT, f>::tagged_MsgContainerType SeqMsgState<MessageT, f>::seq_msg_tagged;

}} //end namespace runtime::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_STATE__*/
