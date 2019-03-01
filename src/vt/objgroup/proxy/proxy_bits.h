/*
//@HEADER
// ************************************************************************
//
//                          proxy_bits.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VT_OBJGROUP_PROXY_PROXY_BITS_H
#define INCLUDED_VT_OBJGROUP_PROXY_PROXY_BITS_H

#include "vt/config.h"
#include "vt/utils/bits/bits_common.h"

namespace vt { namespace objgroup { namespace proxy {

static constexpr BitCountType const objgrp_is_collective_num_bits = 1;
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
  Collective = 0,
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
  static void setIsCollective(ObjGroupProxyType& proxy, bool is_coll);
  static void setNode(ObjGroupProxyType& proxy, NodeType const& node);
  static void setID(ObjGroupProxyType& proxy, ObjGroupIDType id);
  static void setTypeIdx(ObjGroupProxyType& proxy, ObjTypeIdxType idx);

  // Getters for obtaining info about the bit-pattern in the obj-group proxy
  static bool isCollective(ObjGroupProxyType proxy);
  static NodeType getNode(ObjGroupProxyType proxy);
  static ObjGroupIDType getID(ObjGroupProxyType proxy);
  static ObjTypeIdxType getTypeIdx(ObjGroupProxyType proxy);
};

}}} /* end namespace vt::objgroup::proxy */

#endif /*INCLUDED_VT_OBJGROUP_PROXY_PROXY_BITS_H*/
