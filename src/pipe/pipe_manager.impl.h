
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/pipe_manager.h"
#include "pipe/state/pipe_state.h"
#include "pipe/interface/remote_container_msg.h"
#include "pipe/interface/send_container.h"
#include "pipe/interface/callback_direct.h"
#include "pipe/pipe_manager_construct.h"
#include "pipe/id/pipe_id.h"
#include "context/context.h"
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

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
PipeManager::CallbackSendType<MsgT>
PipeManager::makeCallbackSingleSendTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
  auto const& handler = auto_registry::makeAutoHandler<MsgT,f>(nullptr);
  auto container = CallbackSendType<MsgT>(
    new_pipe_id,send_to_node,handler
  );
  return container;
}

template <typename MsgT, ActiveTypedFnType<MsgT>*... f>
auto
PipeManager::makeCallbackMultiSendTyped(
  bool const is_persist, NodeType const& send_to_node
) {
  using CBSendT = callback::CallbackSend<MsgT>;
  using ConsT = std::tuple<NodeType>;
  using TupleConsT = typename RepeatNImpl<sizeof...(f),ConsT>::ResultType;
  using ConstructMeta = ConstructCallbacks<CBSendT,TupleConsT,MsgT,f...>;
  using TupleCBType = typename ConstructMeta::ResultType;

  auto const& new_pipe_id = makePipeID(is_persist,false);
  std::array<NodeType,sizeof...(f)> send_node_array;
  send_node_array.fill(send_to_node);
  auto const cons = TupleConsT{send_node_array};
  auto const tuple = ConstructMeta::make(cons);
  auto rcm = interface::CallbackDirectSendMulti<MsgT,TupleCBType>(
    interface::CallbackDirectSendMultiTag,new_pipe_id,tuple
  );
  return rcm;
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
void PipeManager::makeCallbackSingleBcast(bool const is_persist) {
  auto const& new_pipe_id = makePipeID(is_persist,false);
}

}} /* end namespace vt::pipe */
