
#if !defined INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_IMPL_H
#define INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/interface/remote_container_msg.h"
#include "pipe/id/pipe_id.h"
#include "pipe/pipe_manager.h"
#include "context/context.h"
#include "messaging/message.h"
#include "pipe/signal/signal.h"

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

namespace vt { namespace pipe { namespace interface {

template <typename MsgT, typename TupleT>
template <typename... Args>
RemoteContainerMsg<MsgT,TupleT>::RemoteContainerMsg(
  PipeType const& in_pipe, Args... args
) : RemoteContainerMsg<MsgT,TupleT>(in_pipe,std::make_tuple(args...))
{}

template <typename MsgT, typename TupleT>
template <typename... Args>
RemoteContainerMsg<MsgT,TupleT>::RemoteContainerMsg(
  PipeType const& in_pipe, std::tuple<Args...> tup
) : BaseContainer<MsgT>(in_pipe), trigger_list_(tup)
{}

template <typename MsgT, typename TupleT>
template <typename MsgU, typename CallbackT>
RemoteContainerMsg<MsgT,TupleT>::IsVoidType<MsgU>
RemoteContainerMsg<MsgT,TupleT>::triggerDirect(CallbackT cb, MsgU*) {
  auto const& pid = BaseContainer<MsgT>::getPipe();
  constexpr auto multi_callback = std::tuple_size<decltype(trigger_list_)>();
  debug_print(
    pipe, node,
    "RemoteContainerMsg: (void) invoke trigger: pipe={:x}, multi={}\n",
    pid, multi_callback
  );
  cb.trigger(nullptr,pid);
}

template <typename MsgT, typename TupleT>
template <typename MsgU, typename CallbackT>
RemoteContainerMsg<MsgT,TupleT>::IsNotVoidType<MsgU>
RemoteContainerMsg<MsgT,TupleT>::triggerDirect(CallbackT cb, MsgU* data) {
  auto const& pid = BaseContainer<MsgT>::getPipe();
  auto const& multi_callback = std::tuple_size<decltype(trigger_list_)>() > 0;
  debug_print(
    pipe, node,
    "RemoteContainerMsg: (typed) invoke trigger: pipe={:x}, multi={}, ptr={}\n",
    pid, multi_callback, print_ptr(data)
  );
  MsgT* cur_msg = data;

  /*
   *  Make a copy of the message for each callback trigger because the callback
   *  trigger may send/bcast messages, thus a copy must be made
   */
  if (multi_callback) {
    cur_msg = makeSharedMessage<MsgT>(*data);
    messageRef(cur_msg);
  }

  cb.trigger(cur_msg,pid);

  if (multi_callback) {
    messageDeref(cur_msg);
  }
}

template <typename MsgT, typename TupleT>
template <typename MsgU>
void RemoteContainerMsg<MsgT,TupleT>::trigger(MsgU* data) {
  auto const& pipe = BaseContainer<MsgT>::getPipe();
  bool const& is_send_back = isSendBack();
  debug_print(
    pipe, node,
    "RemoteContainerMsg: pipe={:x}, send_back={}, size={}\n",
    pipe, is_send_back, std::tuple_size<decltype(trigger_list_)>()
  );
  if (is_send_back) {
    auto const& pipe = this->getPipe();
    theCB()->triggerSendBack<MsgT>(pipe,data);
  } else {
    triggerDirect(data);
  }
}

template <typename MsgT, typename TupleT>
bool RemoteContainerMsg<MsgT,TupleT>::isSendBack() const {
  auto const& pipe = this->getPipe();
  return PipeIDBuilder::isSendback(pipe);
}

template <typename MsgT, typename TupleT>
template <typename... Ts>
void RemoteContainerMsg<MsgT,TupleT>::foreach(
  std::tuple<Ts...> const& t, MsgT* data
) {
  return foreach(t, std::index_sequence_for<Ts...>{}, data);
}

template <typename MsgT, typename TupleT>
template <typename... Ts, std::size_t... Idx>
void RemoteContainerMsg<MsgT,TupleT>::foreach(
  std::tuple<Ts...> const& tup, std::index_sequence<Idx...>, MsgT* data
) {
  auto _={(triggerDirect(std::get<Idx>(tup),data),0)...};
}

template <typename MsgT, typename TupleT>
void RemoteContainerMsg<MsgT,TupleT>::triggerDirect(MsgT* data) {
  return foreach(trigger_list_, data);
}

template <typename MsgT, typename TupleT>
template <typename SerializerT>
void RemoteContainerMsg<MsgT,TupleT>::serialize(SerializerT& s) {
  BaseContainer<MsgT>::serializer(s);
  s | trigger_list_;
}

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_IMPL_H*/
