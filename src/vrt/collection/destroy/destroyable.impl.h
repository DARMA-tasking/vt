
#if !defined INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_IMPL_H

#include "config.h"
#include "vrt/collection/destroy/destroyable.h"
#include "vrt/proxy/base_wrapper.h"
#include "vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
Destroyable<ColT, IndexT>::Destroyable(VirtualProxyType const in_proxy)
  : BaseEntireCollectionProxy<ColT, IndexT>(in_proxy)
{ }

template <typename ColT, typename IndexT>
void Destroyable<ColT, IndexT>::destroy() {
  return theCollection()->destroy<ColT,IndexT>(this->getProxy());
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_IMPL_H*/
