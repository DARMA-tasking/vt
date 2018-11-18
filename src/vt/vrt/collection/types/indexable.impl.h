
#if !defined INCLUDED_VT_VRT_COLLECTION_TYPES_INDEXABLE_IMPL_H
#define INCLUDED_VT_VRT_COLLECTION_TYPES_INDEXABLE_IMPL_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/types/type_attorney.h"
#include "vt/vrt/collection/types/migrate_hooks.h"
#include "vt/vrt/collection/types/migratable.h"
#include "vt/vrt/collection/types/indexable.h"
#include "vt/vrt/collection/manager.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
Indexable<ColT,IndexT>::Indexable(IndexT&& in_index)
  : Migratable<ColT>(), index_(std::move(in_index)), set_index_(true)
{ }


template <typename ColT, typename IndexT>
IndexT const& Indexable<ColT,IndexT>::getIndex() const {
  if (!set_index_) {
    auto ctx_idx = theCollection()->queryIndexContext<IndexT>();
    vtAssertExpr(ctx_idx != nullptr);
    return *ctx_idx;
  } else {
    return index_;
  }
}

template <typename ColT, typename IndexT>
template <typename SerializerT>
void Indexable<ColT,IndexT>::serialize(SerializerT& s) {
  Migratable<ColT>::serialize(s);
  s | set_index_;
  s | index_;
}

template <typename ColT, typename IndexT>
void Indexable<ColT,IndexT>::setIndex(IndexT const& in_index) {
  // Set the field and then indicate that the `index_` field is now valid with
  // `set_index_`
  index_ = in_index;
  set_index_ = true;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VT_VRT_COLLECTION_TYPES_INDEXABLE_IMPL_H*/
