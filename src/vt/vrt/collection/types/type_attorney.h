
#if !defined INCLUDED_VRT_COLLECTION_TYPES_TYPE_ATTORNEY_H
#define INCLUDED_VRT_COLLECTION_TYPES_TYPE_ATTORNEY_H

#include "config.h"
#include "vrt/collection/manager.fwd.h"

namespace vt { namespace vrt { namespace collection {

struct CollectionTypeAttorney {
  friend struct CollectionManager;

private:
  template <typename ColT>
  static void setSize(ColT const& col, VirtualElmCountType const& elms);
  template <typename ColT, typename IndexT>
  static void setIndex(ColT const& col, IndexT const& idx);
  template <typename ColT>
  static void setProxy(ColT const& col, VirtualProxyType const& proxy);
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/types/type_attorney.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_TYPES_TYPE_ATTORNEY_H*/
