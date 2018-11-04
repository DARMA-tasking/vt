
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_H
#define INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/types/migratable.fwd.h"
#include "vt/vrt/collection/types/base.fwd.h"
#include "vt/vrt/collection/migrate/migrate_status.h"
#include "vt/vrt/collection/migrate/migrate_handlers.fwd.h"

#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct Collection;

template <typename ColT, typename IndexT>
struct CollectionElmAttorney {
  using CollectionType = CollectionBase<ColT, IndexT>;
  using VirtualPtrType = std::unique_ptr<CollectionType>;

  friend struct CollectionBase<ColT, IndexT>;
  friend struct MigrateHandlers;
  friend struct Migratable<ColT>;

private:
  static MigrateStatus migrateOut(
    VirtualProxyType const& proxy, IndexT const& idx, NodeType const& dest
  );

  static MigrateStatus migrateIn(
    VirtualProxyType const& proxy, IndexT const& idx, NodeType const& from,
    VirtualPtrType vc_elm, IndexT const& range, HandlerType const& map_han
  );
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_MIGRATE_ATTORNEY_H*/
