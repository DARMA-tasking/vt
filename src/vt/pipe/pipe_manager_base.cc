
#include "vt/config.h"
#include "vt/pipe/pipe_common.h"
#include "vt/pipe/pipe_manager_base.h"
#include "vt/pipe/pipe_manager.h"
#include "vt/pipe/state/pipe_state.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/context/context.h"
#include "vt/pipe/signal/signal_holder.h"
#include "vt/pipe/callback/anon/callback_anon_listener.h"
#include "vt/pipe/id/pipe_id.h"
#include "vt/pipe/msg/callback.h"
#include "vt/context/context.h"

#include <cassert>
#include <cstdlib>
#include <unordered_map>
#include <memory>
#include <functional>

namespace vt { namespace pipe {

void PipeManagerBase::newPipeState(
  PipeType const& pipe, bool persist, bool typeless, RefType num_signals,
  RefType num_listeners, RefType num_reg_listeners, DispatchFuncType fn
) {
  /*
   *  Create pipe state of `pipe' to track the state locally on this node
   */
  auto iter = pipe_state_.find(pipe);
  assert(iter == pipe_state_.end() && "State must not exist for new pipe");
  assert((persist || num_signals > 0) && "Valid signal count non-persist");
  auto state_in =
    !persist ?
    PipeState{pipe,num_signals,num_listeners,typeless} :
    PipeState(pipe,typeless);
  pipe_state_.emplace(
    std::piecewise_construct,
    std::forward_as_tuple(pipe),
    std::forward_as_tuple(std::move(state_in))
  );
  iter = pipe_state_.find(pipe);
  assert(iter != pipe_state_.end() && "State must exist for new pipe");
  auto& state = iter->second;
  /*
   *  Increment (manually) the number of registered listeners
   */
  for (int i = 0; i < num_reg_listeners; i++) {
    state.listenerReg();
  }
  if (fn) {
    state.setDispatch(fn);
  }
}

PipeType PipeManagerBase::makeCallbackFuncVoid(
  bool const& persist, FuncType fn, bool const& dispatch, RefType num_signals,
  RefType num_listeners
) {
  using SignalType = signal::SignalVoid;

  bool const& send_back = false;
  auto const& pipe_id = makePipeID(persist,send_back);
  auto dispatch_fn = [pipe_id](void*) {
    signal_holder_<SignalType>.deliverAll(pipe_id,nullptr);
  };
  DispatchFuncType dfn = nullptr;
  if (dispatch) {
    dfn = dispatch_fn;
  }
  /*
   *  Add a new entry for pipe state to track it on this node
   */
  newPipeState(pipe_id,persist,dispatch,num_signals,num_listeners,1,dfn);
  /*
   *  Register a listener for this signal to trigger the anon function when the
   *  callback is invoked
   */
  assert(num_listeners > 0 && "Number of listeners must be positive");
  auto const& num_refs = !persist ? num_signals / num_listeners : -1;
  auto closure = [fn](typename SignalType::DataType*){ fn(); };
  auto anon = std::make_unique<callback::AnonListener<SignalType>>(
    closure, persist, num_refs
  );
  /*
   *  Do not update the pipe state since this newly registered listener is
   *  already counted by the `1' in the newPipeState(...,1)
   */
  registerCallback<SignalType>(pipe_id,std::move(anon),false);

  return pipe_id;
}

void PipeManagerBase::addListenerVoid(PipeType const& pipe, FuncType fn) {
  using SignalType = signal::SignalVoid;
  auto closure = [fn](typename SignalType::DataType*){ fn(); };
  return addListener<typename SignalType::DataType>(pipe, closure);
}

void PipeManagerBase::generalSignalTrigger(PipeType const& pipe) {
  auto iter = pipe_state_.find(pipe);
  assert(iter != pipe_state_.end() && "Must exist");
  auto& state = iter->second;
  state.signalRecv();
  auto const& automatic = state.isAutomatic();
  if (automatic) {
    if (state.finished()) {
      // Remove from the list of active pipe states
      pipe_state_.erase(iter);
    }
  }
}

void PipeManagerBase::triggerPipe(PipeType const& pipe) {
  using SignalType = signal::SignalVoid;

  auto const& exists = signal_holder_<SignalType>.exists(pipe);

  debug_print(
    pipe, node,
    "PipeManagerBase: triggerPipe: pipe={:x}, exists={}: delivering\n",
    pipe, exists
  );

  if (exists) {
    signal_holder_<SignalType>.deliverAll(pipe, nullptr);
  } else {
    triggerPipeUnknown<void>(pipe,nullptr);
  }

  generalSignalTrigger(pipe);
}

/*static*/ void PipeManagerBase::triggerCallbackHan(CallbackMsg* msg) {
  auto const& pid = msg->getPipe();

  debug_print(
    pipe, node,
    "PipeManagerBase: triggerCallbackHan pipe={:x}\n",
    pid
  );

  theCB()->triggerPipe(pid);
}

PipeType PipeManagerBase::makePipeID(bool const persist, bool const send_back) {
  auto const& this_node = theContext()->getNode();
  auto const next_id = cur_pipe_id_++;
  auto const pipe_id = PipeIDBuilder::createPipeID(
    next_id,this_node,send_back,persist
  );
  return pipe_id;
}

}} /* end namespace vt::pipe */
