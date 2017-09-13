
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY__

#include "auto_registry_common.h"
#include "common.h"
#include "registry.h"
#include "demangle.h"
#include "trace.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

template <typename = void>
AutoActiveContainerType& getAutoRegistry();

template <typename ActiveFnT>
struct Registrar {
  AutoHandlerType index;

  Registrar();
};

AutoActiveType getAutoHandler(HandlerType const& handler);

template <typename MessageT, ActiveAnyFunctionType<MessageT>* f>
HandlerType makeAutoHandler(MessageT* const msg);

template <typename T, T value>
HandlerType makeAutoHandler();

template <typename ActiveFnT>
struct RegistrarWrapper {
  Registrar<ActiveFnT> registrar;
};

template <typename ActiveFnT>
AutoHandlerType registerActiveFn();

template <typename F, F* f>
struct FunctorAdapter {
  using FunctionPtrType = F;

  static constexpr F* getFunction() { return f; }

  template <typename... A>
  auto operator()(A&&... a) -> decltype(f(std::forward<A>(a)...)) {
    return f(std::forward<A>(a)...);
   }
};

template <typename Callable>
struct Runnable {
  using FunctionPtrType = typename Callable::FunctionPtrType;

  static AutoHandlerType const idx;

  static constexpr FunctionPtrType* getFunction();

  Runnable() = default;
};

template <typename ActiveFnT>
AutoHandlerType const Runnable<ActiveFnT>::idx =
  registerActiveFn<Runnable<ActiveFnT>>();

}} // end namespace vt::auto_registry

// convenience macro for registration
#define GET_HANDLER_ACTIVE_FUNCTION_EXPAND(TYPE_F, ADD_F)               \
  vt::auto_registry::Runnable<                                     \
    decltype(                                                           \
      vt::auto_registry::FunctorAdapter<TYPE_F, ADD_F>()           \
    )>::idx;                                                            \

#include "auto_registry_impl.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY__*/
