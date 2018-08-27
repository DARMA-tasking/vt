
#if !defined INCLUDED_VRT_COLLECTION_MAPPED_NODE_MAPPED_NODE_IMPL_H
#define INCLUDED_VRT_COLLECTION_MAPPED_NODE_MAPPED_NODE_IMPL_H

#include "config.h"
#include "vrt/proxy/base_collection_proxy.h"
#include "vrt/proxy/base_collection_elm_proxy.h"
#include "vrt/collection/mapped_node/mapped_node.impl.h"

#include <functional>
#include <cassert>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
void MappedNode<ColT,IndexT,BaseProxyT>::getMappedNode() const {
  auto const& idx = getIndex();
  auto col_proxy = this->getCollectionProxy();
  // auto elm_proxy = this->getElementProxy();
  // auto proxy = VrtElmProxy<ColT, IndexT>(col_proxy,elm_proxy);
  theCollection()->getMappedNode(col_proxy,idx);
}

template <typename ColT, typename IndexT, typename BaseProxyT>
void MappedNode<ColT,IndexT,BaseProxyT>::lookupLocation(FuncLocType fn) {
  vtAssert(0, "Not implemented yet");
}

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MAPPED_NODE_MAPPED_NODE_IMPL_H*/
