
#if !defined INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_IMPL_H

#include "config.h"
#include "vrt/collection/types/insertable.h"
#include "vrt/collection/manager.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
void Insertable<ColT,IndexT>::insert(IndexT const& idx, NodeType const& node) {
  theCollection()->insert<ColT>(idx, node);
}

template <typename ColT, typename IndexT>
void Insertable<ColT,IndexT>::beginInserting() {
    assert(doneInserting == false and "Must not be done inserting");
}

template <typename ColT, typename IndexT>
void Insertable<ColT,IndexT>::finishInserting() {
  assert(doneInserting == false and "Must not be done inserting");
  doneInserting = true;
  // barrier, make sure that size is consistent
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_IMPL_H*/
