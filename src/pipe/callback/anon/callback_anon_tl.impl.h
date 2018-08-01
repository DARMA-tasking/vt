
#if !defined INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_IMPL_H
#define INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"
#include "pipe/callback/anon/callback_anon_tl.h"
#include "pipe/id/pipe_id.h"
#include "pipe/msg/callback.h"
#include "pipe/pipe_manager.h"
#include "activefn/activefn.h"
#include "context/context.h"
#include "messaging/active.h"
#include "runnable/general.h"

namespace vt { namespace pipe { namespace callback {

template <typename SerializerT>
void CallbackAnonTypeless::serialize(SerializerT& s) {}

template <typename MsgT>
void CallbackAnonTypeless::trigger(MsgT* msg, PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  auto const& pipe_node = PipeIDBuilder::getNode(pipe);
  debug_print(
    pipe, node,
    "CallbackAnonTypeless: trigger_: pipe={:x}, this_node={}\n",
    pipe, this_node
  );
  if (this_node == pipe_node) {
    assert(0);
    //theCB()->triggerPipe<MsgT>(pipe);
  } else {
    // auto msg = makeSharedMessage<CallbackMsg>(pipe,true);
    // theMsg()->sendMsg<CallbackMsg,PipeManager::triggerCallbackHan>(
    //   pipe_node, msg
    // );
  }
}

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_ANON_CALLBACK_ANON_TL_IMPL_H*/
