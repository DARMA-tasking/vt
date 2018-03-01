
#if !defined INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_IMPL_H

#include "config.h"
#include "vrt/collection/destroy/destroyable.h"
#include "vrt/proxy/base_wrapper.h"
#include "vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
Destroyable<IndexT>::Destroyable(VirtualProxyType const in_proxy)
  : BaseEntireCollectionProxy<IndexT>(in_proxy)
{ }

template <typename IndexT>
template <typename ColT>
void Destroyable<IndexT>::destroy() {
  return theCollection()->destroy<ColT,IndexT>(this->proxy_);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_IMPL_H*/
