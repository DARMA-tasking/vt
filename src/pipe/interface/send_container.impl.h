
#if !defined INCLUDED_PIPE_INTERFACE_SEND_CONTAINER_IMPL_H
#define INCLUDED_PIPE_INTERFACE_SEND_CONTAINER_IMPL_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/interface/send_container.h"
#include "pipe/id/pipe_id.h"
#include "pipe/pipe_manager.h"

#include <tuple>
#include <utility>
#include <functional>
#include <type_traits>

namespace vt { namespace pipe { namespace interface {

template <typename DataT, typename... Args>
SendContainer<DataT,Args...>::SendContainer(
  PipeType const& in_pipe, Args&&... args
) : BaseContainer<DataT>(in_pipe),
    trigger_list_(std::make_tuple(std::forward<Args...>(args...)))
{}

template <typename DataT, typename... Args>
template <typename CallbackT>
void SendContainer<DataT,Args...>::triggerDirect(CallbackT cb, DataT data) {
  cb.trigger(data);
}

template <typename DataT, typename... Args>
void SendContainer<DataT,Args...>::trigger(DataT data) {
  bool const& is_send_back = isSendBack();
  if (is_send_back) {
    auto const& pipe = this->getPipe();
    theCB()->triggerSendBack<DataT>(pipe,data);
  } else {
    triggerDirect(data);
  }
}

template <typename DataT, typename... Args>
bool SendContainer<DataT,Args...>::isSendBack() const {
  auto const& pipe = this->getPipe();
  return PipeIDBuilder::isSendback(pipe);
}

template <typename DataT, typename... Args>
template <typename... Ts>
void SendContainer<DataT,Args...>::foreach(
  std::tuple<Ts...> const& t, DataT data
) {
  return foreach(t, std::index_sequence_for<Ts...>{}, data);
}

template <typename DataT, typename... Args>
template <typename... Ts, std::size_t... Idx>
void SendContainer<DataT,Args...>::foreach(
  std::tuple<Ts...> const& tup, std::index_sequence<Idx...>, DataT data
) {
  auto _={(triggerDirect(std::get<Idx>(tup),data),0)...};
}

template <typename DataT, typename... Args>
void SendContainer<DataT,Args...>::triggerDirect(DataT data) {
  return foreach(trigger_list_, data);
}

template <typename DataT, typename... Args>
template <typename SerializerT>
void SendContainer<DataT,Args...>::serialize(SerializerT& s) {
  BaseContainer<DataT>::serializer(s);
  s | trigger_list_;
}

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_SEND_CONTAINER_IMPL_H*/
