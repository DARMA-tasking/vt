
#if !defined INCLUDED_VRT_COLLECTION_TYPES_INDEXABLE_H
#define INCLUDED_VRT_COLLECTION_TYPES_INDEXABLE_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/types/type_attorney.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct Indexable {
  explicit Indexable(IndexT&& in_index)
    : index_(std::move(in_index))
  { }

  Indexable() = default;

  IndexT getIndex() const { return index_; }

private:
  friend struct CollectionTypeAttorney;

  void setIndex(IndexT&& in_index) { index_ = std::move(in_index); }

private:
  IndexT index_;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_INDEXABLE_H*/
