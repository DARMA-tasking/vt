#include "vt/trace/trace_common.h"
#include "vt/trace/trace_containers.h"

namespace vt { namespace trace {

/*static*/ TraceContainerEventClassType*
TraceContainers::event_type_container_{nullptr};

/*static*/ TraceContainerEventType*
TraceContainers::event_container_{nullptr};

}} //end namespace vt::trace
