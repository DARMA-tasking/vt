
#if !defined INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_H
#define INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_H

#include "config.h"
#include "vrt/proxy/base_collection_proxy.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct Destroyable : BaseProxyT {
  Destroyable() = default;
  Destroyable(Destroyable const&) = default;
  Destroyable(Destroyable&&) = default;
  explicit Destroyable(VirtualProxyType const in_proxy);
  Destroyable& operator=(Destroyable const&) = default;

  void destroy();
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DESTROY_DESTROYABLE_H*/
