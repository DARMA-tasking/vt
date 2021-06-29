/*
//@HEADER
// *****************************************************************************
//
//                                 proxy_bits.h
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

#if !defined INCLUDED_VT_OBJGROUP_PROXY_PROXY_BITS_H
#define INCLUDED_VT_OBJGROUP_PROXY_PROXY_BITS_H

#include "vt/config.h"
#include "vt/utils/bits/bits_common.h"

namespace vt { namespace objgroup { namespace proxy {

static constexpr BitCountType const objgrp_is_collective_num_bits = 1;
static constexpr BitCountType const objgrp_control_num_bits = 1;
static constexpr BitCountType const objgrp_node_num_bits =
    BitCounterType<NodeType>::value;
static constexpr BitCountType const objgrp_id_num_bits =
    BitCounterType<ObjGroupIDType>::value;
static constexpr BitCountType const objgrp_proxy_num_bits =
    BitCounterType<ObjGroupProxyType>::value;
static constexpr BitCountType const objgrp_idx_num_bits =
   objgrp_proxy_num_bits -
  (objgrp_is_collective_num_bits + objgrp_node_num_bits + objgrp_id_num_bits);

enum eObjGroupProxyBits {
  ObjGroup   = 0,
  Collective = eObjGroupProxyBits::ObjGroup   + objgrp_control_num_bits,
  Node       = eObjGroupProxyBits::Collective + objgrp_is_collective_num_bits,
  TypeIdx    = eObjGroupProxyBits::Node       + objgrp_idx_num_bits,
  ID         = eObjGroupProxyBits::TypeIdx    + objgrp_node_num_bits
};

struct ObjGroupProxy {
  // Creation of a new proxy with properties
  static ObjGroupProxyType create(
    ObjGroupIDType id, ObjTypeIdxType idx, NodeType node, bool coll
  );

  // Setters for mixing the proxy bits
  static void setControl(ObjGroupProxyType& proxy, bool is_objgroup = true);
  static void setIsCollective(ObjGroupProxyType& proxy, bool is_coll);
  static void setNode(ObjGroupProxyType& proxy, NodeType const& node);
  static void setID(ObjGroupProxyType& proxy, ObjGroupIDType id);
  static void setTypeIdx(ObjGroupProxyType& proxy, ObjTypeIdxType idx);

  // Getters for obtaining info about the bit-pattern in the obj-group proxy
  static bool isControl(ObjGroupProxyType proxy);
  static bool isCollective(ObjGroupProxyType proxy);
  static NodeType getNode(ObjGroupProxyType proxy);
  static ObjGroupIDType getID(ObjGroupProxyType proxy);
  static ObjTypeIdxType getTypeIdx(ObjGroupProxyType proxy);
};

}}} /* end namespace vt::objgroup::proxy */

#endif /*INCLUDED_VT_OBJGROUP_PROXY_PROXY_BITS_H*/
