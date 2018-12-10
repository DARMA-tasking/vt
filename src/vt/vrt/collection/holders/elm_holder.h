
#if !defined INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_H
#define INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/collection/types/headers.h"

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

  EpochType getBcastEpoch() const { return vc_ptr_->cur_bcast_epoch_; }
  void setBcastEpoch(EpochType const& cur) { vc_ptr_->cur_bcast_epoch_ = cur; }
  typename VirtualPtrType::pointer getCollection() const;

  bool erased_ = false;
  VirtualPtrType vc_ptr_ = nullptr;
  HandlerType map_fn = uninitialized_handler;
  IndexT max_idx;
};

}}} /* end namespace vt::vrt::collection */

#include "vt/vrt/collection/holders/elm_holder.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_HOLDERS_ELM_HOLDER_H*/
