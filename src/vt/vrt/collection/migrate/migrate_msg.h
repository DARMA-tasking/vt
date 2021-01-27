/*
//@HEADER
// *****************************************************************************
//
//                                migrate_msg.h
//                           DARMA Toolkit v. 1.0.0
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

#if !defined INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_MSG_H
#define INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_MSG_H

#include "vt/config.h"
#include "vt/messaging/message.h"
#include "vt/vrt/proxy/collection_elm_proxy.h"
#include "vt/vrt/collection/collection_info.h"

namespace vt { namespace vrt { namespace collection {

template <typename ColT, typename IndexT>
struct MigrateMsg final : ::vt::Message {
  using MessageParentType = ::vt::Message;
  vt_msg_serialize_required(); // by elm_, maybe others

  MigrateMsg() = default;
  MigrateMsg(
    VrtElmProxy<ColT, IndexT> const& in_elm_proxy, NodeType const& in_from,
    NodeType const& in_to, HandlerType const in_map_fn, IndexT const& in_range,
    ColT* in_elm
  ) : elm_proxy_(in_elm_proxy), from_(in_from), to_(in_to), map_fn_(in_map_fn),
      range_(in_range), elm_(in_elm)
  { }

  VrtElmProxy<ColT, IndexT> getElementProxy() const { return elm_proxy_; }
  NodeType getFromNode() const { return from_; }
  NodeType getToNode() const { return to_; }
  HandlerType getMapHandler() const { return map_fn_; }
  IndexT getRange() const { return range_; }

  template <typename Serializer>
  void serialize(Serializer& s) {
    MessageParentType::serialize(s);
    s | elm_proxy_ | from_ | to_ | map_fn_ | range_;
    if (s.isUnpacking()) {
      elm_ = new ColT{};
    }
    s | *elm_;
  }

private:
  VrtElmProxy<ColT, IndexT> elm_proxy_;
  NodeType from_ = uninitialized_destination;
  NodeType to_ = uninitialized_destination;
  HandlerType map_fn_ = uninitialized_handler;
  IndexT range_;
public:
  ColT* elm_ = nullptr;
};

}}} /* end namespace vt::vrt::collection */

#endif /*INCLUDED_VRT_COLLECTION_MIGRATE_MIGRATE_MSG_H*/
