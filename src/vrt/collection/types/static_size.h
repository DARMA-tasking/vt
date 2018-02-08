
#if !defined INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_H
#define INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_H

#include "config.h"
#include "vrt/collection/types/base.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct StaticCollectionBase :
  CollectionBase<IndexT>
{
  explicit StaticCollectionBase(VirtualElmCountType const inNumElems)
    : CollectionBase<IndexT>(false, false), numElems_(inNumElems)
  { }

  VirtualElmCountType getSize() const { return numElems_; }

  static bool isStaticSized() { return true; }

protected:
  VirtualElmCountType numElems_ = no_elms;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_STATIC_SIZE_H*/
