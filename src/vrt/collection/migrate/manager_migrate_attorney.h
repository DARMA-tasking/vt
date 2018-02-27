
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_H
#define INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/types/migratable.fwd.h"
#include "vrt/collection/types/base.fwd.h"
#include "vrt/collection/migrate/migrate_status.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct CollectionElmAttorney {
  friend struct CollectionBase<IndexT>;
  friend struct Migratable;

private:
  static MigrateStatus migrate(
    VirtualProxyType const& proxy, IndexT const& index, NodeType const& node
  );
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_H*/
