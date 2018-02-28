
#if !defined INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_STATUS_H
#define INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_STATUS_H

#include "config.h"

namespace vt { namespace vrt { namespace collection {

enum struct MigrateStatus : int16_t {
  MigratedToRemote = 0,
  MigrateInLocal,
  NoMigrationNecessary,
  MigrateError,
  PendingLocatAction
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_STATUS_H*/
