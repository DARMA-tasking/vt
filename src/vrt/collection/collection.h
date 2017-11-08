
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/collection_elm_proxy.h"
#include "vrt/collection/collection_base.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Collection : StaticCollectionBase<IndexT> {
  explicit Collection(VirtualElmCountType const elms)
    : StaticCollectionBase<IndexT>(elms)
  { }
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_H*/
