
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/manager.fwd.h"
#include "vrt/collection/types/headers.h"

#include <memory>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct ElementHolder {
  using VirtualPtrType = std::unique_ptr<CollectionBase<ColT,IndexT>>;

  ElementHolder(
    VirtualPtrType in_vc_ptr_, HandlerType const& in_han, IndexT const& idx
  );
  ElementHolder(ElementHolder&&) = default;

  virtual ~ElementHolder();

  typename VirtualPtrType::pointer getCollection() const;

  bool erased_ = false;
  VirtualPtrType vc_ptr_ = nullptr;
  HandlerType map_fn = uninitialized_handler;
  IndexT max_idx;
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/holders/elm_holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_H*/
