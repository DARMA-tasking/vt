
#if !defined INCLUDED_VRT_PROXY_PROXY_H
#define INCLUDED_VRT_PROXY_PROXY_H

#include "config.h"
#include "vrt/proxy/proxy_element.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct VrtElmProxy {
  VirtualProxyType colProxy = no_vrt_proxy;
  VirtualProxyElementType<IndexT> elmProxy{virtual_proxy_elm_empty_tag};

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

template <typename IndexT>
using ElmProxyType = ::vt::vrt::collection::VrtElmProxy<IndexT>;

namespace std {
  template <typename IndexT>
  struct hash<ElmProxyType<IndexT>> {
    size_t operator()(ElmProxyType<IndexT> const& in) const {
      return
        std::hash<decltype(in.colProxy)>()(in.colProxy) +
        std::hash<decltype(in.elmProxy)>()(in.elmProxy);
    }
  };
}

#endif /*INCLUDED_VRT_PROXY_PROXY_H*/
