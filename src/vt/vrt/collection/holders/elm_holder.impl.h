
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_IMPL_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_IMPL_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/holders/elm_holder.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/types/headers.h"

#include <memory>
#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
ElementHolder<ColT, IndexT>::ElementHolder(
  VirtualPtrType in_vc_ptr_, HandlerType const& in_han, IndexT const& idx
) : vc_ptr_(std::move(in_vc_ptr_)), map_fn(in_han), max_idx(idx)
{ }

template <typename ColT, typename IndexT>
/*virtual*/ ElementHolder<ColT, IndexT>::~ElementHolder() {
  vc_ptr_ = nullptr;
}

template <typename ColT, typename IndexT>
typename ElementHolder<ColT, IndexT>::VirtualPtrType::pointer
ElementHolder<ColT, IndexT>::getCollection() const {
  vtAssert(vc_ptr_ != nullptr, "Must be valid pointer");
  return vc_ptr_.get();
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_IMPL_H*/
