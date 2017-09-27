
#if !defined INCLUDED_CONTEXT_VRT
#define INCLUDED_CONTEXT_VRT

#include "config.h"
#include "utils/bits/bits_common.h"

#include "context_vrtproxy.h"

namespace vt { namespace vrt {

struct VrtContext {
  VrtContext_ProxyType myProxy_;

  VrtContext() = default;
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT*/
