
#if !defined INCLUDED_VRT_PROXY_COLLECTION_WRAPPER_H
#define INCLUDED_VRT_PROXY_COLLECTION_WRAPPER_H

#include "config.h"
#include "vrt/proxy/proxy_element.h"
#include "vrt/proxy/proxy_collection.h"

namespace vt { namespace vrt { namespace collection {

/*
 * `CollectionProxy': variant w/o IndexT baked into class. Thus, all accesses
 * require that IndexT be determinable (so the index cannot be created on the
 * fly).
 */

struct CollectionProxy {
  template <typename IndexT>
  using ElmProxyType = VrtElmProxy<IndexT>;

  CollectionProxy() = default;
  CollectionProxy(VirtualProxyType const in_proxy);

  VirtualProxyType getProxy() const;

  template <typename IndexT>
  ElmProxyType<IndexT> index(IndexT const& idx);
  template <typename IndexT>
  ElmProxyType<IndexT> operator[](IndexT const& idx);
  template <typename IndexT>
  ElmProxyType<IndexT> operator()(IndexT const& idx);

private:
  VirtualProxyType const proxy_ = no_vrt_proxy;
};

/*
 * `CollectionIndexProxy' (variant with IndexT baked into class, allowing
 * constructors to be forwarded for building indicies in line without the type.
 */

template <typename IndexT>
struct CollectionIndexProxy {
  using ElmProxyType = VrtElmProxy<IndexT>;

  CollectionIndexProxy() = default;
  CollectionIndexProxy(VirtualProxyType const in_proxy);

  VirtualProxyType getProxy() const;

  template <typename... IndexArgsT>
  ElmProxyType index_build(IndexArgsT&&... idx);
  template <typename... IndexArgsT>
  ElmProxyType operator[](IndexArgsT&&... idx);
  template <typename... IndexArgsT>
  ElmProxyType operator()(IndexArgsT&&... idx);

  ElmProxyType index(IndexT const& idx);
  ElmProxyType operator[](IndexT const& idx);
  ElmProxyType operator()(IndexT const& idx);

private:
  VirtualProxyType const proxy_ = no_vrt_proxy;
};


}}} /* end namespace vt::vrt::collection */

#include "vrt/proxy/collection_wrapper.impl.h"

#endif /*INCLUDED_VRT_PROXY_COLLECTION_WRAPPER_H*/
