
#if !defined INCLUDED_PIPE_STATE_PIPE_STATE_H
#define INCLUDED_PIPE_STATE_PIPE_STATE_H

#include "config.h"
#include "pipe/pipe_common.h"

namespace vt { namespace pipe {

struct PipeState {
  PipeState(
    PipeType const& in_pipe, RefType const& in_signals, RefType const& in_lis
  );

  explicit PipeState(PipeType const& in_pipe);

  void signalRecv();
  void listenerReg();
  bool isAutomatic() const;
  PipeType getPipe() const;
  bool finished() const;

private:
  bool automatic_                 = false;
  RefType num_signals_expected_   = -1;
  RefType num_signals_received_   = 0;
  RefType num_listeners_expected_ = -1;
  RefType num_listeners_received_ = 0;
  PipeType pipe_                = no_pipe;
};

}} /* end namespace vt::pipe */

#endif /*INCLUDED_PIPE_STATE_PIPE_STATE_H*/
