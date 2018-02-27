
#if !defined INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H
#define INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H

#include "config.h"
#include "vrt/vrt_common.h"

namespace vt { namespace vrt { namespace collection {

struct MigrateHookBase {
  virtual void onMigrateAway() = 0;
  virtual void onMigrateTo() = 0;
};

struct MigrateHookInterface : MigrateHookBase {
  virtual void onMigrateAway() override {}
  virtual void onMigrateTo() override {}
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H*/
