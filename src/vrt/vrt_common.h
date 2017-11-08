
#if !defined INCLUDED_VRT_VRT_COMMON_H
#define INCLUDED_VRT_VRT_COMMON_H

#include "config.h"
#include "vrt/context/context_vrt_fwd.h"
#include "vrt/context/context_vrtproxy.h"

namespace vt { namespace vrt {

struct VrtBase {
  VirtualProxyType getProxy() const { return proxy_; }

protected:
  void setProxy(VirtualProxyType const& in_proxy) { proxy_ = in_proxy; }

private:
  VirtualProxyType proxy_ = no_vrt_proxy;
};

struct VrtElmProxy {
  VirtualProxyType colProxy = no_vrt_proxy;
  VirtualElmOnlyProxyType elmProxy = no_vrt_proxy;

  explicit VrtElmProxy(
    VirtualProxyType const& colProxy_, VirtualElmOnlyProxyType const& elmProxy_
  ) : colProxy(colProxy_), elmProxy(elmProxy_)
  { }
};

using VirtualElmProxyType = VrtElmProxy;

}} /* end namespace vt::vrt */

#endif /*INCLUDED_VRT_VRT_COMMON_H*/
