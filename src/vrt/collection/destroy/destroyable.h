
#if !defined INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_H
#define INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_H

#include "config.h"
#include "vrt/proxy/base_wrapper.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Destroyable : BaseEntireCollectionProxy<IndexT> {
  Destroyable() = default;
  Destroyable(VirtualProxyType const in_proxy);

  template <typename ColT>
  void destroy();
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_H*/
