
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager.h"
#include "pipe/state/pipe_state.h"
#include "pipe/interface/send_container.h"
#include "pipe/id/pipe_id.h"
#include "context/context.h"

namespace vt { namespace pipe {

template <typename DataT>
void PipeManager::triggerSendBack(PipeType const& pipe, DataT data) {
  auto const& this_node = theContext()->getNode();
  auto const& node_back = PipeIDBuilder::getNode(pipe);
  if (node_back != this_node) {
    // Send the message back to the owner node
  } else {
    // Directly trigger the action because the pipe meta-data is located here
    assert(0);
  }
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
PipeManager::CallbackSendType<MsgT,f> PipeManager::makeCallbackSingleSend(
  bool const is_persist, NodeType const& send_to_node
) {
  using SendContainerType = typename interface::SendContainer<MsgT*>;
  auto const& new_pipe_id = makePipeID(is_persist,false);
  auto container = CallbackSendType<MsgT,f>(new_pipe_id,send_to_node);
  return container;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
void PipeManager::makeCallbackSingleBcast(bool const is_persist) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
}

}} /* end namespace vt::pipe */
