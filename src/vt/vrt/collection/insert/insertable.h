
#if !defined INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_H
#define INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_H

#include "vt/config.h"
#include "vt/vrt/collection/send/sendable.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct ElmInsertable : BaseProxyT {
  ElmInsertable() = default;
  ElmInsertable(
    typename BaseProxyT::ProxyType const& in_proxy,
    typename BaseProxyT::ElementProxyType const& in_elm
  );

  template <typename SerializerT>
  void serialize(SerializerT& s);

  void insert(NodeType node = uninitialized_destination) const;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_H*/
