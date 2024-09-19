/*
//@HEADER
// *****************************************************************************
//
//                                 rdma_group.h
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019-2024 National Technology & Engineering Solutions of Sandia, LLC
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

#if !defined INCLUDED_VT_RDMA_GROUP_RDMA_GROUP_H
#define INCLUDED_VT_RDMA_GROUP_RDMA_GROUP_H

#include "vt/config.h"
#include "vt/rdma/group/rdma_map.h"
#include "vt/rdma/group/rdma_region.h"

#include <unordered_map>

namespace vt { namespace rdma {

struct Group {
  using RDMA_MapType = Map;
  using RDMA_RegionType = Region;

  Group(
    RDMA_MapType const& in_map, RDMA_ElmType const& in_total_elms,
    RDMA_BlockType const& in_num_blocks, ByteType const& in_elm_size,
    bool const& in_unsized = false
  ) : map(in_map), elm_size(in_elm_size), num_total_elems(in_total_elms),
      num_blocks(in_num_blocks), unsized_(in_unsized)
  { }

  void set_map(RDMA_MapType const& map);
  RDMA_MapType get_map() const;

  template <typename Walker>
  void walk_region(RDMA_RegionType const& region, Walker&& walk) {
    RDMA_ElmType lo = region.lo;
    RDMA_ElmType hi = region.hi;
    RDMA_ElmType elm_hi = lo;
    RDMA_BlockType blk_lo;
    NodeType cur_node;

    vtAssertExpr(map.elm_map != nullptr);
    vtAssertExpr(map.block_map != nullptr);

    while (elm_hi < hi) {
      auto const& ret = map.elm_map(lo, num_total_elems, num_blocks);
      blk_lo = std::get<0>(ret);
      elm_hi = std::get<2>(ret);
      cur_node = map.block_map(blk_lo, num_blocks);

      walk(cur_node, ret, lo, std::min(elm_hi,hi));

      lo = elm_hi;
    }
  }

  NodeType findDefaultNode(RDMA_ElmType const& elm);

  template <typename Serializer>
  void serialize(Serializer& s) {
    s | map
      | elm_size
      | num_total_elems
      | num_blocks
      | unsized_;
  }

  RDMA_MapType map;

  ByteType elm_size;
  RDMA_ElmType num_total_elems;
  RDMA_BlockType num_blocks = no_rdma_block;
  bool unsized_ = false;
};

}} //end namespace vt::rdma

#endif /*INCLUDED_VT_RDMA_GROUP_RDMA_GROUP_H*/
