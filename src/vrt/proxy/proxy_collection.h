
#if !defined INCLUDED_VRT_PROXY_PROXY_H
#define INCLUDED_VRT_PROXY_PROXY_H

#include "config.h"

namespace vt { namespace vrt { namespace collection {

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

}}} /* end namespace vt::vrt::collection */

namespace std {
  template<>
  struct hash<::vt::vrt::collection::VrtElmProxy> {
    size_t operator()(::vt::vrt::collection::VrtElmProxy const& in) const {
      return
        std::hash<decltype(in.colProxy)>()(in.colProxy) +
        std::hash<decltype(in.elmProxy)>()(in.elmProxy);
    }
  };
}

#endif /*INCLUDED_VRT_PROXY_PROXY_H*/
