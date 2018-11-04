
#if !defined INCLUDED_CONTEXT_VRT
#define INCLUDED_CONTEXT_VRT

#include "vt/config.h"
#include "vt/vrt/context/context_vrt_fwd.h"
#include "vt/vrt/context/context_vrtproxy.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/vrt/vrt_common.h"

namespace vt { namespace vrt {

struct VirtualContext : VrtBase {
  VirtualContext() = default;
  VirtualContext(bool const in_is_main) : is_main(in_is_main) { }

  friend struct VirtualContextAttorney;

private:

private:
  bool is_main = false;
  SeedType seed_ = 0;
  VirtualProxyType proxy_ = no_vrt_proxy;
};

struct MainVirtualContext : VirtualContext {
  MainVirtualContext() : VirtualContext(true) { }
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT*/
