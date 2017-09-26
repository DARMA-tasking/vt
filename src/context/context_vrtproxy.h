
#if !defined INCLUDED_CONTEXT_VRT_PROXY
#define INCLUDED_CONTEXT_VRT_PROXY

#include "context_vrt.h"

namespace vt { namespace vrt {

static constexpr BitCountType const vrtCntx_collection_num_bits = 1;
static constexpr BitCountType const vrtCntx_migratable_num_bits = 1;
static constexpr BitCountType const vrtCntx_node_num_bits =
    BitCounterType<NodeType>::value;
static constexpr BitCountType const vrtCntx_id_num_bits =
    BitCounterType<VrtContext_IdType>::value;

enum eVrtContextProxyBits {
  Collection = 0,
  Migratable = eVrtContextProxyBits::Collection + vrtCntx_collection_num_bits,
  Node       = eVrtContextProxyBits::Migratable + vrtCntx_migratable_num_bits,
  ID         = eVrtContextProxyBits::Node + vrtCntx_node_num_bits
};

struct VrtContextProxy {

  static VrtContext_ProxyType createNewProxy(
      VrtContext_IdType const& id, NodeType const& node,
      bool const& is_coll = false, bool const& is_migratable = false);

  static void setIsCollection(
      VrtContext_ProxyType& proxy, bool const& is_coll);
  static void setIsMigratable(
      VrtContext_ProxyType& proxy, bool const& is_migratable);
  static void setVrtContextNode(
      VrtContext_ProxyType& proxy, NodeType const& node);
  static void setVrtContextId(
      VrtContext_ProxyType& proxy, VrtContext_IdType const& id);

  static bool isCollection(VrtContext_ProxyType const& proxy);
  static bool isMigratable(VrtContext_ProxyType const& proxy);
  static NodeType getVrtContextNode(VrtContext_ProxyType const& proxy);
  static VrtContext_IdType getVrtContextId(VrtContext_ProxyType const& proxy);
};

}}  // end namespace vt::vrt

#endif  /*INCLUDED_CONTEXT_VRT_PROXY*/
