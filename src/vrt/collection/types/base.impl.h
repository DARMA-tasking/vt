
#if !defined INCLUDED_VRT_COLLECTION_TYPES_BASE_IMPL_H
#define INCLUDED_VRT_COLLECTION_TYPES_BASE_IMPL_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/types/base.h"
#include "vrt/collection/migrate/manager_migrate_attorney.h"
#include "vrt/collection/migrate/migrate_status.h"

#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
CollectionBase<ColT, IndexT>::CollectionBase(
  bool const static_size, bool const elms_fixed
) : Indexable<ColT, IndexT>(),
    hasStaticSize_(static_size),
    elmsFixedAtCreation_(elms_fixed)
{ }

template <typename ColT, typename IndexT>
typename CollectionBase<ColT, IndexT>::ProxyType
CollectionBase<ColT, IndexT>::getElementProxy(IndexT const& idx) const {
  VirtualElmOnlyProxyType elmProxy;
  VirtualElemProxyBuilder::createElmProxy(elmProxy, idx.uniqueBits());
  ProxyType proxy(this->getProxy(), elmProxy);
  return proxy;
}

template <typename ColT, typename IndexT>
typename CollectionBase<ColT, IndexT>::CollectionProxyType
CollectionBase<ColT, IndexT>::getCollectionProxy() const {
  auto const& proxy = this->getProxy();
  typename CollectionBase<ColT, IndexT>::CollectionProxyType col_proxy(proxy);
  return col_proxy;
}

template <typename ColT, typename IndexT>
bool CollectionBase<ColT, IndexT>::isStatic() const {
  return hasStaticSize_ && elmsFixedAtCreation_;
}

template <typename ColT, typename IndexT>
/*static*/ bool CollectionBase<ColT, IndexT>::isStaticSized() {
  return true;
}

template <typename ColT, typename IndexT>
/*virtual*/ void CollectionBase<ColT, IndexT>::migrate(NodeType const& node) {
  auto const& collection_proxy = this->getProxy();
  auto const& collection_index = this->getIndex();
  auto const& migrate_status = CollectionElmAttorney<ColT,IndexT>::migrateOut(
    collection_proxy, collection_index, node
  );
  assert(
    migrate_status == MigrateStatus::MigratedToRemote &&
    "Required be immediate, valid migration currently"
  );
}

template <typename ColT, typename IndexT>
template <typename Serializer>
void CollectionBase<ColT, IndexT>::serialize(Serializer& s) {
  Indexable<ColT, IndexT>::serialize(s);
  s | hasStaticSize_;
  s | elmsFixedAtCreation_;
}

template <typename ColT, typename IndexT>
/*virtual*/ CollectionBase<ColT, IndexT>::~CollectionBase() {}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_BASE_IMPL_H*/
