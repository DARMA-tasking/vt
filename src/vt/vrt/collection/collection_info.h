
#if !defined INCLUDED_VRT_COLLECTION_COLLECTION_INFO_H
#define INCLUDED_VRT_COLLECTION_COLLECTION_INFO_H

#include "config.h"
#include "vrt/collection/manager.fwd.h"
#include "vrt/context/context_vrt_fwd.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct CollectionInfo {
  CollectionInfo() = default;
  CollectionInfo(CollectionInfo const&) = default;
  CollectionInfo(
    IndexT const& in_range, bool const in_immediate,
    NodeType const& in_from_node, VirtualProxyType in_proxy
  ) : range_(in_range), immediate_(in_immediate), proxy_(in_proxy),
      from_node_(in_from_node)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | immediate_ | proxy_ | req_id_ | from_node_ | range_ | insert_epoch_;
  }

  VirtualProxyType getProxy() const { return proxy_; }
  IndexT getRange() const { return range_; }

  void setInsertEpoch(EpochType const& in_epoch) { insert_epoch_ = in_epoch; }
  EpochType getInsertEpoch() const { return insert_epoch_; }

  friend struct CollectionManager;

private:
  bool immediate_ = false;
  VirtualProxyType proxy_ = no_vrt_proxy;
  VirtualRequestIDType req_id_ = no_request_id;
  NodeType from_node_ = uninitialized_destination;
  IndexT range_;
  EpochType insert_epoch_ = no_epoch;
};


}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_COLLECTION_INFO_H*/
