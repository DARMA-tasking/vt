
#if !defined INCLUDED_RUNNABLE_GENERAL_IMPL_H
#define INCLUDED_RUNNABLE_GENERAL_IMPL_H

#include "vt/config.h"
#include "vt/runnable/general.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/registry/auto/auto_registry_general.h"
#include "vt/trace/trace_common.h"
#include "vt/messaging/envelope.h"
#include "vt/handler/handler.h"

#include <cassert>

namespace vt { namespace runnable {

template <typename MsgT>
/*static*/ void Runnable<MsgT>::run(
  HandlerType handler, ActiveFnPtrType func, MsgT* msg, NodeType from_node,
  TagType in_tag
) {
  using HandlerManagerType = HandlerManager;

  #if backend_check_enabled(trace_enabled)
    trace::TraceEntryIDType trace_id = auto_registry::theTraceID(
      handler, auto_registry::RegistryTypeEnum::RegGeneral
    );
    trace::TraceEventIDType trace_event = trace::no_trace_event;
    if (msg) {
      trace_event = envelopeGetTraceEvent(msg->env);
    }
  #endif

  #if backend_check_enabled(trace_enabled)
    theTrace()->beginProcessing(trace_id, sizeof(MsgT), trace_event, from_node);
  #endif

  bool is_functor = false;
  bool is_auto = false;
  bool bare_handler = false;
  auto_registry::NumArgsType num_args = 1;

  if (func == nullptr) {
    is_auto = HandlerManagerType::isHandlerAuto(handler);
    is_functor = HandlerManagerType::isHandlerFunctor(handler);

    if (is_auto && is_functor) {
      func = auto_registry::getAutoHandlerFunctor(handler);
      num_args = auto_registry::getAutoHandlerFunctorArgs(handler);
    } else if (is_auto) {
      func = auto_registry::getAutoHandler(handler);
    } else {
      auto typed_func = theRegistry()->getHandler(handler, in_tag);
      typed_func(msg);
      bare_handler = true;
    }
  }

  if (num_args == 0) {
    auto no_arg_fn = reinterpret_cast<FnParamType<>>(func);
    no_arg_fn();
  } else if (!bare_handler) {
    func(msg);
  }

  #if backend_check_enabled(trace_enabled)
    theTrace()->endProcessing(trace_id, sizeof(MsgT), trace_event, from_node);
  #endif
}

/*static*/ inline void RunnableVoid::run(
  HandlerType handler, NodeType from_node
) {

  using HandlerManagerType = HandlerManager;

  bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);

  ActiveFnPtrType func = nullptr;

  if (is_auto && is_functor) {
    func = auto_registry::getAutoHandlerFunctor(handler);
  } else if (is_auto) {
    func = auto_registry::getAutoHandler(handler);
  } else {
    vtAssert(0, "Must be auto handler");
  }

  auto void_fn = reinterpret_cast<FnParamType<>>(func);

  void_fn();
}

}} /* end namespace vt::runnable */

#endif /*INCLUDED_RUNNABLE_GENERAL_IMPL_H*/
