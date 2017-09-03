
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_INTERFACE__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_INTERFACE__

#include "auto_registry_common.h"
#include "common.h"
#include "registry.h"

namespace runtime { namespace auto_registry {

template <typename MessageT, action_any_function_t<MessageT>* f>
handler_t make_auto_handler(MessageT* const msg);

template <typename T, T value>
handler_t make_auto_handler();

template <typename T, bool is_msg, typename... Args>
handler_t make_auto_handler_functor();

auto_active_t get_auto_handler(handler_t const& handler);

auto_active_functor_t get_auto_handler_functor(handler_t const& handler);

}} // end namespace runtime::auto_registry

#include "auto_registry.h"
#include "auto_registry_functor.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_INTERFACE__*/
