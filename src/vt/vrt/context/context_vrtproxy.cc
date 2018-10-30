
#include "config.h"
#include "vrt/context/context_vrtproxy.h"

namespace vt {namespace vrt {

/*static*/ VirtualProxyType VirtualProxyBuilder::createProxy(
  VirtualIDType const& id, NodeType const& node, bool const& is_coll,
  bool const& is_migratable
) {
  VirtualProxyType new_proxy = 0;

  setIsCollection(new_proxy, is_coll);
  setIsMigratable(new_proxy, is_migratable);
  setIsRemote(new_proxy, false);
  setVirtualNode(new_proxy, node);
  setVirtualID(new_proxy, id);

  return new_proxy;
}

/*static*/ VirtualProxyType VirtualProxyBuilder::createRemoteProxy(
  VirtualRemoteIDType const& id, NodeType const& this_node,
  NodeType const& target_node, bool const& is_coll, bool const& is_migratable
) {
  VirtualProxyType new_proxy = 0;

  setIsCollection(new_proxy, is_coll);
  setIsMigratable(new_proxy, is_migratable);
  setIsRemote(new_proxy, true);
  setVirtualNode(new_proxy, target_node);
  setVirtualRemoteID(new_proxy, id);
  setVirtualRemoteNode(new_proxy, this_node);

  auto const& id1 = VirtualProxyBuilder::getVirtualID(new_proxy);

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

/*static*/ void VirtualProxyBuilder::setIsRemote(
  VirtualProxyType& proxy, bool const& is_remote
) {
  BitPackerType::boolSetField<eVirtualProxyBits::Remote>(proxy, is_remote);
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

/*static*/ void VirtualProxyBuilder::setVirtualRemoteNode(
  VirtualProxyType& proxy, NodeType const& node
) {
  BitPackerType::setField<
    eVirtualProxyRemoteBits::RemoteNode, virtual_node_num_bits
  >(proxy, node);
}

/*static*/ void VirtualProxyBuilder::setVirtualRemoteID(
  VirtualProxyType& proxy, VirtualRemoteIDType const& id
) {
  BitPackerType::setField<
    eVirtualProxyRemoteBits::RemoteID, virtual_remote_id_num_bits
  >(proxy, id);
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

/*static*/ bool VirtualProxyBuilder::isRemote(
  VirtualProxyType const& proxy
) {
  return BitPackerType::boolGetField<eVirtualProxyBits::Remote>(proxy);
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
