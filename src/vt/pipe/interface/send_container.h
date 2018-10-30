
#if !defined INCLUDED_PIPE_INTERFACE_SEND_CONTAINER_H
#define INCLUDED_PIPE_INTERFACE_SEND_CONTAINER_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/id/pipe_id.h"
#include "pipe/interface/base_container.h"

#include <tuple>
#include <utility>
#include <type_traits>

namespace vt { namespace pipe { namespace interface {

template <typename DataT, typename... Args>
struct SendContainer : BaseContainer<DataT> {

  explicit SendContainer(PipeType const& in_pipe, Args&&... args);

private:
  template <typename CallbackT>
  void triggerDirect(CallbackT cb, DataT data);

  void triggerDirect(DataT data);

  template <typename... Ts>
  void foreach(std::tuple<Ts...> const& t, DataT data);

  template <typename... Ts, std::size_t... Idx>
  void foreach(
    std::tuple<Ts...> const& tup, std::index_sequence<Idx...>, DataT data
  );

  bool isSendBack() const;

public:
  void trigger(DataT data);

  template <typename SerializerT>
  void serialize(SerializerT& s);

private:
  std::tuple<Args...> trigger_list_;
};

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_SEND_CONTAINER_H*/
