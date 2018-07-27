
#if !defined INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_H
#define INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/id/pipe_id.h"
#include "pipe/interface/base_container.h"

#include <tuple>
#include <utility>
#include <type_traits>

namespace vt { namespace pipe { namespace interface {

template <typename MsgT, typename TupleT>
struct RemoteContainerMsg : BaseContainer<MsgT> {

  template <typename... Args>
  explicit RemoteContainerMsg(PipeType const& in_pipe, Args... args);

  template <typename... Args>
  RemoteContainerMsg(PipeType const& in_pipe, std::tuple<Args...> tup);

private:
  template <typename CallbackT>
  void triggerDirect(CallbackT cb, MsgT* data);

  void triggerDirect(MsgT* data);

  template <typename... Ts>
  void foreach(std::tuple<Ts...> const& t, MsgT* data);

  template <typename... Ts, std::size_t... Idx>
  void foreach(
    std::tuple<Ts...> const& tup, std::index_sequence<Idx...>, MsgT* data
  );

  bool isSendBack() const;

public:
  void trigger(MsgT* data);

  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  TupleT trigger_list_;
};

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_REMOTE_CONTAINER_MSG_H*/
