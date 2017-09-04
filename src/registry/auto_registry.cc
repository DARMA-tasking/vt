
#include "auto_registry_common.h"
#include "auto_registry_interface.h"
#include "common.h"
#include "registry.h"

namespace runtime { namespace auto_registry {

#if backend_check_enabled(trace_enabled)
trace::trace_ep_t
get_trace_id(handler_t const& handler) {
  auto const& han_id = handler_manager_t::get_handler_identifier(handler);

  bool const& is_functor = handler_manager_t::is_handler_functor(handler);

  if (not is_functor) {
    return get_auto_registry().at(han_id).get_trace_id();
  } else {
    return get_auto_registry_functor().at(han_id).get_trace_id();
  }
}
#endif

}} // end namespace runtime::auto_registry
