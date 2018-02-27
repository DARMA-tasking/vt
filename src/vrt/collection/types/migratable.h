
#if !defined INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_H
#define INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/base/base.h"
#include "vrt/collection/types/untyped.h"
#include "vrt/collection/types/migrate_hooks.h"
#include "vrt/collection/types/migratable.fwd.h"

namespace vt { namespace vrt { namespace collection {

struct Migratable : MigrateHookBase, UntypedCollection {
  Migratable() = default;

  virtual void migrate(NodeType const& node) = 0;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_H*/
