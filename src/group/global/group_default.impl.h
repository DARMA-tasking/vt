
#if !defined INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_IMPL_H
#define INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_IMPL_H

#include "config.h"
#include "group/group_common.h"
#include "group/global/group_default.h"
#include "messaging/active.h"
#include "activefn/activefn.h"
#include "context/context.h"

namespace vt { namespace group { namespace global {

template <typename MsgT, ActiveTypedFnType<MsgT>* handler>
/*static*/ void DefaultGroup::sendPhaseMsg(
  PhaseType const& phase, NodeType const& node
) {
  auto const& this_node = theContext()->getNode();
  if (this_node == node) {
    auto msg = makeSharedMessage<MsgT>();
    envelopeSetTag(msg->env, phase);
    messageRef(msg);
    handler(msg);
    messageDeref(msg);
  } else {
    auto msg = makeSharedMessage<MsgT>();
    envelopeSetTag(msg->env, phase);
    theMsg()->sendMsg<MsgT, handler>(node, msg);
  }
}

}}} /* end namespace vt::group::global */

#endif /*INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_IMPL_H*/
