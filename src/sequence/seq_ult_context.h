
#if !defined __RUNTIME_TRANSPORT_SEQ_ULT_CONTEXT__
#define __RUNTIME_TRANSPORT_SEQ_ULT_CONTEXT__

#include "config.h"
#include "seq_common.h"

#include "context/src/context.h"

#include <functional>

namespace vt { namespace seq {

static struct SeqULTConstTag { } seq_ult_cons_tag_t { };

void seq_context_fn(fcontext::ContextFuncTransfer t);

struct SeqULTContext {
  using ULTContextFuncType = fcontext::ContextFunctionParam;
  using ULTContextStatefulFnType = std::function<void()>;

  fcontext::ULTContextType stack;
  fcontext::Context fctx;

  explicit SeqULTContext(SeqULTConstTag);

  SeqULTContext() : SeqULTContext(seq_ult_cons_tag_t) { }

  void initialize(ULTContextFuncType func);
  void initialize(ULTContextStatefulFnType stateful_func);
  void startExecution();
  bool initialized() const;
  bool isContextActive() const;
  void runStateFunc(fcontext::ContextFuncTransfer* state);
  void setCurTransferState(fcontext::ContextFuncTransfer* state);
  void clearCurTransferState();
  void suspend();
  void continueExecution();
  void finishedExecution();

private:
  bool has_valid_context_state_ = false;

  fcontext::ContextTransfer transfer_holder_main_;
  fcontext::ContextTransfer transfer_holder_ctx_;

  fcontext::ContextFuncTransfer* cur_transfer_main_state_ = nullptr;

  ULTContextStatefulFnType state_fn_ = nullptr;

  bool context_initialized = false;
};

}} /* end namespace vt::seq */

#endif /*__RUNTIME_TRANSPORT_SEQ_ULT_CONTEXT__*/
