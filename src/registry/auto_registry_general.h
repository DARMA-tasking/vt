#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_GENERAL__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_GENERAL__

#include "config.h"
#include "auto_registry_common.h"

namespace vt { namespace auto_registry {

template <typename F, F* f>
struct FunctorAdapter {
  using FunctionPtrType = F;

  static constexpr F* getFunction() { return f; }

  template <typename... A>
  auto operator()(A&&... a) -> decltype(f(std::forward<A>(a)...)) {
    return f(std::forward<A>(a)...);
   }
};

}} // end namespace vt::auto_registry

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_GENERAL__*/
