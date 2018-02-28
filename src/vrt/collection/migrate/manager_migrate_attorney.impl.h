
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_IMPL_H
#define INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/migrate/manager_migrate_attorney.h"
#include "vrt/collection/migrate/migrate_status.h"
#include "vrt/collection/types/migratable.fwd.h"
#include "vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
/*static*/ MigrateStatus CollectionElmAttorney<ColT, IndexT>::migrateOut(
  VirtualProxyType const& proxy, IndexT const& index, NodeType const& node
) {
  return theCollection()->migrateOut<ColT,IndexT>(proxy,index,node);
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_IMPL_H*/
