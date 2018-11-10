
#if !defined INCLUDED_VRT_CONTEXT_CONTEXT_VRT_INTERNAL_MSGS_H
#define INCLUDED_VRT_CONTEXT_CONTEXT_VRT_INTERNAL_MSGS_H

#include "vt/config.h"
#include "vt/vrt/proxy/proxy_bits.h"
#include "vt/vrt/context/context_vrt_fwd.h"
#include "vt/messaging/message.h"
#include "vt/serialization/serialization.h"

#include <array>

namespace vt { namespace vrt {

template <typename RemoteInfo, typename ArgsTuple, typename VirtualContextT>
struct VrtConstructMsg : Message {
  using VirtualContextType = VirtualContextT;
  using ArgsTupleType = ArgsTuple;

  RemoteInfo info;
  ArgsTuple tup;

  VrtConstructMsg() = default;

  VrtConstructMsg(ArgsTuple&& in_tup)
    : Message(), tup(std::forward<ArgsTuple>(in_tup))
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | info | tup;
  }
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

#endif /*INCLUDED_VRT_CONTEXT/CONTEXT_VRT_INTERNAL_MSGS_H*/
