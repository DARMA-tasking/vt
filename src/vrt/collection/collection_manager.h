
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/collection_elm_proxy.h"
#include "vrt/collection/collection.h"

namespace vt { namespace vrt { namespace collection {

struct CollectionManager {
  CollectionManager() = default;
};

}}} /* end namespace vt::vrt::collection */

namespace vt {

extern vrt::collection::CollectionManager* theCollection();

}  // end namespace vt

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_MANAGER_H*/
