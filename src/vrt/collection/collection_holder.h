
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_HOLDER_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_HOLDER_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/collection_elm_proxy.h"
#include "vrt/collection/collection.h"

#include <unordered_map>

namespace vt { namespace vrt { namespace collection {

// fwd decl for friendship
struct CollectionManager;

template <typename IndexT>
struct CollectionHolder {
  using CollectionType = Collection<IndexT>;
  using VirtualPtrType = std::unique_ptr<CollectionType>;
  using TypedIndexContainer = std::unordered_map<IndexT, VirtualPtrType>;
  using TypedProxyContainer = std::unordered_map<VirtualProxyType, TypedIndexContainer>;
  using UntypedIndexContainer = std::unordered_map<UniqueIndexBitType, VirtualPtrType>;
  using UntypedProxyContainer = std::unordered_map<VirtualProxyType, UntypedIndexContainer>;

  friend struct CollectionManager;

private:
  static UntypedProxyContainer vc_container_;
};

template <typename IndexT>
typename CollectionHolder<IndexT>::UntypedProxyContainer
  CollectionHolder<IndexT>::vc_container_;


}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_HOLDER_H*/
