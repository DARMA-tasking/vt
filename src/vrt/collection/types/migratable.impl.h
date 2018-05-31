
#if !defined INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_IMPL_H

#include "config.h"
#include "vrt/collection/types/migratable.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT>
template <typename Serializer>
void Migratable<ColT>::serialize(Serializer& s) {
  MigrateHookBase::serialize(s);
}

template <typename ColT>
/*virtual*/ void Migratable<ColT>::destroy() {
  debug_print(
    vrt_coll, node,
    "Migratable<ColT>::destroy(): this={}\n", print_ptr(this)
  );
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_MIGRATABLE_IMPL_H*/
