
#if !defined INCLUDED_CONTEXT_VRT
#define INCLUDED_CONTEXT_VRT

#include "config.h"
#include "utils/bits/bits_common.h"
#include "context_vrt_fwd.h"
#include "context_vrtproxy.h"

namespace vt { namespace vrt {

struct VirtualContext {
  VirtualContext() = default;

  VirtualProxyType getProxy() const {
    return proxy_;
  }

  friend struct VirtualContextManager;

private:
  SeedType seed_ = 0;
  VirtualProxyType proxy_ = no_vrt_proxy;
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT*/
