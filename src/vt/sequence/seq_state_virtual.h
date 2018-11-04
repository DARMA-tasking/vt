
#if !defined INCLUDED_SEQUENCE_SEQ_STATE_VIRTUAL_H
#define INCLUDED_SEQUENCE_SEQ_STATE_VIRTUAL_H

#include <list>
#include <unordered_map>

#include "vt/config.h"
#include "vt/sequence/seq_common.h"
#include "vt/sequence/seq_action_virtual.h"

namespace vt { namespace seq {

using namespace vrt;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
struct SeqMsgStateVirtual {
  using ActionType = ActionVirtual<MsgT, VcT>;

  template <typename T>
  using TagContainerType = std::unordered_map<TagType, T>;

  template <typename T>
  using ContainerType = std::list<T>;

  using ActionContainerType = ContainerType<ActionType>;
  using TaggedActionContainerType = TagContainerType<ActionContainerType>;

  using MsgContainerType = ContainerType<MsgT*>;
  using TaggedMsgContainerType = TagContainerType<std::list<MsgT*>>;

  // waiting actions on matching message arrival
  static ActionContainerType seq_action;
  static TaggedActionContainerType seq_action_tagged;

  // waiting messages on matching action arrival
  static MsgContainerType seq_msg;
  static TaggedMsgContainerType seq_msg_tagged;
};

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
using SeqStateVirtualType = SeqMsgStateVirtual<VcT, MsgT, f>;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
typename SeqStateVirtualType<VcT, MsgT, f>::ActionContainerType
  SeqMsgStateVirtual<VcT, MsgT, f>::seq_action;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
typename SeqStateVirtualType<VcT, MsgT, f>::TaggedActionContainerType
  SeqMsgStateVirtual<VcT, MsgT, f>::seq_action_tagged;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
typename SeqStateVirtualType<VcT, MsgT, f>::MsgContainerType
  SeqMsgStateVirtual<VcT, MsgT, f>::seq_msg;

template <typename VcT, typename MsgT, ActiveVrtTypedFnType<MsgT, VcT> *f>
typename SeqStateVirtualType<VcT, MsgT, f>::TaggedMsgContainerType
  SeqMsgStateVirtual<VcT, MsgT, f>::seq_msg_tagged;

}} //end namespace vt::seq

#endif /* INCLUDED_SEQUENCE_SEQ_STATE_VIRTUAL_H*/
