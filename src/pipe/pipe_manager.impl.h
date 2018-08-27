
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

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
Callback<MsgT> PipeManager::makeSend(NodeType const& node) {
  return makeCallbackSingleSend<MsgT,f>(node);
}

template <typename FunctorT, typename MsgT>
Callback<MsgT> PipeManager::makeSend(NodeType const& node) {
  return makeCallbackFunctorSend<FunctorT,MsgT>(node);
}

template <typename FunctorT, typename not_used_>
Callback<PipeManager::Void> PipeManager::makeSend(NodeType const& node) {
  return makeCallbackFunctorSendVoid<FunctorT>(node);
}

template <typename ColT, typename MsgT, PipeManager::ColHanType<ColT,MsgT>* f>
Callback<MsgT> PipeManager::makeSend(typename ColT::ProxyType proxy) {
  return makeCallbackSingleProxySend<ColT,MsgT,f>(proxy);
}

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
Callback<MsgT> PipeManager::makeBcast() {
  return makeCallbackSingleBcast<MsgT,f>(true);
}

template <typename FunctorT, typename MsgT>
Callback<MsgT> PipeManager::makeBcast() {
  return makeCallbackFunctorBcast<FunctorT,MsgT>(true);
}

template <typename FunctorT, typename not_used_>
Callback<PipeManager::Void> PipeManager::makeBcast() {
  return makeCallbackFunctorBcastVoid<FunctorT>(true);
}

template <typename ColT, typename MsgT, PipeManager::ColHanType<ColT,MsgT>* f>
Callback<MsgT> PipeManager::makeBcast(ColProxyType<ColT> proxy) {
  return makeCallbackSingleProxyBcast<ColT,MsgT,f>(proxy);
}

}} /* end namespace vt::pipe */
