
#if !defined INCLUDED_CONTEXT_VRT
#define INCLUDED_CONTEXT_VRT

#include "config.h"
#include "utils/bits/bits_common.h"
#include "context_vrt_fwd.h"
#include "context_vrtproxy.h"

namespace vt { namespace vrt {

struct VirtualContext {
  VirtualContext() = default;
  VirtualContext(bool const in_is_main) : is_main(in_is_main) { }

  friend struct VirtualContextAttorney;

  VirtualProxyType getProxy() const { return proxy_; }

private:
  void setProxy(VirtualProxyType const& in_proxy) { proxy_ = in_proxy; }

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
