
#include "config.h"
#include "pipe/pipe_common.h"
#include "pipe/state/pipe_state.h"

namespace vt { namespace pipe {

PipeState::PipeState(
  PipeType const& in_pipe, RefType const& in_signals, RefType const& in_lis,
  bool const& in_typeless
) : automatic_(true), typeless_(in_typeless), num_signals_expected_(in_signals),
    num_listeners_expected_(in_lis), pipe_(in_pipe)
{}

PipeState::PipeState(PipeType const& in_pipe, bool const& in_typeless)
  : automatic_(false), typeless_(in_typeless), pipe_(in_pipe)
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

bool PipeState::isTypeless() const {
  return typeless_;
}

bool PipeState::isPersist() const {
  return !automatic_;
}

PipeType PipeState::getPipe() const {
  return pipe_;
}

RefType PipeState::refsPerListener() const {
  return num_signals_expected_;
}

bool PipeState::hasDispatch() const {
  return dispatch_ != nullptr;
}

void PipeState::setDispatch(DispatchFuncType in_dispatch) {
  dispatch_ = in_dispatch;
}

void PipeState::dispatch(void* ptr) {
  vtAssert(dispatch_ != nullptr, "Dispatch function must be available");
  dispatch_(ptr);
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
