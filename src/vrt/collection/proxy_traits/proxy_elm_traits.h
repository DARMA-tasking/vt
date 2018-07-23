
#if !defined INCLUDED_VRT_COLLECTION_PROXY_TRAITS_PROXY_ELM_TRAITS_H
#define INCLUDED_VRT_COLLECTION_PROXY_TRAITS_PROXY_ELM_TRAITS_H

#include "config.h"
#include "vrt/proxy/base_collection_elm_proxy.h"
#include "vrt/proxy/base_elm_proxy.h"
#include "vrt/collection/send/sendable.h"
#include "vrt/collection/insert/insertable.h"

namespace vt { namespace vrt { namespace collection {

namespace elm_proxy {

template <typename ColT, typename IndexT>
using Chain2 = ElmInsertable<ColT,IndexT,BaseCollectionElmProxy<ColT,IndexT>>;

template <typename ColT, typename IndexT>
using Chain1 = Sendable<ColT,IndexT,Chain2<ColT,IndexT>>;

} /* end namespace proxy */

template <typename ColT, typename IndexT>
struct ProxyCollectionElmTraits : elm_proxy::Chain1<ColT,IndexT> {
  ProxyCollectionElmTraits() = default;
  ProxyCollectionElmTraits(ProxyCollectionElmTraits const&) = default;
  ProxyCollectionElmTraits(ProxyCollectionElmTraits&&) = default;
  ProxyCollectionElmTraits(
    typename elm_proxy::Chain1<ColT,IndexT>::ProxyType const& in_proxy,
    typename elm_proxy::Chain1<ColT,IndexT>::ElementProxyType const& in_elm
  ) : elm_proxy::Chain1<ColT,IndexT>(in_proxy,in_elm)
  {}
  ProxyCollectionElmTraits& operator=(ProxyCollectionElmTraits const&) = default;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_PROXY_TRAITS_PROXY_ELM_TRAITS_H*/
