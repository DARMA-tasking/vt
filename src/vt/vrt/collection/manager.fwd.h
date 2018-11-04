
#if !defined INCLUDED_VRT_COLLECTION_MANAGER_FWD_H
#define INCLUDED_VRT_COLLECTION_MANAGER_FWD_H

#include "vt/config.h"

namespace vt { namespace vrt { namespace collection {

struct CollectionManager;

}}} /* end namespace vt::vrt::collection */

namespace vt {

extern vrt::collection::CollectionManager* theCollection();

}  // end namespace vt

#endif /*INCLUDED_VRT_COLLECTION_MANAGER_FWD_H*/
