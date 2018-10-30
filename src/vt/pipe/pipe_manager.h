
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
#include "pipe/callback/cb_union/cb_raw_base.h"
#include "activefn/activefn.h"

#include <unordered_map>
#include <functional>

namespace vt { namespace pipe {


template <typename T>
struct FunctorTraits {
  template <typename U>
  using FunctorNoMsgArchType = decltype(std::declval<U>().operator()());
  using FunctorNoMsgType = detection::is_detected<FunctorNoMsgArchType,T>;
  static constexpr auto const has_no_msg_type = FunctorNoMsgType::value;
};

struct PipeManager : PipeManagerTL, PipeManagerTyped {

  template <typename FunctorT>
  using GetMsgType = typename util::FunctorExtractor<FunctorT>::MessageType;
  using Void = V;

  PipeManager();

  template <typename MsgT>
  Callback<MsgT> makeFunc(FuncMsgType<MsgT> fn);
  Callback<Void> makeFunc(FuncVoidType fn);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  Callback<MsgT> makeSend(NodeType const& node);
  template <typename FunctorT, typename MsgT = GetMsgType<FunctorT>>
  Callback<MsgT> makeSend(NodeType const& node);
  template <
    typename FunctorT,
    typename = std::enable_if_t<FunctorTraits<FunctorT>::has_no_msg_type>
  >
  Callback<Void> makeSend(NodeType const& node);
  template <typename ColT, typename MsgT, ColHanType<ColT,MsgT>* f>
  Callback<MsgT> makeSend(typename ColT::ProxyType proxy);

  template <typename MsgT, ActiveTypedFnType<MsgT>* f>
  Callback<MsgT> makeBcast();
  template <typename FunctorT, typename MsgT = GetMsgType<FunctorT>>
  Callback<MsgT> makeBcast();
  template <
    typename FunctorT,
    typename = std::enable_if_t<FunctorTraits<FunctorT>::has_no_msg_type>
  >
  Callback<Void> makeBcast();
  template <typename ColT, typename MsgT, ColHanType<ColT,MsgT>* f>
  Callback<MsgT> makeBcast(ColProxyType<ColT> proxy);


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
#include "pipe/callback/handler_send/callback_send.impl.h"
#include "pipe/callback/handler_bcast/callback_bcast.impl.h"
#include "pipe/callback/anon/callback_anon.impl.h"
#include "pipe/callback/anon/callback_anon_listener.impl.h"
#include "pipe/callback/anon/callback_anon_tl.impl.h"
#include "pipe/callback/handler_send/callback_send_tl.impl.h"
#include "pipe/callback/handler_bcast/callback_bcast_tl.impl.h"
#include "pipe/callback/proxy_bcast/callback_proxy_bcast_tl.impl.h"
#include "pipe/callback/proxy_send/callback_proxy_send_tl.impl.h"
#include "pipe/signal/signal_holder.impl.h"
#include "pipe/pipe_manager_base.impl.h"
#include "pipe/pipe_manager_tl.impl.h"
#include "pipe/pipe_manager_typed.impl.h"
#include "pipe/pipe_manager.impl.h"

#endif /*INCLUDED_PIPE_PIPE_MANAGER_H*/
