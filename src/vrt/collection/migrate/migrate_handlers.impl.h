
#if !defined INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_HANDLERS_IMPL_H
#define INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_HANDLERS_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/migrate/migrate_msg.h"
#include "vrt/collection/migrate/migrate_handlers.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
/*static*/ void MigrateHandlers::migrateInHandler(
  MigrateMsg<ColT, IndexT>* msg
) {
  auto const& proxy = msg->getElementProxy();

  // auto const& migrate_status = CollectionElmAttorney<ColT, IndexT>::migrateIn(
  //   proxy.
  // );
  // assert(
  //   migrate_status == MigrateStatus::MigratedToRemote &&
  //   "Required be immediate, valid migration currently"
  // );
}

}}} /* end namespace vt::vrt::collection */


#endif /*INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_HANDLERS_IMPL_H*/
