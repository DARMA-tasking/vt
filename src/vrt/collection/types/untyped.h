
#if !defined INCLUDED_VRT_COLLECTION_TYPES_UNTYPED_H
#define INCLUDED_VRT_COLLECTION_TYPES_UNTYPED_H

#include "config.h"

namespace vt { namespace vrt { namespace collection {

/*
 *      Base untyped collection for safe casting
 */

struct UntypedCollection : VrtBase, LBMigratable {
  UntypedCollection() = default;

protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    VrtBase::serialize(s);
  }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_UNTYPED_H*/
