
#if ! defined __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC__
#define __RUNTIME_TRANSPORT_AUTO_REGISTRY_VC__

#include "config.h"
#include "auto_registry_common.h"
#include "auto_registry_general.h"
#include "registry.h"

#include "registry_function.h"
#include "context/context_vrt.h"
#include "context_vrt_fwd.h"

#include <vector>
#include <memory>

namespace vt { namespace auto_registry {

template <typename = void>
AutoActiveVCContainerType& getAutoRegistryVC();

template <typename ActiveFnT>
struct RegistrarVC {
  AutoHandlerType index;

  RegistrarVC();
};

AutoActiveVCType getAutoHandlerVC(HandlerType const& handler);

template <
  typename VirtualContextT,
  typename MessageT,
  ActiveVCFunctionType<MessageT, VirtualContextT>* f
>
HandlerType makeAutoHandlerVC(MessageT* const msg);

template <typename ActiveFnT>
struct RegistrarWrapperVC {
  RegistrarVC<ActiveFnT> registrar;
};

template <typename ActiveFnT>
AutoHandlerType registerActiveFnVC();

template <typename F, F* f>
struct FunctorAdapterVC {
  using FunctionPtrType = F;

  static constexpr F* getFunction() { return f; }

  template <typename... A>
  auto operator()(A&&... a) -> decltype(f(std::forward<A>(a)...)) {
    return f(std::forward<A>(a)...);
   }
};

template <typename Callable>
struct RunnableVC {
  using FunctionPtrType = typename Callable::FunctionPtrType;

  static AutoHandlerType const idx;

  static constexpr FunctionPtrType* getFunction();

  RunnableVC() = default;
};

template <typename ActiveFnT>
AutoHandlerType const RunnableVC<ActiveFnT>::idx =
  registerActiveFnVC<RunnableVC<ActiveFnT>>();

}} // end namespace vt::auto_registry

#include "auto_registry_vc_impl.h"

#endif /*__RUNTIME_TRANSPORT_AUTO_REGISTRY_VC__*/
