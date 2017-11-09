
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_BASE_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_BASE_H

#include "config.h"
#include "vrt/vrt_common.h"
#include "vrt/collection/collection_elm_proxy.h"
#include "vrt/collection/insertable.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct CollectionBase : VrtBase {
  using ProxyType = VirtualElmProxyType;

  CollectionBase() = default;
  CollectionBase(
    bool const inHasStaticSize, bool const inElmsFixedAtCreation_ = true
  ) : hasStaticSize_(inHasStaticSize),
      elmsFixedAtCreation_(inElmsFixedAtCreation_)
  { }

  ProxyType getElementProxy(IndexT const& idx) const {
    VirtualElmOnlyProxyType elmProxy;
    VirtualElemProxyBuilder::createElmProxy(elmProxy, idx.uniqueBits());
    ProxyType proxy(getProxy(), elmProxy);
    return proxy;
  }

  bool isStatic() const { return hasStaticSize_ and elmsFixedAtCreation_; }

  static bool isStaticSized() { return true; }

  // Should be implemented in derived class (non-virtual)
  VirtualElmCountType getSize() const;

protected:
  bool hasStaticSize_ = true;
  bool elmsFixedAtCreation_ = true;
};


template <typename IndexT>
struct StaticCollectionBase : CollectionBase<IndexT> {
  StaticCollectionBase(VirtualElmCountType const inNumElems)
    : CollectionBase<IndexT>(false, false), numElems_(inNumElems)
  { }

  VirtualElmCountType getSize() const { return numElems_; }

  static bool isStaticSized() { return true; }

protected:
  VirtualElmCountType numElems_ = no_elms;
};

template <typename IndexT>
struct StaticInsertableCollectionBase :
    StaticCollectionBase<IndexT>, Insertable<IndexT>
{
  StaticInsertableCollectionBase(VirtualElmCountType const inNumElems)
    : StaticCollectionBase<IndexT>(inNumElems)
  {
    CollectionBase<IndexT>::elmsFixedAtCreation_ = false;
  }

  static bool isStaticSized() { return false; }
};

template <typename IndexT>
struct DynamicCollectionBase : CollectionBase<IndexT>, InsertableEpoch<IndexT> {
  DynamicCollectionBase() : CollectionBase<IndexT>(false, false) { }

  // Unknown so return no_elms as the size
  VirtualElmCountType getSize() const { return no_elms; }

  static bool isStaticSized() { return false; }
};


}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_BASE_H*/
