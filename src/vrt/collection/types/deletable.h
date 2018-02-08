
#if !defined INCLUDED_VRT_COLLECTION_TYPES_DELETABLE_H
#define INCLUDED_VRT_COLLECTION_TYPES_DELETABLE_H

#include "config.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Deletable {
  void deleteElement(IndexT const& idx);
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_DELETABLE_H*/
