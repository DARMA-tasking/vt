
#include "config.h"
#include "seq_common.h"
#include "seq_ult_context.h"

#include <cassert>
#include <functional>

namespace vt { namespace seq {

void seq_context_fn(fcontext::ContextFuncTransfer t) {
  void* data = t.data;
  SeqULTContext* ctx = reinterpret_cast<SeqULTContext*>(data);
  ctx->runStateFunc(&t);
  ctx->finishedExecution();
  fcontext::jumpContext(t);
}

SeqULTContext::SeqULTContext(SeqULTConstTag)
  : stack(fcontext::createStack())
{ }

void SeqULTContext::initialize(ULTContextFuncType func) {
  context_initialized = true;
  fctx = fcontext::makeContext(stack, func);
}

void SeqULTContext::initialize(ULTContextStatefulFnType stateful_func) {
  context_initialized = true;
  fctx = fcontext::makeContext(stack, seq_context_fn);
  state_fn_ = stateful_func;
}

bool SeqULTContext::initialized() const {
  return context_initialized;
}

void SeqULTContext::startExecution() {
  has_valid_context_state_ = true;
  transfer_holder_ctx_ = fcontext::jumpContext(fctx, static_cast<void*>(this));
}

bool SeqULTContext::isContextActive() const {
  return has_valid_context_state_;
}

void SeqULTContext::runStateFunc(fcontext::ContextFuncTransfer* state) {
  assert(state_fn_ != nullptr and "Must have valid state fn");
  setCurTransferState(state);
  state_fn_();
}

void SeqULTContext::setCurTransferState(fcontext::ContextFuncTransfer* state) {
  cur_transfer_main_state_ = state;
}

void SeqULTContext::clearCurTransferState() {
  cur_transfer_main_state_ = nullptr;
}

void SeqULTContext::suspend() {
  assert(cur_transfer_main_state_ != nullptr and "Must have valid state");
  has_valid_context_state_ = true;
  transfer_holder_main_ = fcontext::jumpContext(cur_transfer_main_state_->ctx);
  cur_transfer_main_state_ = &transfer_holder_main_.transfer;
}

void SeqULTContext::continueExecution() {
  assert(has_valid_context_state_ and "Must have valid context state");
  transfer_holder_ctx_ = fcontext::jumpContext(transfer_holder_ctx_.transfer);
}

void SeqULTContext::finishedExecution() {
  assert(has_valid_context_state_ and "Must have valid context state");
  has_valid_context_state_ = false;
  cur_transfer_main_state_ = nullptr;
}


}} /* end namespace vt::seq */
