/*
//@HEADER
// *****************************************************************************
//
//                                proxy_bits.cc
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
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
#include "vt/objgroup/common.h"
#include "vt/objgroup/proxy/proxy_bits.h"

namespace vt { namespace objgroup { namespace proxy {

/*static*/ ObjGroupProxyType ObjGroupProxy::create(
  ObjGroupIDType id, ObjTypeIdxType idx, NodeType node, bool is_collective
) {
  constexpr NodeType const default_node = 0;
  NodeType const target_node = is_collective ? default_node : node;
  ObjGroupProxyType new_proxy = 0;

  setControl(new_proxy);
  setIsCollective(new_proxy, is_collective);
  setNode(new_proxy, target_node);
  setID(new_proxy, id);
  setTypeIdx(new_proxy, idx);

  return new_proxy;
}

/*static*/ void ObjGroupProxy::setControl(
  ObjGroupProxyType& proxy, bool is_objgroup
) {
  BitPackerType::boolSetField<eObjGroupProxyBits::ObjGroup>(proxy, is_objgroup);
}

/*static*/ void ObjGroupProxy::setIsCollective(
  ObjGroupProxyType& proxy, bool is_coll
) {
  BitPackerType::boolSetField<eObjGroupProxyBits::Collective>(proxy, is_coll);
}

/*static*/ void ObjGroupProxy::setNode(
  ObjGroupProxyType& proxy, NodeType const& node
) {
  BitPackerType::setField<eObjGroupProxyBits::Node, objgrp_node_num_bits>(
    proxy, node
  );
}

/*static*/ void ObjGroupProxy::setID(
  ObjGroupProxyType& proxy, ObjGroupIDType id
) {
  BitPackerType::setField<eObjGroupProxyBits::ID, objgrp_id_num_bits>(
    proxy, id
  );
}

/*static*/ void ObjGroupProxy::setTypeIdx(
  ObjGroupProxyType& proxy, ObjTypeIdxType idx
) {
  BitPackerType::setField<eObjGroupProxyBits::TypeIdx, objgrp_idx_num_bits>(
    proxy, idx
  );
}

/*static*/ bool ObjGroupProxy::isControl(ObjGroupProxyType proxy) {
  return BitPackerType::boolGetField<eObjGroupProxyBits::ObjGroup>(proxy);
}

/*static*/ bool ObjGroupProxy::isCollective(ObjGroupProxyType proxy) {
  return BitPackerType::boolGetField<eObjGroupProxyBits::Collective>(proxy);
}

/*static*/ NodeType ObjGroupProxy::getNode(ObjGroupProxyType proxy) {
  return BitPackerType::getField<
    eObjGroupProxyBits::Node, objgrp_node_num_bits, NodeType
  >(proxy);
}

/*static*/ ObjGroupIDType ObjGroupProxy::getID(ObjGroupProxyType proxy) {
  return BitPackerType::getField<
    eObjGroupProxyBits::ID, objgrp_id_num_bits, ObjGroupIDType
  >(proxy);
}

/*static*/ ObjTypeIdxType ObjGroupProxy::getTypeIdx(ObjGroupProxyType proxy) {
  return BitPackerType::getField<
    eObjGroupProxyBits::TypeIdx, objgrp_idx_num_bits, ObjTypeIdxType
  >(proxy);
}

}}} /* end namespace vt::objgroup::proxy */
