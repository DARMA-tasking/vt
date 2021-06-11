/*
//@HEADER
// *****************************************************************************
//
//                                record.impl.h
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

#if !defined INCLUDED_TOPOS_LOCATION_RECORD_RECORD_IMPL_H
#define INCLUDED_TOPOS_LOCATION_RECORD_RECORD_IMPL_H

#include "vt/config.h"
#include "vt/topos/location/record/record.h"
#include "vt/topos/location/record/state_printer.h"

namespace vt { namespace location {

template <typename EntityID>
LocRecord<EntityID>::LocRecord(
  EntityID const& in_id, LocStateType const& in_state,
  NodeType const& in_node
) : id_(in_id), state_(in_state), cur_node_(in_node)
{ }

template <typename EntityID>
void LocRecord<EntityID>::updateNode(NodeType const& new_node) {
  if (new_node == theContext()->getNode()) {
    state_ = eLocState::Local;
  } else {
    state_ = eLocState::Remote;
  }

  cur_node_ = new_node;
}

template <typename EntityID>
bool LocRecord<EntityID>::isLocal() const {
  return state_ == eLocState::Local;
}

template <typename EntityID>
bool LocRecord<EntityID>::isRemote() const {
  return state_ == eLocState::Remote;
}

template <typename EntityID>
NodeType LocRecord<EntityID>::getRemoteNode() const {
  return cur_node_;
}

template <typename EntityID>
EntityID LocRecord<EntityID>::getEntityID() const {
  return id_;
}

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_RECORD_RECORD_IMPL_H*/
