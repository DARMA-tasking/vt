
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_INFO_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_INFO_H

#include "config.h"
#include "vrt/context/context_vrt_fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename IndexT>
struct CollectionInfo {
  bool isImmediate = false;
  VirtualProxyType proxy = no_vrt_proxy;
  VirtualRequestIDType req_id = no_request_id;
  NodeType from_node = uninitialized_destination;
  IndexT range;

  CollectionInfo() = default;
  CollectionInfo(CollectionInfo const&) = default;
  CollectionInfo(
    IndexT const& in_range, bool const immediate, NodeType const& node,
    VirtualProxyType p
  ) : range(in_range), isImmediate(immediate), proxy(p), from_node(node)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | isImmediate;
    s | proxy;
    s | req_id;
    s | from_node;
    s | range;
  }
};


}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_INFO_H*/
