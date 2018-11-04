
#if !defined INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_HANDLERS_H
#define INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_HANDLERS_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/migrate/migrate_msg.h"
#include "vt/vrt/collection/migrate/migrate_handlers.fwd.h"

namespace vt { namespace vrt { namespace collection {

struct MigrateHandlers {
  template <typename ColT, typename IndexT>
  static void migrateInHandler(MigrateMsg<ColT, IndexT>* msg);
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/migrate/migrate_handlers.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_HANDLERS_H*/
