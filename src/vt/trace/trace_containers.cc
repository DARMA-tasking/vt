#include "vt/trace/trace_common.h"
#include "vt/trace/trace_containers.h"

#include <memory>

namespace vt { namespace trace {

/*static*/ TraceContainerEventClassType*
TraceContainers::getEventTypeContainer(){
  // n.b. Container MUST be constant-initialized as this function is used
  // from dynamic initialization contexts (ie. auto registrations).
  // static INSIDE function to avoid Clang 3.9 bug; issue not present Clang 5+.
  static std::unique_ptr<TraceContainerEventClassType> event_type_container_ = nullptr;

  if (event_type_container_ == nullptr) {
    event_type_container_ = std::make_unique<TraceContainerEventClassType>();
  }
  return event_type_container_.get();
}

/*static*/ TraceContainerEventType*
TraceContainers::getEventContainer(){
  // n.b. Container MUST be constant-initialized as this function is used
  // from dynamic initialization contexts (ie. auto registrations).
  // static INSIDE function to avoid Clang 3.9 bug; fixed Clang 5+.
  static std::unique_ptr<TraceContainerEventType> event_container_ = nullptr;

  if (event_container_ == nullptr) {
    event_container_ = std::make_unique<TraceContainerEventType>();
  }
  return event_container_.get();
}

}} //end namespace vt::trace
