
#if !defined INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_IMPL_H
#define INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_IMPL_H

#include "vt/config.h"
#include "vt/group/group_common.h"
#include "vt/group/global/group_default.h"
#include "vt/messaging/active.h"
#include "vt/activefn/activefn.h"
#include "vt/context/context.h"

namespace vt { namespace group { namespace global {

template <typename MsgT, ActiveTypedFnType<MsgT>* handler>
/*static*/ void DefaultGroup::sendPhaseMsg(
  PhaseType const& phase, NodeType const& node
) {
  auto const& this_node = theContext()->getNode();
  if (this_node == node) {
    auto msg = makeMessage<MsgT>();
    envelopeSetTag(msg.get()->env, phase);
    handler(msg.get());
  } else {
    auto msg = makeSharedMessage<MsgT>();
    envelopeSetTag(msg->env, phase);
    theMsg()->sendMsg<MsgT, handler>(node, msg);
  }
}

}}} /* end namespace vt::group::global */

#endif /*INCLUDED_GROUP_GLOBAL_GROUP_DEFAULT_IMPL_H*/
