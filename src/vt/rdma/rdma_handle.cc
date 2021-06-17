/*
//@HEADER
// *****************************************************************************
//
//                                rdma_handle.cc
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

#include "vt/rdma/rdma_handle.h"
#include "vt/utils/bits/bits_common.h"

namespace vt { namespace rdma {

/*static*/ void HandleManager::setIsSized(
  RDMA_UniversalIdType& handle, bool const& is_sized
) {
  BitPackerType::boolSetField<RDMA_BitsType::Sized>(handle, is_sized);
}

/*static*/ void HandleManager::setIsCollective(
  RDMA_UniversalIdType& handle, bool const& is_collective
) {
  BitPackerType::boolSetField<RDMA_BitsType::Collective>(handle, is_collective);
}

/*static*/ void HandleManager::setIsHandler(
  RDMA_UniversalIdType& handle, bool const& is_handler
) {
  BitPackerType::boolSetField<RDMA_BitsType::HandlerType>(handle, is_handler);
}

/*static*/ void HandleManager::setOpType(
  RDMA_UniversalIdType& handle, RDMA_TypeType const& rdma_type
) {
  BitPackerType::setField<RDMA_BitsType::OpType, rdma_type_num_bits>(
    handle, rdma_type
  );
}

/*static*/ void HandleManager::setRdmaNode(
  RDMA_UniversalIdType& handle, NodeType const& node
) {
  BitPackerType::setField<RDMA_BitsType::Node, node_num_bits>(handle, node);
}

/*static*/ void HandleManager::setRdmaIdentifier(
  RDMA_UniversalIdType& handle, RDMA_IdentifierType const& ident
) {
  BitPackerType::setField<RDMA_BitsType::Identifier, rdma_identifier_num_bits>(
    handle, ident
  );
}

/*static*/ NodeType HandleManager::getRdmaNode(
  RDMA_UniversalIdType const& handle
) {
  return BitPackerType::getField<RDMA_BitsType::Node, node_num_bits, NodeType>(handle);
}

/*static*/ RDMA_IdentifierType HandleManager::getRdmaIdentifier(
  RDMA_UniversalIdType const& handle
) {
  return BitPackerType::getField<
    RDMA_BitsType::Identifier, rdma_identifier_num_bits, RDMA_IdentifierType
  >(handle);
}

/*static*/ bool HandleManager::isSized(RDMA_UniversalIdType const& handle) {
  return BitPackerType::boolGetField<RDMA_BitsType::Sized>(handle);
}

/*static*/ bool HandleManager::isCollective(RDMA_UniversalIdType const& handle) {
  return BitPackerType::boolGetField<RDMA_BitsType::Collective>(handle);
}

/*static*/ bool HandleManager::isHandler(RDMA_UniversalIdType const& handle) {
  return BitPackerType::boolGetField<RDMA_BitsType::HandlerType>(handle);
}

/*static*/ HandleManager::RDMA_TypeType HandleManager::getOpType(
  RDMA_UniversalIdType const& handle
) {
  return BitPackerType::getField<
    RDMA_BitsType::OpType, rdma_type_num_bits, RDMA_TypeType
  >(handle);
}

/*static*/ HandleManager::RDMA_UniversalIdType
HandleManager::createNewHandler() {
  RDMA_TypeType const& type = RDMA_TypeType::Uninitialized;
  RDMA_HandleType handle = 0;
  setOpType(handle, type);
  return handle;
}

}} // end namespace vt::rdma

