
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/callback/callback_base_tl.h"
#include "pipe/callback/anon/callback_anon_tl.h"
#include "pipe/id/pipe_id.h"
#include "pipe/msg/callback.h"
#include "pipe/pipe_manager.h"
#include "context/context.h"
#include "messaging/active.h"

namespace vt { namespace pipe { namespace callback {

void CallbackAnonTypeless::triggerVoid(PipeType const& pipe) {
  auto const& this_node = theContext()->getNode();
  auto const& pipe_node = PipeIDBuilder::getNode(pipe);
  debug_print(
    pipe, node,
    "CallbackAnonTypeless: trigger_: pipe={:x}, this_node={}\n",
    pipe, this_node
  );
  if (this_node == pipe_node) {
    theCB()->triggerPipe(pipe);
  } else {
    auto msg = makeSharedMessage<CallbackMsg>(pipe,true);
    theMsg()->sendMsg<CallbackMsg,PipeManager::triggerCallbackHan>(
      pipe_node, msg
    );
  }
}

}}} /* end namespace vt::pipe::callback */
