
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
  explicit Indexable(IndexT&& in_index)
    : Migratable<ColT>(), index_(std::move(in_index))
  { }

  Indexable() = default;

  IndexT const& getIndex() const { return index_; }

protected:
  template <typename Serializer>
  void serialize(Serializer& s) {
    Migratable<ColT>::serialize(s);
    s | index_;
  }

private:
  friend struct CollectionTypeAttorney;

  void setIndex(IndexT const& in_index) { index_ = in_index; }

private:
  IndexT index_;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_INDEXABLE_H*/
