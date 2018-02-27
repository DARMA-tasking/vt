
#if !defined INCLUDED_VRT_COLLECTION_TYPES_BASE_H
#define INCLUDED_VRT_COLLECTION_TYPES_BASE_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vrt/collection/types/insertable.h"
#include "vrt/collection/types/migrate_hooks.h"
#include "vrt/collection/types/untyped.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct CollectionBase : UntypedCollection, MigrateHookInterface {
  using ProxyType = VirtualElmProxyType<IndexT>;
  using IndexType = IndexT;

  CollectionBase() = default;
  CollectionBase(bool const static_size, bool const elms_fixed);

  ProxyType getElementProxy(IndexT const& idx) const;

  bool isStatic() const;

  static bool isStaticSized();

  // Should be implemented in derived class (non-virtual)
  VirtualElmCountType getSize() const;

protected:
  bool hasStaticSize_ = true;
  bool elmsFixedAtCreation_ = true;
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/types/base.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_TYPES_BASE_H*/
