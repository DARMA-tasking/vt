
#if !defined INCLUDED_RUNNABLE_VRT_IMPL_H
#define INCLUDED_RUNNABLE_VRT_IMPL_H

#include "vt/config.h"
#include "vt/runnable/vrt.h"
#include "vt/registry/auto/auto_registry_common.h"
#include "vt/registry/auto/auto_registry_interface.h"
#include "vt/registry/auto/vc/auto_registry_vc.h"
#include "vt/trace/trace_common.h"
#include "vt/messaging/envelope.h"

namespace vt { namespace runnable {

template <typename MsgT, typename ElementT>
/*static*/ void RunnableVrt<MsgT,ElementT>::run(
  HandlerType handler, MsgT* msg, ElementT* elm, NodeType from_node
) {
  #if backend_check_enabled(trace_enabled)
    trace::TraceEntryIDType trace_id = auto_registry::theTraceID(
      handler, auto_registry::RegistryTypeEnum::RegVrt
    );
    trace::TraceEventIDType trace_event = envelopeGetTraceEvent(msg->env);
  #endif

  #if backend_check_enabled(trace_enabled)
    theTrace()->beginProcessing(trace_id, sizeof(*msg), trace_event, from_node);
  #endif

  auto const func = auto_registry::getAutoHandlerVC(handler);
  func(msg, elm);

  #if backend_check_enabled(trace_enabled)
    theTrace()->endProcessing(trace_id, sizeof(*msg), trace_event, from_node);
  #endif
}

}} /* end namespace vt::runnable */

#endif /*INCLUDED_RUNNABLE_VRT_IMPL_H*/
