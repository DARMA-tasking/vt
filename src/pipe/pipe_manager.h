
#if !defined INCLUDED_PIPE_PIPE_MANAGER_H
#define INCLUDED_PIPE_PIPE_MANAGER_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/state/pipe_state.h"
#include "pipe/pipe_manager.fwd.h"
#include "pipe/interface/send_container.h"
#include "pipe/interface/callback_direct.h"
#include "pipe/callback/handler_send/callback_handler_send_remote.h"
#include "pipe/pipe_manager_construct.h"
#include "activefn/activefn.h"

#include <unordered_map>
#include <tuple>
#include <type_traits>

namespace vt { namespace pipe {

struct PipeManager {
  using PipeStateType = PipeState;

  template <typename MsgT>
  using CallbackSendType = interface::CallbackDirectSend<MsgT>;

  PipeManager() = default;

  /*
   *  Builders for non-send-back type of pipe callback: they are invoked
   *  directly from the sender; thus this node is not involved in the process
   */
  template <typename MsgT, ActiveTypedFnType<MsgT>*... f>
  interface::CallbackDirectSendMulti<
    MsgT,
    typename RepeatNImpl<sizeof...(f),callback::CallbackSend<MsgT>>::ResultType
  >
  makeCallbackMultiSendTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  auto pushTarget(NodeType const& send_to_node);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f, typename CallbackT>
  auto pushTarget(CallbackT in, NodeType const& send_to_node);

  template <typename CallbackT>
  auto buildMultiCB(CallbackT in);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  CallbackSendType<MsgT> makeCallbackSingleSendTyped(
    bool const is_persist, NodeType const& send_to_node
  );

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  void makeCallbackSingleBcast(bool const is_persist);

  /*
   *  Trigger and send back on a pipe that is not locally triggerable and thus
   *  requires communication if it is "sent" off-node.
   */
  template <typename MsgT>
  void triggerSendBack(PipeType const& pipe, MsgT* data);

private:
  PipeType makePipeID(bool const persist, bool const send_back);

private:
  // the current pipe id local to this node
  PipeIDType cur_pipe_id_ = initial_pipe_id;
  // the pipe state for pipes that have a send back
  std::unordered_map<PipeType,PipeStateType> pipe_state_;
};

}} /* end namespace vt::pipe */

#include "pipe/pipe_manager.impl.h"
#include "pipe/interface/send_container.impl.h"
#include "pipe/interface/remote_container_msg.impl.h"

#endif /*INCLUDED_PIPE_PIPE_MANAGER_H*/
