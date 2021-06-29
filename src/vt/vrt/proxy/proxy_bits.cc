/*
//@HEADER
// *****************************************************************************
//
//                                proxy_bits.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2021 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#include "vt/config.h"
#include "vt/vrt/proxy/proxy_bits.h"

namespace vt {namespace vrt {

/*static*/ VirtualProxyType VirtualProxyBuilder::createProxy(
  VirtualIDType const& id, NodeType const& node, bool const& is_coll,
  bool const& is_migratable, bool const& is_distributed
) {
  constexpr NodeType const default_remote_node = 0;
  VirtualProxyType new_proxy = 0;
  NodeType remote_node = is_distributed ? default_remote_node : node;

  setIsCollection(new_proxy, is_coll);
  setIsMigratable(new_proxy, is_migratable);
  setIsRemote    (new_proxy, is_distributed);
  setVirtualNode (new_proxy, remote_node);
  setVirtualID   (new_proxy, id);

  return new_proxy;
}

/*static*/ VirtualProxyType VirtualProxyBuilder::createRemoteProxy(
  VirtualRemoteIDType const& id, NodeType const& this_node,
  NodeType const& target_node, bool const& is_coll, bool const& is_migratable
) {
  VirtualProxyType new_proxy = 0;

  setIsCollection     (new_proxy, is_coll);
  setIsMigratable     (new_proxy, is_migratable);
  setIsRemote         (new_proxy, true);
  setVirtualNode      (new_proxy, target_node);
  setVirtualRemoteID  (new_proxy, id);
  setVirtualRemoteNode(new_proxy, this_node);

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
