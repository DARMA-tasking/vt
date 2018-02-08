
#if !defined INCLUDED_VRT_COLLECTION_TYPES_BASE_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_BASE_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/types/base.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
CollectionBase<IndexT>::CollectionBase(
  bool const static_size, bool const elms_fixed
) : UntypedCollection(),
    hasStaticSize_(static_size),
    elmsFixedAtCreation_(elms_fixed)
{ }

template <typename IndexT>
typename CollectionBase<IndexT>::ProxyType
CollectionBase<IndexT>::getElementProxy(IndexT const& idx) const {
  VirtualElmOnlyProxyType elmProxy;
  VirtualElemProxyBuilder::createElmProxy(elmProxy, idx.uniqueBits());
  ProxyType proxy(getProxy(), elmProxy);
  return proxy;
}

template <typename IndexT>
bool CollectionBase<IndexT>::isStatic() const {
  return hasStaticSize_ && elmsFixedAtCreation_;
}

template <typename IndexT>
/*static*/ bool CollectionBase<IndexT>::isStaticSized() {
  return true;
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_BASE_IMPL_H*/
