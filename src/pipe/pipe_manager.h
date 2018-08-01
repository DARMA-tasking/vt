
#if !defined INCLUDED_PIPE_PIPE_MANAGER_H
#define INCLUDED_PIPE_PIPE_MANAGER_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/state/pipe_state.h"
#include "pipe/pipe_manager.fwd.h"
#include "pipe/pipe_manager_tl.h"
#include "pipe/pipe_manager_typed.h"
#include "pipe/msg/callback.h"
#include "pipe/signal/signal_holder.h"
#include "pipe/callback/anon/callback_anon.fwd.h"
#include "activefn/activefn.h"

#include <unordered_map>
#include <functional>

namespace vt { namespace pipe {

struct PipeManager : PipeManagerTL, PipeManagerTyped {

  PipeManager();

public:
  /*
   *  Trigger and send back on a pipe that is not locally triggerable and thus
   *  requires communication if it is "sent" off-node.
   */
  template <typename MsgT>
  void triggerSendBack(PipeType const& pipe, MsgT* data);

private:
  // The group ID used to indicate that the message is being used as a pipe
  GroupType group_id_ = no_group;
};

}} /* end namespace vt::pipe */

#include "pipe/interface/send_container.impl.h"
#include "pipe/interface/remote_container_msg.impl.h"
#include "pipe/callback/anon/callback_anon.impl.h"
#include "pipe/callback/anon/callback_anon_listener.impl.h"
#include "pipe/signal/signal_holder.impl.h"
#include "pipe/pipe_manager_base.impl.h"
#include "pipe/pipe_manager_tl.impl.h"
#include "pipe/pipe_manager_typed.impl.h"
#include "pipe/pipe_manager.impl.h"

#endif /*INCLUDED_PIPE_PIPE_MANAGER_H*/
