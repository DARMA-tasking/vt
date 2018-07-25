
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/state/pipe_state.h"

namespace vt { namespace pipe {

PipeState::PipeState(
  PipeType const& in_pipe, RefType const& in_signals, RefType const& in_lis
) : automatic_(true), num_signals_expected_(in_signals),
    num_listeners_expected_(in_lis), pipe_(in_pipe)
{}

PipeState::PipeState(PipeType const& in_pipe)
  : automatic_(false), pipe_(in_pipe)
{}

void PipeState::signalRecv() {
  num_signals_received_++;
}

void PipeState::listenerReg() {
  num_listeners_received_++;
}

bool PipeState::isAutomatic() const {
  return automatic_;
}

PipeType PipeState::getPipe() const {
  return pipe_;
}

bool PipeState::finished() const {
  if (automatic_) {
    auto const total_expect = num_signals_expected_ * num_listeners_expected_;
    auto const total_recv = num_signals_received_ * num_listeners_received_;
    return total_expect == total_recv;
  } else {
    return false;
  }
}

}} /* end namespace vt::pipe */
