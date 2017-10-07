#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
inline HandlerType makeAutoHandler(MessageT* const __attribute__((unused)) msg) {
  HandlerType const id = GET_HANDLER_ACTIVE_FUNCTION_EXPAND(
    ActiveAnyFunctionType<MessageT>, f
  );
  return HandlerManagerType::makeHandler(true, false, id);
}

template <typename T, T value>
inline HandlerType makeAutoHandler() {
  HandlerType const id = GET_HANDLER_ACTIVE_FUNCTION_EXPAND(T, value);
  return HandlerManagerType::makeHandler(true, false, id);
}

inline AutoActiveType getAutoHandler(HandlerType const& handler) {
  auto const& han_id = HandlerManagerType::getHandlerIdentifier(handler);

  bool const& is_auto = HandlerManagerType::isHandlerAuto(handler);
  bool const& is_functor = HandlerManagerType::isHandlerFunctor(handler);

  debug_print(
    handler, node,
    "get_auto_handler: handler=%d, id=%d, is_auto=%s, is_functor=%s\n",
    handler, han_id, print_bool(is_auto), print_bool(is_functor)
  );

  assert(
    not is_functor and is_auto and "Handler should not be a functor, but auto"
  );

  return getAutoRegistryGen<AutoActiveContainerType>().at(han_id).getFun();
}

}} // end namespace vt::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_IMPL__*/
