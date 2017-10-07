
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
  ctx->finish();
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

bool SeqULTContext::isContextActive() const {
  return has_valid_context_state_;
}

void SeqULTContext::runStateFunc(fcontext::ContextFuncTransfer* state) {
  debug_print_force(
    sequence, node,
    "SeqULTContext: runStateFunc: state=%p\n", state
  );

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

void SeqULTContext::start() {
  debug_print_force(
    sequence, node,
    "SeqULTContext: start\n"
  );

  has_valid_context_state_ = true;
  transfer_holder_ctx_ = fcontext::jumpContext(fctx, static_cast<void*>(this));
}

void SeqULTContext::suspend() {
  assert(cur_transfer_main_state_ != nullptr and "Must have valid state");
  has_valid_context_state_ = true;

  debug_print_force(
    sequence, node,
    "SeqULTContext: suspend: cur_transfer_main_state=%p\n",
    cur_transfer_main_state_
  );

  transfer_holder_main_ = fcontext::jumpContext(cur_transfer_main_state_->ctx);
  cur_transfer_main_state_ = &transfer_holder_main_.transfer;
}

void SeqULTContext::resume() {
  assert(has_valid_context_state_ and "Must have valid context state");
  transfer_holder_ctx_ = fcontext::jumpContext(transfer_holder_ctx_.transfer);
}

void SeqULTContext::finish() {
  assert(has_valid_context_state_ and "Must have valid context state");
  has_valid_context_state_ = false;
  cur_transfer_main_state_ = nullptr;
}


}} /* end namespace vt::seq */
