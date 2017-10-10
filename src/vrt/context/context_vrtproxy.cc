
#include "context_vrtproxy.h"

namespace vt {namespace vrt {

/*static*/ VirtualProxyType VirtualProxyBuilder::createProxy(
  VirtualIDType const& id, NodeType const& node, bool const& is_coll,
  bool const& is_migratable
) {
  VirtualProxyType new_proxy = 0;

  setIsCollection(new_proxy, is_coll);
  setIsMigratable(new_proxy, is_migratable);
  setVirtualNode(new_proxy, node);
  setVirtualID(new_proxy, id);

  return new_proxy;
}

/*static*/ void VirtualProxyBuilder::setIsCollection(
  VirtualProxyType& proxy, bool const& is_coll
) {
  BitPackerType::boolSetField<eVirtualProxyBits::Collection>(proxy, is_coll);
}

/*static*/ void VirtualProxyBuilder::setIsMigratable(
  VirtualProxyType& proxy, bool const& is_mig
) {
  BitPackerType::boolSetField<eVirtualProxyBits::Migratable>(proxy, is_mig);
}

/*static*/ void VirtualProxyBuilder::setVirtualNode(
  VirtualProxyType& proxy, NodeType const& node
) {
  BitPackerType::setField<eVirtualProxyBits::Node, virtual_node_num_bits>(
    proxy, node
  );
}

/*static*/ void VirtualProxyBuilder::setVirtualID(
  VirtualProxyType& proxy, VirtualIDType const& id
) {
  BitPackerType::setField<eVirtualProxyBits::ID, virtual_id_num_bits>(
    proxy, id
  );
}

/*static*/ bool VirtualProxyBuilder::isCollection(
  VirtualProxyType const& proxy
) {
  return BitPackerType::boolGetField<eVirtualProxyBits::Collection>(proxy);
}

/*static*/ bool VirtualProxyBuilder::isMigratable(
  VirtualProxyType const& proxy
) {
  return BitPackerType::boolGetField<eVirtualProxyBits::Migratable>(proxy);
}

/*static*/ NodeType VirtualProxyBuilder::getVirtualNode(
  VirtualProxyType const& proxy
) {
  return BitPackerType::getField<
    eVirtualProxyBits::Node, virtual_node_num_bits, NodeType
  >(proxy);
}

/*static*/ VirtualIDType VirtualProxyBuilder::getVirtualID(
  VirtualProxyType const& proxy
) {
  return BitPackerType::getField<
    eVirtualProxyBits::ID, virtual_id_num_bits, VirtualIDType
  >(proxy);
}

}}  // end namespace vt::vrt
