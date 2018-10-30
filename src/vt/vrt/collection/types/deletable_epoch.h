
#if !defined INCLUDED_VRT_COLLECTION_TYPES_DELETABLE_EPOCH_H
#define INCLUDED_VRT_COLLECTION_TYPES_DELETABLE_EPOCH_H

#include "config.h"
#include "vrt/collection/types/deletable.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct DeletableEpoch :
  Deletable<IndexT>
{
  DeletableEpoch() : Deletable<IndexT>() { }

protected:
  EpochType curEpoch_ = no_epoch;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_DELETABLE_EPOCH_H*/
