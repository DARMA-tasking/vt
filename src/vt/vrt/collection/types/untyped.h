
#if !defined INCLUDED_VRT_COLLECTION_TYPES_UNTYPED_H
#define INCLUDED_VRT_COLLECTION_TYPES_UNTYPED_H

#include "vt/config.h"
#include "vt/lb/migration/lb_migratable.h"

namespace vt { namespace vrt { namespace collection {

/*
 *      Base untyped collection for safe casting
 */

struct UntypedCollection : VrtBase, HasMigrate {
  UntypedCollection() = default;

protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    VrtBase::serialize(s);
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_UNTYPED_H*/
