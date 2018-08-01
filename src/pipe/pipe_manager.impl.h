
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager.h"
#include "pipe/state/pipe_state.h"
#include "pipe/interface/remote_container_msg.h"
#include "pipe/interface/send_container.h"
#include "pipe/interface/callback_direct.h"
#include "pipe/signal/signal_holder.h"
#include "pipe/id/pipe_id.h"
#include "pipe/callback/anon/callback_anon_listener.h"
#include "context/context.h"
#include "messaging/envelope.h"
#include "registry/auto/auto_registry.h"

#include <cassert>
#include <tuple>
#include <type_traits>
#include <array>

namespace vt { namespace pipe {

template <typename MsgT>
void PipeManager::triggerSendBack(PipeType const& pipe, MsgT* data) {
  auto const& this_node = theContext()->getNode();
  auto const& node_back = PipeIDBuilder::getNode(pipe);
  if (node_back != this_node) {
    // Send the message back to the owner node
    assert(0);
  } else {
    // Directly trigger the action because the pipe meta-data is located here
    assert(0);
  }
}

}} /* end namespace vt::pipe */
