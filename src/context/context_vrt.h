
#if !defined INCLUDED_CONTEXT_VRT
#define INCLUDED_CONTEXT_VRT

#include "config.h"
#include "utils/bits/bits_common.h"
#include "context_vrt_fwd.h"
#include "context_vrtproxy.h"

namespace vt { namespace vrt {

struct VrtContext {
  VrtContext() = default;

  VrtContext_ProxyType getProxy() const {
    return proxy_;
  }

  friend struct VrtContextManager;

private:
  VrtContext_ProxyType proxy_ = no_vrt_proxy;
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT*/
