#include "vt/trace/trace_common.h"
#include "vt/trace/trace_containers.h"

#include <memory>

namespace vt { namespace trace {

// These MUST have static-lifetime and be initialized during constant
// initialization as they are modified during dynamic initialization as
// that is when auto-handler events are registered.
// Using a constexpr constructor / initializer list is also relevant.
// - https://en.cppreference.com/w/cpp/language/constant_initialization
// static TraceContainerEventClassType event_type_container_  = std::make_unique<TraceContainerEventClassType>();
// static TraceContainerEventType event_container_ = std::make_unique<TraceContainerEventType>();

/*static*/ TraceContainerEventClassType TraceContainers::event_type_container_;
/*static*/ TraceContainerEventType TraceContainers::event_container_;

}} //end namespace vt::trace
