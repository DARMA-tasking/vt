
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_IMPL_H
#define INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_IMPL_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/migrate/manager_migrate_attorney.h"
#include "vt/vrt/collection/migrate/migrate_status.h"
#include "vt/vrt/collection/types/migratable.fwd.h"
#include "vt/vrt/collection/manager.fwd.h"

#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
/*static*/ MigrateStatus CollectionElmAttorney<ColT, IndexT>::migrateOut(
  VirtualProxyType const& proxy, IndexT const& idx, NodeType const& dest
) {
  return theCollection()->migrateOut<ColT,IndexT>(proxy,idx,dest);
}

template <typename ColT, typename IndexT>
/*static*/ MigrateStatus CollectionElmAttorney<ColT, IndexT>::migrateIn(
  VirtualProxyType const& proxy, IndexT const& idx, NodeType const& from,
  VirtualPtrType vc_elm, IndexT const& range, HandlerType const& map_han
) {
  return theCollection()->migrateIn<ColT,IndexT>(
    proxy, idx, from, std::move(vc_elm), range, map_han
  );
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_IMPL_H*/
