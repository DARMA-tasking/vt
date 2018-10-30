
#if !defined INCLUDED_VRT_COLLECTION_MAPPED_NODE_MAPPED_NODE_H
#define INCLUDED_VRT_COLLECTION_MAPPED_NODE_MAPPED_NODE_H

#include "config.h"
#include "vrt/proxy/base_collection_proxy.h"
#include "vrt/proxy/base_collection_elm_proxy.h"

#include <functional>

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT, typename BaseProxyT>
struct MappedNode : BaseProxyT {
  using FuncLocType = std::function<void(NodeType)>;

  MappedNode() = default;
  MappedNode(
    typename BaseProxyT::ProxyType const& in_proxy,
    typename BaseProxyT::ElementProxyType const& in_elm
  );

  void getMappedNode() const;
  void lookupLocation(FuncLocType fn);
};

}}} /* end namespace vt::vrt::collection */

#include "vrt/collection/mapped_node/mapped_node.impl.h"

#endif /*INCLUDED_VRT_COLLECTION_MAPPED_NODE_MAPPED_NODE_H*/
