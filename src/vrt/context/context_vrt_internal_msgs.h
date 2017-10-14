
#if !defined __RUNTIME_TRANSPORT_CONTEXT_VRT_INTERNAL_MSGS__
#define __RUNTIME_TRANSPORT_CONTEXT_VRT_INTERNAL_MSGS__

#include "config.h"
#include "context_vrtproxy.h"
#include "context_vrt_fwd.h"
#include "messaging/message.h"
#include "serialization/serialization.h"

#include <array>

namespace vt { namespace vrt {

template <typename Tuple>
struct VirtualConstructDataMsg : ShortMessage {
  Tuple* tup = nullptr;
  VirtualConstructDataMsg(Tuple* in_tup) : ShortMessage(), tup(in_tup) { }
};

struct VirtualProxyRequestMsg : ShortMessage {
  NodeType request_node = uninitialized_destination;
  NodeType construct_node = uninitialized_destination;
  VirtualRequestIDType request_id = no_request_id;
  VirtualProxyType proxy = no_vrt_proxy;

  VirtualProxyRequestMsg(
    NodeType const& in_node, NodeType const& in_req_node,
    VirtualRequestIDType const& in_request_id,
    VirtualProxyType const& in_proxy = no_vrt_proxy
  )
    : ShortMessage(), request_node(in_req_node), construct_node(in_node),
      request_id(in_request_id), proxy(in_proxy)
  { }
};

}} /* end namespace vt::vrt */

#endif /*__RUNTIME_TRANSPORT_CONTEXT_VRT_INTERNAL_MSGS__*/
