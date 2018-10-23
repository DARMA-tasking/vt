
#if !defined INCLUDED_VRT_COLLECTION_GETTABLE_GETTABLE_H
#define INCLUDED_VRT_COLLECTION_GETTABLE_GETTABLE_H

#include "config.h"
#include "vrt/collection/send/sendable.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct Gettable : BaseProxyT {
  Gettable() = default;
  Gettable(
    typename BaseProxyT::ProxyType const& in_proxy,
    typename BaseProxyT::ElementProxyType const& in_elm
  );

  template <typename SerializerT>
  void serialize(SerializerT& s);

  ColT* tryGetLocalPtr();
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_GETTABLE_GETTABLE_H*/
