
#if !defined INCLUDED_PIPE_INTERFACE_CALLBACK_DIRECT_H
#define INCLUDED_PIPE_INTERFACE_CALLBACK_DIRECT_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/interface/base_container.h"
#include "vt/pipe/interface/remote_container_msg.h"
#include "vt/context/context.h"

#include <tuple>
#include <utility>
#include <type_traits>

namespace vt { namespace pipe { namespace interface {

template <typename MsgT, typename CallbackT>
struct CallbackDirect : RemoteContainerMsg<MsgT,std::tuple<CallbackT>> {
  using BaseType      = RemoteContainerMsg<MsgT,std::tuple<CallbackT>>;
  using VoidSigType   = signal::SigVoidType;
  template <typename T, typename U=void>
  using IsVoidType    = std::enable_if_t<std::is_same<T,VoidSigType>::value,U>;
  template <typename T, typename U=void>
  using IsNotVoidType = std::enable_if_t<!std::is_same<T,VoidSigType>::value,U>;

  template <typename... Args>
  CallbackDirect(PipeType const& in_pipe, Args... args)
    : BaseType(in_pipe, CallbackT(args...))
  { }

  template <typename MsgU>
  IsNotVoidType<MsgU> send(MsgU* m) {
    static_assert(std::is_same<MsgT,MsgU>::value, "Required exact type match");
    BaseType::template trigger<MsgU>(m);
  }

  template <typename T=void, typename=IsVoidType<MsgT,T>>
  void send() {
    BaseType::template trigger<MsgT>(nullptr);
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    BaseType::serialize(s);
  }

};

}}} /* end namespace vt::pipe::interface */

#endif /*INCLUDED_PIPE_INTERFACE_CALLBACK_DIRECT_H*/
