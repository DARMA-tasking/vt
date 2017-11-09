
#if !defined INCLUDED_VRT_VRT_COMMON_H
#define INCLUDED_VRT_VRT_COMMON_H

#include "config.h"

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

  VrtElmProxy() = default;
  VrtElmProxy(VrtElmProxy const&) = default;
  VrtElmProxy(VrtElmProxy&&) = default;
  VrtElmProxy& operator=(VrtElmProxy const&) = default;

  bool operator==(VrtElmProxy const& other) const {
    return other.colProxy == colProxy and other.elmProxy == elmProxy;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | colProxy | elmProxy;
  }

};

using VirtualElmProxyType = VrtElmProxy;

}} /* end namespace vt::vrt */

namespace std {
  template<>
  struct hash<::vt::vrt::VrtElmProxy> {
    size_t operator()(::vt::vrt::VrtElmProxy const& in) const {
      return
        std::hash<decltype(in.colProxy)>()(in.colProxy) +
        std::hash<decltype(in.elmProxy)>()(in.elmProxy);
    }
  };
}

#endif /*INCLUDED_VRT_VRT_COMMON_H*/
