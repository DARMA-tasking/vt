
#if !defined INCLUDED_VRT_PROXY_COLLECTION_ELM_PROXY_H
#define INCLUDED_VRT_PROXY_COLLECTION_ELM_PROXY_H

#include "vt/config.h"
#include "vt/vrt/collection/proxy_traits/proxy_elm_traits.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/send/sendable.h"
#include "vt/vrt/collection/insert/insertable.h"
#include "vt/vrt/proxy/base_elm_proxy.h"

#include <ostream>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct VrtElmProxy : ProxyCollectionElmTraits<ColT, IndexT> {
  using CollectionProxyType =
    typename ProxyCollectionElmTraits<ColT, IndexT>::ProxyType;
  using ElementProxyType =
    typename ProxyCollectionElmTraits<ColT, IndexT>::ElementProxyType;

  VrtElmProxy(
    VirtualProxyType const& in_col_proxy,
    BaseElmProxy<ColT, IndexT> const& in_elm_proxy
  ) : ProxyCollectionElmTraits<ColT, IndexT>(in_col_proxy, in_elm_proxy)
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
    ProxyCollectionElmTraits<ColT, IndexT>::serialize(s);
  }

  template <typename ColU, typename IndexU>
  friend std::ostream& operator<<(
    std::ostream& os, VrtElmProxy<ColU,IndexU> const& vrt
  );

  friend struct CollectionManager;
};

template <typename ColT, typename IndexT>
std::ostream& operator<<(
  std::ostream& os, VrtElmProxy<ColT,IndexT> const& vrt
) {
  os << "("
     << "vrt=" << vrt.col_proxy_ << ","
     << "idx=" << vrt.elm_proxy_.getIndex()
     << ")";
  return os;
}

}}} /* end namespace vt::vrt::collection */

template <typename ColT, typename IndexT>
using ElmProxyType = ::vt::vrt::collection::VrtElmProxy<ColT, IndexT>;

namespace std {
template <typename ColT, typename IndexT>
struct hash<ElmProxyType<ColT, IndexT>> {
  size_t operator()(ElmProxyType<ColT, IndexT> const& in) const {
    return
      std::hash<typename ElmProxyType<ColT, IndexT>::CollectionProxyType>()(
        in.getCollectionProxy()
      ) +
      std::hash<typename ElmProxyType<ColT, IndexT>::ElementProxyType>()(
        in.getElementProxy()
      );
  }
};
}

#endif /*INCLUDED_VRT_PROXY_COLLECTION_ELM_PROXY_H*/
