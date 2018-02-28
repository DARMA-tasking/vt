
#if !defined INCLUDED_VRT_PROXY_PROXY_H
#define INCLUDED_VRT_PROXY_PROXY_H

#include "config.h"
#include "vrt/collection/manager.fwd.h"
#include "vrt/collection/send/sendable.h"
#include "vrt/proxy/proxy_element.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct VrtElmProxy : Sendable<IndexT> {
  using CollectionProxyType = typename Sendable<IndexT>::ProxyType;
  using ElementProxyType = typename Sendable<IndexT>::ElementProxyType;

  VrtElmProxy(
    VirtualProxyType const& in_col_proxy,
    VirtualProxyElementType<IndexT> const& in_elm_proxy
  ) : Sendable<IndexT>(in_col_proxy, in_elm_proxy)
  { }

  VrtElmProxy(
    VirtualProxyType const& in_col_proxy, IndexT const& in_index
  ) : VrtElmProxy(in_col_proxy, ElementProxyType{in_index})
  { }

  VrtElmProxy() = default;
  VrtElmProxy(VrtElmProxy const&) = default;
  VrtElmProxy(VrtElmProxy&&) = default;
  VrtElmProxy& operator=(VrtElmProxy const&) = default;

  bool operator==(VrtElmProxy const& other) const {
    return other.col_proxy_ == this->col_proxy_ &&
           other.elm_proxy_ == this->elm_proxy_;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    Sendable<IndexT>::serialize(s);
  }

  friend struct CollectionManager;
};

}}} /* end namespace vt::vrt::collection */

template <typename IndexT>
using ElmProxyType = ::vt::vrt::collection::VrtElmProxy<IndexT>;

namespace std {
  template <typename IndexT>
  struct hash<ElmProxyType<IndexT>> {
    size_t operator()(ElmProxyType<IndexT> const& in) const {
      return
        std::hash<typename ElmProxyType<IndexT>::CollectionProxyType>()(
          in.getCollectionProxy()
        ) +
        std::hash<typename ElmProxyType<IndexT>::ElementProxyType>()(
          in.getElementProxy()
        );
    }
  };
}

#endif /*INCLUDED_VRT_PROXY_PROXY_H*/
