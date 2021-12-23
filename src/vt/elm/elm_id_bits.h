/*
//@HEADER
// *****************************************************************************
//
//                                elm_id_bits.h
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

#if !defined INCLUDED_VT_ELM_ELM_ID_BITS_H
#define INCLUDED_VT_ELM_ELM_ID_BITS_H

#include "vt/configs/types/types_type.h"
#include "vt/utils/bits/bits_common.h"
#include "vt/elm/elm_id.h"

namespace vt { namespace elm {

enum eElmIDControlBits {
  ObjGroup                = 0,  /**< An objgroup element ID (non-migratable) */
  BareHandler             = 1,  /**< A bare handler element ID */
  CollectionNonMigratable = 2,  /**< A non-migratable collection element */
  CollectionMigratable    = 3   /**< A migratable collection element */
};

static constexpr BitCountType const num_control_bits = 2;

enum eElmIDProxyBitsObjGroup {
  Control    = 0,
  ObjGroupID = num_control_bits
};

enum eElmIDProxyBitsNonObjGroup {
  Control2   = 0,
  Node       = num_control_bits,
  ID         = eElmIDProxyBitsNonObjGroup::Node + BitCounterType<NodeType>::value
};

static constexpr BitCountType const elm_id_num_bits =
  BitCounterType<ElementIDType>::value - (2 + BitCounterType<NodeType>::value);

struct ElmIDBits {
  static ElementIDStruct createCollection(bool migratable, NodeType curr_node);
  static ElementIDStruct createObjGroup(ObjGroupProxyType proxy, NodeType node);
  static ElementIDStruct createBareHandler(NodeType node);

  static ElementIDStruct createCollectionImpl(
    bool migratable, ElementIDType seq_id, NodeType home_node, NodeType curr_node
  );

  static void setObjGroup(
    ElementIDType& id, ObjGroupProxyType proxy, NodeType node
  );
  static void setCollectionID(
    ElementIDType& id, bool migratable, ElementIDType seq_id, NodeType node
  );

  static eElmIDControlBits getControlBits(ElementIDType id);
  static bool isMigratable(ElementIDType id);
  static NodeType getNode(ElementIDType id);
  static ObjGroupProxyType getObjGroupProxy(ElementIDType id, bool include_node);
};

}} /* end namespace vt::elm */

#endif /*INCLUDED_VT_ELM_ELM_ID_BITS_H*/
