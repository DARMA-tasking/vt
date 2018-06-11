
#if !defined INCLUDED_VRT_PROXY_COLLECTION_WRAPPER_H
#define INCLUDED_VRT_PROXY_COLLECTION_WRAPPER_H

#include "config.h"
#include "vrt/collection/send/sendable.h"
#include "vrt/collection/destroy/destroyable.h"
#include "vrt/collection/broadcast/broadcastable.h"
#include "vrt/proxy/proxy_element.h"
#include "vrt/proxy/proxy_collection.h"
#include "vrt/proxy/base_wrapper.h"

namespace vt { namespace vrt { namespace collection {

/*
 * `CollectionProxy': variant w/o IndexT baked into class. Thus, all accesses
 * require that IndexT be determinable (so the index cannot be created on the
 * fly).
 */

struct CollectionProxy {
  template <typename ColT, typename IndexT>
  using ElmProxyType = VrtElmProxy<ColT, IndexT>;

  CollectionProxy() = default;
  CollectionProxy(VirtualProxyType const in_proxy);

  VirtualProxyType getProxy() const;

  template <typename ColT, typename IndexT>
  ElmProxyType<ColT, IndexT> index(IndexT const& idx) const;
  template <typename ColT, typename IndexT>
  ElmProxyType<ColT, IndexT> operator[](IndexT const& idx) const;
  template <typename ColT, typename IndexT>
  ElmProxyType<ColT, IndexT> operator()(IndexT const& idx) const;

private:
  VirtualProxyType const proxy_ = no_vrt_proxy;
};

/*
 * `CollectionIndexProxy' (variant with IndexT baked into class, allowing
 * constructors to be forwarded for building indicies in line without the type.
 */

template <typename ColT, typename IndexT>
struct CollectionIndexProxy : Broadcastable<ColT, IndexT> {
  using ElmProxyType = VrtElmProxy<ColT, IndexT>;

  CollectionIndexProxy() = default;
  CollectionIndexProxy(CollectionIndexProxy const&) = default;
  CollectionIndexProxy(VirtualProxyType const in_proxy);

  CollectionIndexProxy& operator=(CollectionIndexProxy const&) = default;

  template <typename... IndexArgsT>
  ElmProxyType index_build(IndexArgsT&&... idx) const;
  template <typename... IndexArgsT>
  ElmProxyType operator[](IndexArgsT&&... idx) const;
  template <typename... IndexArgsT>
  ElmProxyType operator()(IndexArgsT&&... idx) const;

  ElmProxyType index(IndexT const& idx) const;
  ElmProxyType operator[](IndexT const& idx) const;
  ElmProxyType operator()(IndexT const& idx) const;
};


}}} /* end namespace vt::vrt::collection */

#include "vrt/proxy/collection_wrapper.impl.h"

#endif /*INCLUDED_VRT_PROXY_COLLECTION_WRAPPER_H*/
