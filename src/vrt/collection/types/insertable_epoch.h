
#if !defined INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_EPOCH_H
#define INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_EPOCH_H

#include "config.h"
#include "vrt/collection/types/insertable.h"

namespace vt { namespace vrt { namespace collection {

/*
 * Can be inserted during a specific epoch which is a dynamic (partially
 * collective) property over time
 */

template <typename IndexT>
struct InsertableEpoch :
  Insertable<IndexT>
{
protected:
  EpochType curEpoch_ = no_epoch;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_EPOCH_H*/
