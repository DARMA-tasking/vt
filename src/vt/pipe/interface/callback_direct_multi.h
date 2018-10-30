
#if !defined INCLUDED_PIPE_INTERFACE_CALLBACK_DIRECT_MULTI_H
#define INCLUDED_PIPE_INTERFACE_CALLBACK_DIRECT_MULTI_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/id/pipe_id.h"
#include "pipe/interface/base_container.h"
#include "pipe/interface/remote_container_msg.h"

#include <tuple>
#include <utility>
#include <type_traits>

namespace vt { namespace pipe { namespace interface {

static struct CallbackDirectSendMultiTagType {} CallbackDirectSendMultiTag { };

template <typename MsgT, typename TupleT>
struct CallbackDirectSendMulti : RemoteContainerMsg<MsgT,TupleT> {

  template <typename... Args>
  explicit CallbackDirectSendMulti(PipeType const& in_pipe, Args... args)
    : RemoteContainerMsg<MsgT,TupleT>(in_pipe,std::make_tuple(args...))
  { }

  template <typename... Args>
  CallbackDirectSendMulti(
    CallbackDirectSendMultiTagType, PipeType const& in_pipe,
    std::tuple<Args...> tup
  ) : RemoteContainerMsg<MsgT,TupleT>(in_pipe,tup)
  { }

  void send(MsgT* m) {
    ::fmt::print("callback send\n");
    RemoteContainerMsg<MsgT,TupleT>::trigger(m);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    RemoteContainerMsg<MsgT,TupleT>::serialize(s);
  }
};

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_CALLBACK_DIRECT_MULTI_H*/
