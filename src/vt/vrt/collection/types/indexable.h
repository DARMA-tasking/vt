
#if !defined INCLUDED_VRT_COLLECTION_TYPES_INDEXABLE_H
#define INCLUDED_VRT_COLLECTION_TYPES_INDEXABLE_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/types/type_attorney.h"
#include "vt/vrt/collection/types/migrate_hooks.h"
#include "vt/vrt/collection/types/migratable.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct Indexable : Migratable<ColT> {

  explicit Indexable(IndexT&& in_index);
  Indexable() = default;

  IndexT const& getIndex() const;

protected:
  template <typename Serializer>
  void serialize(Serializer& s);

private:
  friend struct CollectionTypeAttorney;

  void setIndex(IndexT const& in_index);

private:
  // The index stored with the collection element
  IndexT index_;
  // This field stores whether the `index_` has been properly set: if the
  // constructor overload has no index, it will not be set until the its set
  // through the `CollectionTypeAttorney`
  bool set_index_ = false;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_INDEXABLE_H*/
