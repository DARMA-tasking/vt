
#if !defined INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_IMPL_H
#define INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/interface/remote_container_msg.h"
#include "pipe/id/pipe_id.h"
#include "pipe/pipe_manager.h"

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
template <typename CallbackT>
void RemoteContainerMsg<MsgT,TupleT>::triggerDirect(CallbackT cb, MsgT* data) {
  ::fmt::print("RemoteContainerMsg: calling trigger on cb\n");
  cb.trigger(data);
}

template <typename MsgT, typename TupleT>
void RemoteContainerMsg<MsgT,TupleT>::trigger(MsgT* data) {
  bool const& is_send_back = isSendBack();
  ::fmt::print("RemoteContainerMsg: send_back={}\n", is_send_back);
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
