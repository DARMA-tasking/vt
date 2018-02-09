
#if !defined INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_H
#define INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_H

#include "config.h"
#include "vrt/vrt_common.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Insertable {
  Insertable() = default;

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

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_H*/
