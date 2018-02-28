
#if !defined INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H
#define INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/types/untyped.h"

namespace vt { namespace vrt { namespace collection {

struct MigrateHookInterface : UntypedCollection {
  virtual void onMigrateAway() = 0;
  virtual void onMigrateTo() = 0;
protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    UntypedCollection::serialize(s);
  }
};

struct MigrateHookBase : MigrateHookInterface {
  virtual void onMigrateAway() override {}
  virtual void onMigrateTo() override {}
protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    MigrateHookInterface::serialize(s);
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_MIGRATE_HOOKS_H*/
