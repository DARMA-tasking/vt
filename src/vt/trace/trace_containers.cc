#include "vt/trace/trace_common.h"
#include "vt/trace/trace_containers.h"

#include <memory>

namespace vt { namespace trace {

// These MUST have static-lifetime and be initialized during constant
// initialization as they are modified during dynamic initialization as
// that is when auto-handler events are registered.
// Using a constexpr constructor / initializer list is also relevant.
// - https://en.cppreference.com/w/cpp/language/constant_initialization

/*static*/ TraceContainerEventClassType*
TraceContainers::getEventTypeContainer(){
  static std::unique_ptr<TraceContainerEventClassType> event_type_container_ = nullptr;
  if (event_type_container_ == nullptr) {
    event_type_container_ = std::make_unique<TraceContainerEventClassType>();
  }
  return event_type_container_.get();
}

/*static*/ TraceContainerEventType*
TraceContainers::getEventContainer(){
  static std::unique_ptr<TraceContainerEventType> event_container_ = nullptr;
  if (event_container_ == nullptr) {
    event_container_ = std::make_unique<TraceContainerEventType>();
  }
  return event_container_.get();
}

}} //end namespace vt::trace
