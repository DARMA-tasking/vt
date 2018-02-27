
#if !defined INCLUDED_VRT_COLLECTION_TYPES_TYPE_ATTORNEY_H
#define INCLUDED_VRT_COLLECTION_TYPES_TYPE_ATTORNEY_H

#include "config.h"
#include "vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

struct CollectionTypeAttorney {
  friend struct CollectionManager;

private:
  template <typename CollectionT>
  static void setSize(
    CollectionT const& collection, VirtualElmCountType const& elms
  );
  template <typename CollectionT, typename IndexT>
  static void setIndex(
    CollectionT const& collection, IndexT&& index
  );
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/types/type_attorney.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_TYPES_TYPE_ATTORNEY_H*/
