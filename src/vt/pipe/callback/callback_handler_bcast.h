
#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_HANDLER_BCAST_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_HANDLER_BCAST_H

#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/signal/signal.h"
#include "vt/pipe/callback/callback_base.h"
#include "vt/activefn/activefn.h"
#include "vt/messaging/active.h"

namespace vt { namespace pipe { namespace callback {

template <typename MsgT, ActiveTypedFnType<MsgT>* f>
struct CallbackBcast : CallbackBase<signal::Signal<MsgT>> {
  using SignalBaseType = typename signal::Signal<MsgT>;
  using SignalType     = typename CallbackBase<SignalBaseType>::SignalType;
  using SignalDataType = typename SignalType::DataType;
  using MessageType    = MsgT;

  explicit CallbackBcast(bool const in_include_root = false)
    : include_root_(in_include_root)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    CallbackBase<SignalBaseType>::serializer(s);
    s | include_root_;
  }

private:
  void trigger_(SignalDataType* data) override {
    theMsg()->broadcastMsg<MsgT,f>(data);
    if (include_root_) {
      auto nmsg = makeSharedMessage<SignalDataType*>(*data);
      f(nmsg);
    }
  }

private:
  bool include_root_ = false;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_HANDLER_BCAST_H*/
