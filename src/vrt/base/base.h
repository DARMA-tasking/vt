
#if !defined INCLUDED_VRT_BASE_BASE_H
#define INCLUDED_VRT_BASE_BASE_H

#include "config.h"

namespace vt { namespace vrt {

struct VrtBase {
  VrtBase() = default;

  explicit VrtBase(VirtualProxyType const& in_proxy)
    : proxy_(in_proxy)
  { }

public:
  VirtualProxyType getProxy() const { return proxy_; }

protected:
  void setProxy(VirtualProxyType const& in_proxy) { proxy_ = in_proxy; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | proxy_;
  }

private:
  VirtualProxyType proxy_ = no_vrt_proxy;
};

}} /* end namespace vt::vrt */

#endif /*INCLUDED_VRT_BASE_BASE_H*/
