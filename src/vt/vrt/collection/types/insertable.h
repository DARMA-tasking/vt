
#if !defined INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_H
#define INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct Insertable {
  Insertable() = default;

  void insert(
    IndexT const& idx, NodeType const& node = uninitialized_destination
  );
  void beginInserting();
  void finishInserting();

protected:
  bool doneInserting = false;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_INSERTABLE_H*/
