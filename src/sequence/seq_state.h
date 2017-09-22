
#if ! defined __RUNTIME_TRANSPORT_SEQ_STATE__
#define __RUNTIME_TRANSPORT_SEQ_STATE__

#include <list>
#include <unordered_map>

#include "config.h"
#include "seq_common.h"
#include "seq_action.h"

namespace vt { namespace seq {

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
struct SeqMsgState {
  using ActionType = Action<MessageT>;

  template <typename T>
  using TagContainerType = std::unordered_map<TagType, T>;

  template <typename T>
  using ContainerType = std::list<T>;

  using ActionContainerType = ContainerType<ActionType>;
  using TaggedActionContainerType = TagContainerType<ActionContainerType>;

  using MsgContainerType = ContainerType<MessageT*>;
  using TaggedMsgContainerType = TagContainerType<std::list<MessageT*>>;

  // waiting actions on matching message arrival
  static ActionContainerType seq_action;
  static TaggedActionContainerType seq_action_tagged;

  // waiting messages on matching action arrival
  static MsgContainerType seq_msg;
  static TaggedMsgContainerType seq_msg_tagged;
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
typename SeqStateType<MessageT, f>::TaggedMsgContainerType SeqMsgState<MessageT, f>::seq_msg_tagged;

}} //end namespace vt::seq

#endif /* __RUNTIME_TRANSPORT_SEQ_STATE__*/
