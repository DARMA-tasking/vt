
#if !defined INCLUDED_VRT_COLLECTION_DESTROY_DESTROY_MSG_H
#define INCLUDED_VRT_COLLECTION_DESTROY_DESTROY_MSG_H

#include "config.h"
#include "messaging/message.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct DestroyMsg final : ::vt::Message {
  DestroyMsg() = default;
  DestroyMsg(
    VirtualProxyType const& in_proxy, NodeType const& in_from
  ) : proxy_(in_proxy), from_(in_from)
  { }

  VirtualProxyType getProxy() const { return proxy_; }
  NodeType getFromNode() const { return from_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | proxy_ | from_;
  }

private:
  VirtualProxyType proxy_;
  NodeType from_ = uninitialized_destination;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_DESTROY_DESTROY_MSG_H*/
