
#include "context_vrtproxy.h"

namespace vt {namespace vrt {

/*static*/ VrtContext_ProxyType VrtContextProxy::createNewProxy(
    VrtContext_IdType const& id, NodeType const& node,
    const bool& is_coll, const bool& is_migratable) {
  VrtContext_ProxyType myProxy;

  setIsCollection(myProxy, is_coll);
  setIsMigratable(myProxy, is_migratable);
  setVrtContextNode(myProxy, node);
  setVrtContextId(myProxy, id);

  return myProxy;
}

/*static*/ void VrtContextProxy::setIsCollection(
    VrtContext_ProxyType& proxy, bool const& is_coll) {
  BitPackerType::boolSetField<eVrtContextProxyBits::Collection>
      (proxy, is_coll);
}

/*static*/ void VrtContextProxy::setIsMigratable(
    VrtContext_ProxyType& proxy, bool const& is_migratable) {
  BitPackerType::boolSetField<eVrtContextProxyBits::Migratable>
      (proxy, is_migratable);
}

/*static*/ void VrtContextProxy::setVrtContextNode(
    VrtContext_ProxyType& proxy, NodeType const& node) {
  BitPackerType::setField<eVrtContextProxyBits::Node,
                          vrtCntx_node_num_bits>(proxy, node);
}

/*static*/ void VrtContextProxy::setVrtContextId(
    VrtContext_ProxyType& proxy, VrtContext_IdType const& id) {
  BitPackerType::setField<eVrtContextProxyBits::ID,
                          vrtCntx_id_num_bits>(proxy, id);
}

/*static*/ bool VrtContextProxy::isCollection(
    VrtContext_ProxyType const& proxy) {
  return BitPackerType::boolGetField<eVrtContextProxyBits::Collection>
      (proxy);
}

/*static*/ bool VrtContextProxy::isMigratable(
    VrtContext_ProxyType const& proxy) {
  return BitPackerType::boolGetField<eVrtContextProxyBits::Migratable>
      (proxy);
}

/*static*/ NodeType VrtContextProxy::getVrtContextNode(
    VrtContext_ProxyType const& proxy) {
  return BitPackerType::getField<eVrtContextProxyBits::Node,
                                 vrtCntx_node_num_bits,
                                 NodeType>(proxy);
}

/*static*/ VrtContext_IdType VrtContextProxy::getVrtContextId(
    VrtContext_ProxyType const& proxy) {
  return BitPackerType::getField<eVrtContextProxyBits::ID,
                                 vrtCntx_id_num_bits,
                                 VrtContext_IdType>(proxy);
}

}}  // end namespace vt::vrt