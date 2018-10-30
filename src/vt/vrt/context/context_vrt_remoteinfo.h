
#if !defined INCLUDED_VRT_CONTEXT_CONTEXT_VRT_REMOTEINFO_H
#define INCLUDED_VRT_CONTEXT_CONTEXT_VRT_REMOTEINFO_H

#include "config.h"
#include "vrt/context/context_vrt_fwd.h"
#include "vrt/context/context_vrtproxy.h"

namespace vt { namespace vrt {

struct RemoteVrtInfo {
  bool isImmediate = false;
  VirtualProxyType proxy = no_vrt_proxy;
  VirtualRequestIDType req_id = no_request_id;
  NodeType from_node = uninitialized_destination;

  RemoteVrtInfo() = default;

  RemoteVrtInfo(NodeType const& node, VirtualRequestIDType const& in_req_id)
    : req_id(in_req_id), from_node(node)
  { }

  RemoteVrtInfo(NodeType const& node, VirtualProxyType p)
    : isImmediate(true), proxy(p), from_node(node)
  { }

  template <typename SerializerT>
  void serialize(SerializerT& s) {
    s | isImmediate;
    s | proxy;
    s | req_id;
    s | from_node;
  }
};

}} /* end namespace vt::vrt */

#endif /*INCLUDED_VRT_CONTEXT/CONTEXT_VRT_REMOTEINFO_H*/
