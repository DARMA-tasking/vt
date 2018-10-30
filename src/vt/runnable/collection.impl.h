
#if !defined INCLUDED_RUNNABLE_COLLECTION_IMPL_H
#define INCLUDED_RUNNABLE_COLLECTION_IMPL_H

#include "config.h"
#include "runnable/collection.h"
#include "registry/auto/auto_registry_common.h"
#include "registry/auto/auto_registry_interface.h"
#include "registry/auto/collection/auto_registry_collection.h"
#include "trace/trace_common.h"
#include "messaging/envelope.h"

namespace vt { namespace runnable {

template <typename MsgT, typename ElementT>
/*static*/ void RunnableCollection<MsgT,ElementT>::run(
  HandlerType handler, MsgT* msg, ElementT* elm, NodeType from_node,
  bool member, uint64_t idx
) {
  #if backend_check_enabled(trace_enabled)
    trace::TraceEntryIDType trace_id = auto_registry::theTraceID(
      handler, auto_registry::RegistryTypeEnum::RegVrtCollection
    );
    trace::TraceEventIDType trace_event = envelopeGetTraceEvent(msg->env);
  #endif

  #if backend_check_enabled(trace_enabled)
    theTrace()->beginProcessing(
      trace_id, sizeof(*msg), trace_event, from_node,
      trace::Trace::getCurrentTime(), idx
    );
  #endif

  if (member) {
    auto const func = auto_registry::getAutoHandlerCollectionMem(handler);
    (elm->*func)(msg);
  } else {
    auto const func = auto_registry::getAutoHandlerCollection(handler);
    func(msg, elm);
  };

  #if backend_check_enabled(trace_enabled)
    theTrace()->endProcessing(
      trace_id, sizeof(*msg), trace_event, from_node,
      trace::Trace::getCurrentTime(), idx
    );
  #endif
}

}} /* end namespace vt::runnable */

#endif /*INCLUDED_RUNNABLE_COLLECTION_IMPL_H*/
