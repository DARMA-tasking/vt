/*
//@HEADER
// *****************************************************************************
//
//                                elm_id_bits.cc
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

#include "vt/elm/elm_id_bits.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_bits.h"
#include "vt/vrt/collection/balance/node_lb_data.h"

namespace vt { namespace elm {

/*static*/ ElementIDStruct ElmIDBits::createCollection(
  bool migratable, NodeT curr_node
) {
  auto const seq_id = theNodeLBData()->getNextElm();
  auto const home_node = theContext()->getNodeStrong();
  return createCollectionImpl(migratable, seq_id, home_node, curr_node);
}

/*static*/ ElementIDStruct ElmIDBits::createCollectionImpl(
  bool migratable, ElementIDType seq_id, NodeT home_node, NodeT curr_node
) {
  ElementIDType ret = 0;
  setCollectionID(ret, migratable, seq_id, home_node);
  return ElementIDStruct{ret, curr_node};
}

/*static*/ ElementIDStruct ElmIDBits::createObjGroup(
  ObjGroupProxyType proxy, NodeT node
) {
  ElementIDType ret = 0;
  setObjGroup(ret, proxy, node);
  auto const this_node = theContext()->getNodeStrong();
  return ElementIDStruct{ret, this_node};
}

/*static*/ ElementIDStruct ElmIDBits::createBareHandler(NodeT node) {
  ElementIDType ret = 0;
  BitPackerType::setField<eElmIDProxyBitsObjGroup::Control, num_control_bits>(
    ret, BareHandler
  );
  constexpr auto num_node_bits = BitCounterType <NodeT  >::value;
  BitPackerType::setField<eElmIDProxyBitsNonObjGroup::Node, num_node_bits>(
    ret, node
  );
  return ElementIDStruct{ret, node};
}

/*static*/ void ElmIDBits::setObjGroup(
  ElementIDType& id, ObjGroupProxyType proxy, NodeT node
) {
  BitPackerType::setField<eElmIDProxyBitsObjGroup::Control, num_control_bits>(
    id, ObjGroup
  );
  objgroup::proxy::ObjGroupProxy::setNode(proxy, node);
  constexpr auto proxy_bits = BitCounterType<ObjGroupProxyType>::value - 2;
  BitPackerType::setField<eElmIDProxyBitsObjGroup::ObjGroupID, proxy_bits>(
    id, proxy
  );
}

/*static*/ void ElmIDBits::setCollectionID(
  ElementIDType& id, bool migratable, ElementIDType seq_id, NodeT node
) {
  BitPackerType::setField<eElmIDProxyBitsNonObjGroup::Control2, num_control_bits>(
    id, migratable ? CollectionMigratable : CollectionNonMigratable
  );
  constexpr auto num_node_bits = BitCounterType <NodeT  >::value;
  BitPackerType::setField<eElmIDProxyBitsNonObjGroup::Node, num_node_bits>(
    id, node
  );
  BitPackerType::setField<eElmIDProxyBitsNonObjGroup::ID, elm_id_num_bits>(
    id, seq_id
  );
}

/*static*/ eElmIDControlBits ElmIDBits::getControlBits(ElementIDType id) {
  auto const n = num_control_bits;
  auto r = BitPackerType::getField<eElmIDProxyBitsObjGroup::Control, n, int>(id);
  return static_cast<eElmIDControlBits>(r);
}

/*static*/ bool ElmIDBits::isMigratable(ElementIDType id) {
  auto const ctrl = getControlBits(id);
  return not (
    ctrl == BareHandler or ctrl == ObjGroup or ctrl == CollectionNonMigratable
  );
}

/*static*/ NodeT ElmIDBits::getNode(ElementIDType id) {
  auto const ctrl = getControlBits(id);
  if (ctrl == ObjGroup) {
    auto const proxy = ElmIDBits::getObjGroupProxy(id, true);
    return objgroup::proxy::ObjGroupProxy::getNode(proxy);
  } else {
    constexpr auto num_node_bits = BitCounterType <NodeT  >::value;
    return BitPackerType::getField<
      eElmIDProxyBitsNonObjGroup::Node, num_node_bits, NodeT
    >(id);
  }
}

/*static*/ ObjGroupProxyType ElmIDBits::getObjGroupProxy(
  ElementIDType id, bool include_node
) {
  constexpr auto proxy_bits = BitCounterType<ObjGroupProxyType>::value - 2;
  auto proxy = BitPackerType::getField<
    eElmIDProxyBitsObjGroup::ObjGroupID, proxy_bits, ObjGroupProxyType
  >(id);
  if (not include_node) {
    objgroup::proxy::ObjGroupProxy::setNode(proxy, NodeT{0});
  }
  return proxy;
}

}} /* end namespace vt::elm */
