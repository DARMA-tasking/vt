
#if !defined INCLUDED_VRT_COLLECTION_PROXY_TRAITS_PROXY_COL_TRAITS_H
#define INCLUDED_VRT_COLLECTION_PROXY_TRAITS_PROXY_COL_TRAITS_H

#include "vt/config.h"
#include "vt/vrt/collection/destroy/destroyable.h"
#include "vt/vrt/collection/reducable/reducable.h"
#include "vt/vrt/collection/broadcast/broadcastable.h"
#include "vt/vrt/collection/insert/insert_finished.h"
#include "vt/vrt/proxy/base_collection_proxy.h"

namespace vt { namespace vrt { namespace collection {

namespace col_proxy {

template <typename ColT, typename IndexT>
using Chain4 = InsertFinished<ColT,IndexT,BaseCollectionProxy<ColT,IndexT>>;

template <typename ColT, typename IndexT>
using Chain3 = Broadcastable<ColT,IndexT,Chain4<ColT,IndexT>>;

template <typename ColT, typename IndexT>
using Chain2 = Destroyable<ColT,IndexT,Chain3<ColT,IndexT>>;

template <typename ColT, typename IndexT>
using Chain1 = Reducable<ColT,IndexT,Chain2<ColT,IndexT>>;

} /* end namespace proxy */

template <typename ColT, typename IndexT>
struct ProxyCollectionTraits : col_proxy::Chain1<ColT,IndexT> {
  ProxyCollectionTraits() = default;
  ProxyCollectionTraits(ProxyCollectionTraits const&) = default;
  ProxyCollectionTraits(ProxyCollectionTraits&&) = default;
  explicit ProxyCollectionTraits(VirtualProxyType const in_proxy)
    : col_proxy::Chain1<ColT,IndexT>(in_proxy)
  {}
  ProxyCollectionTraits& operator=(ProxyCollectionTraits const&) = default;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_PROXY_TRAITS_PROXY_COL_TRAITS_H*/
