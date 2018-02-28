
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_H
#define INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/types/migratable.fwd.h"
#include "vrt/collection/types/base.fwd.h"
#include "vrt/collection/migrate/migrate_status.h"
#include "vrt/collection/migrate/migrate_handlers.fwd.h"

#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct CollectionElmAttorney {
  friend struct CollectionBase<IndexT>;
  friend struct MigrateHandlers;
  friend struct Migratable;

private:
  static MigrateStatus migrateOut(
    VirtualProxyType const& proxy, IndexT const& idx, NodeType const& dest
  );

  template <typename UniquePtrT>
  static MigrateStatus migrateIn(
    VirtualProxyType const& proxy, IndexT const& idx, NodeType const& from,
    UniquePtrT vc_elm
  );
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_H*/
