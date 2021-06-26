/*
//@HEADER
// *****************************************************************************
//
//                            rdma_channel_lookup.h
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

#if !defined INCLUDED_VT_RDMA_CHANNEL_RDMA_CHANNEL_LOOKUP_H
#define INCLUDED_VT_RDMA_CHANNEL_RDMA_CHANNEL_LOOKUP_H

#include "vt/config.h"
#include "vt/rdma/rdma_common.h"
#include "vt/rdma/rdma_handle.h"

#include <functional>

namespace vt { namespace rdma {

struct ChannelLookup {
  RDMA_HandleType handle = no_rdma_handle;
  NodeType target = uninitialized_destination;
  NodeType non_target = uninitialized_destination;

  ChannelLookup(
    RDMA_HandleType const& han, NodeType const& in_target,
    NodeType const& in_non_target
  ) : handle(han), target(in_target), non_target(in_non_target)
  {
    vtAssertExpr(target != uninitialized_destination);
    vtAssertExpr(non_target != uninitialized_destination);
  }

  ChannelLookup(ChannelLookup const&) = default;

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | handle
      | target
      | non_target;
  }
};

inline bool
operator==(ChannelLookup const& c1, ChannelLookup const& c2) {
  return c1.handle == c2.handle and c1.target == c2.target and
    c1.non_target == c2.non_target;
}

}} //end namespace vt::rdma

namespace std {
  using RDMA_ChannelLookupType = vt::rdma::ChannelLookup;

  template <>
  struct hash<RDMA_ChannelLookupType> {
    size_t operator()(RDMA_ChannelLookupType const& in) const {
      auto const& combined =
        std::hash<vt::RDMA_HandleType>()(in.handle) ^
        std::hash<vt::NodeType>()(in.target) ^
        std::hash<vt::NodeType>()(in.non_target);
      return combined;
    }
  };
}

#endif /*INCLUDED_VT_RDMA_CHANNEL_RDMA_CHANNEL_LOOKUP_H*/
