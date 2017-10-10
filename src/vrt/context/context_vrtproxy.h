
#if !defined INCLUDED_CONTEXT_VRT_PROXY
#define INCLUDED_CONTEXT_VRT_PROXY

#include "config.h"
#include "context_vrt_fwd.h"
#include "context_vrt.h"

namespace vt { namespace vrt {

static constexpr BitCountType const virtual_is_collection_num_bits = 1;
static constexpr BitCountType const virtual_is_migratable_num_bits = 1;
static constexpr BitCountType const virtual_node_num_bits =
    BitCounterType<NodeType>::value;
static constexpr BitCountType const virtual_id_num_bits =
    BitCounterType<VirtualIDType>::value;

enum eVirtualProxyBits {
  Collection = 0,
  Migratable = eVirtualProxyBits::Collection + virtual_is_collection_num_bits,
  Node       = eVirtualProxyBits::Migratable + virtual_is_migratable_num_bits,
  ID         = eVirtualProxyBits::Node + virtual_node_num_bits
};

struct VirtualProxyBuilder {
  static VirtualProxyType createProxy(
    VirtualIDType const& id, NodeType const& node,
    bool const& is_coll = false, bool const& is_migratable = false
  );

  static void setIsCollection(VirtualProxyType& proxy, bool const& is_coll);
  static void setIsMigratable(VirtualProxyType& proxy, bool const& is_migratable);
  static void setVirtualNode(VirtualProxyType& proxy, NodeType const& node);
  static void setVirtualID(VirtualProxyType& proxy, VirtualIDType const& id);
  static bool isCollection(VirtualProxyType const& proxy);
  static bool isMigratable(VirtualProxyType const& proxy);
  static NodeType getVirtualNode(VirtualProxyType const& proxy);
  static VirtualIDType getVirtualID(VirtualProxyType const& proxy);
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT_PROXY*/
