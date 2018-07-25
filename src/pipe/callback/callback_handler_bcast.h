
#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_HANDLER_BCAST_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_HANDLER_BCAST_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/signal/signal.h"
#include "pipe/callback/callback_base.h"
#include "activefn/activefn.h"
#include "messaging/active.h"

namespace vt { namespace pipe { namespace callback {

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct CallbackBcast : CallbackBase<signal::Signal<MsgT*>> {
  using SignalBaseType = typename signal::Signal<MsgT*>;
  using SignalType     = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType = typename SignalType::DataType;

  explicit CallbackBcast(bool const in_include_root = false)
    : include_root_(in_include_root)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | include_root_;
  }

private:
  void trigger_(SignalDataType data) override {
    theMsg()->broadcastMsg<MsgT,f>(data);
    if (include_root_) {
      auto nmsg = makeSharedMessage<SignalDataType>(*data);
      f(nmsg);
    }
  }

private:
  bool include_root_ = false;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_HANDLER_BCAST_H*/
