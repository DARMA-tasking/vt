
#if !defined INCLUDED_VRT_COLLECTION_INSERTABLE_H
#define INCLUDED_VRT_COLLECTION_INSERTABLE_H

#include "config.h"
#include "vrt/vrt_common.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Insertable {
  void insert(IndexT const& idx);
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_INSERTABLE_H*/
