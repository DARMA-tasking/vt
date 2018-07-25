
#if !defined INCLUDED_PIPE_CALLBACK_CALLBACK_ANON_H
#define INCLUDED_PIPE_CALLBACK_CALLBACK_ANON_H

#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/signal/signal.h"
#include "pipe/callback/callback_base.h"

#include <functional>
#include <cassert>

namespace vt { namespace pipe { namespace callback {

template <typename SignalT>
struct CallbackAnonFun : CallbackBase<SignalT> {
  using SignalType = SignalT;
  using SignalDataType = typename SignalType::DataType;
  using CallbackFnType = std::function<void(SignalDataType)>;

  explicit CallbackAnonFun(CallbackFnType const& in_fn)
    : fn_(in_fn)
  { }

private:
  void trigger_(SignalDataType data) override {
    fn_(data);
  }

private:
  CallbackFnType fn_ = nullptr;
};

}}} /* end namespace vt::pipe::callback */

#endif /*INCLUDED_PIPE_CALLBACK_CALLBACK_ANON_H*/
