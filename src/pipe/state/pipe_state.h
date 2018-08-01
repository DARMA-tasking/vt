
#if !defined INCLUDED_PIPE_STATE_PIPE_STATE_H
#define INCLUDED_PIPE_STATE_PIPE_STATE_H

#include "config.h"
#include "pipe/pipe_common.h"

#include <functional>

namespace vt { namespace pipe {

struct PipeState {
  using DispatchFuncType = std::function<void(void*)>;

  PipeState(
    PipeType const& in_pipe, RefType const& in_signals, RefType const& in_lis,
    bool const& in_typeless = false
  );

  PipeState(PipeType const& in_pipe, bool const& in_typeless = false);

  void signalRecv();
  void listenerReg();
  bool isAutomatic() const;
  bool isTypeless() const;
  bool isPersist() const;
  PipeType getPipe() const;
  bool finished() const;
  RefType refsPerListener() const;

  void setDispatch(DispatchFuncType in_dispatch);
  void dispatch(void* ptr);

private:
  bool automatic_                 = false;
  bool typeless_                  = false;
  RefType num_signals_expected_   = -1;
  RefType num_signals_received_   = 0;
  RefType num_listeners_expected_ = -1;
  RefType num_listeners_received_ = 0;
  PipeType pipe_                  = no_pipe;
  DispatchFuncType dispatch_      = nullptr;
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_STATE_PIPE_STATE_H*/
