
#if !defined INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_H
#define INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_H

#include "config.h"
#include "vrt/proxy/base_collection.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct ElmInsertable : Sendable<ColT,IndexT> {
  ElmInsertable() = default;
  ElmInsertable(
    typename BaseCollectionProxy<ColT, IndexT>::ProxyType const& in_proxy,
    typename BaseCollectionProxy<ColT, IndexT>::ElementProxyType const& in_elm
  );

  template <typename SerializerT>
  void serialize(SerializerT& s);

  void insert(NodeType node = uninitialized_destination);
  void finishedInserting(ActionType action = nullptr);
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_INSERT_INSERTABLE_H*/
