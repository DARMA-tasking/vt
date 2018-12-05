
#if !defined INCLUDED_VRT_COLLECTION_TYPES_BASE_H
#define INCLUDED_VRT_COLLECTION_TYPES_BASE_H

#include "vt/config.h"
#include "vt/vrt/vrt_common.h"
#include "vt/vrt/collection/proxy_builder/elm_proxy_builder.h"
#include "vt/vrt/collection/types/base.fwd.h"
#include "vt/vrt/collection/types/insertable.h"
#include "vt/vrt/collection/types/indexable.h"
#include "vt/vrt/collection/types/untyped.h"
#include "vt/vrt/collection/manager.fwd.h"
#include "vt/vrt/proxy/collection_proxy.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct CollectionBase : Indexable<ColT, IndexT> {
  using ProxyType = VirtualElmProxyType<ColT, IndexT>;
  using CollectionProxyType = CollectionProxy<ColT, IndexT>;
  using IndexType = IndexT;

  CollectionBase() = default;
  CollectionBase(
    bool const static_size, bool const elms_fixed,
    VirtualElmCountType const num = -1
  );

  virtual ~CollectionBase();

  ProxyType getElementProxy(IndexT const& idx) const;
  CollectionProxyType getCollectionProxy() const;

  bool isStatic() const;

  static bool isStaticSized();

  void setSize(VirtualElmCountType const& elms);

  // Should be implemented in derived class (non-virtual)
  VirtualElmCountType getSize() const;

  virtual void migrate(NodeType const& node) override;

  template <typename Serializer>
  void serialize(Serializer& s);

  friend struct CollectionManager;

protected:
  VirtualElmCountType numElems_ = no_elms;
  EpochType cur_bcast_epoch_ = 0;
  bool hasStaticSize_ = true;
  bool elmsFixedAtCreation_ = true;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_TYPES_BASE_H*/
