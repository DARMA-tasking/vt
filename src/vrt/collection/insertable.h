
#if !defined INCLUDED_VRT_COLLECTION_INSERTABLE_H
#define INCLUDED_VRT_COLLECTION_INSERTABLE_H

#include "config.h"
#include "vrt/vrt_common.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Insertable {
  void insert(IndexT const& idx);

  void beginInserting() {
    assert(doneInserting == false and "Must not be done inserting");
  }
  void finishInserting() {
    assert(doneInserting == false and "Must not be done inserting");
    doneInserting = true;

    // barrier, make sure that size is consistent
  }

protected:
  bool doneInserting = false;
};

/*
 * Can be inserted during a specific epoch which is a dynamic (partially
 * collective) property over time
 */

template <typename IndexT>
struct InsertableEpoch : Insertable<IndexT> {
protected:
  EpochType curEpoch_ = no_epoch;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_INSERTABLE_H*/
