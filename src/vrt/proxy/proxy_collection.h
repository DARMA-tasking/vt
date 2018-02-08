
#if !defined INCLUDED_VRT_PROXY_PROXY_H
#define INCLUDED_VRT_PROXY_PROXY_H

#include "config.h"
#include "vrt/proxy/proxy_element.h"
#include "vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct VrtElmProxy {
  using CollectionProxyType = VirtualProxyType;
  using ElementProxyType = VirtualProxyElementType<IndexT>;

  VrtElmProxy(
    VirtualProxyType const& in_col_proxy,
    VirtualProxyElementType<IndexT> const& in_elm_proxy
  ) : col_proxy_(in_col_proxy), elm_proxy_(in_elm_proxy)
  { }

  VrtElmProxy(
    VirtualProxyType const& in_col_proxy, IndexT const& in_index
  ) : col_proxy_(in_col_proxy), elm_proxy_(ElementProxyType{in_index})
  { }

  VrtElmProxy() = default;
  VrtElmProxy(VrtElmProxy const&) = default;
  VrtElmProxy(VrtElmProxy&&) = default;
  VrtElmProxy& operator=(VrtElmProxy const&) = default;

  bool operator==(VrtElmProxy const& other) const {
    return other.col_proxy_ == col_proxy_ && other.elm_proxy_ == elm_proxy_;
  }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | col_proxy_ | elm_proxy_;
  }

  friend struct CollectionManager;

  CollectionProxyType getCollectionProxy() const { return col_proxy_; }
  ElementProxyType getElementProxy() const { return elm_proxy_; }

protected:
  CollectionProxyType col_proxy_ = no_vrt_proxy;
  ElementProxyType elm_proxy_{virtual_proxy_elm_empty_tag};
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
