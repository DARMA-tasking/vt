/*
//@HEADER
// ************************************************************************
//
//                          rdma_map.h
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

#if !defined INCLUDED_RDMA_RDMA_MAP_H
#define INCLUDED_RDMA_RDMA_MAP_H

#include "vt/config.h"
#include "vt/rdma/rdma_common.h"

#include <sstream>
#include <iostream>

namespace vt { namespace rdma {

struct Map {
  RDMA_BlockMapType block_map = nullptr;
  RDMA_ElmMapType elm_map = nullptr;

  Map() = default;

  Map(RDMA_BlockMapType in_block_map, RDMA_ElmMapType in_elm_map)
    : block_map(in_block_map), elm_map(in_elm_map)
  { }

  static NodeType defaultBlockMap(
    RDMA_BlockType block, RDMA_BlockType __attribute__((unused)) num_blocks
  ) {
    auto const& num_nodes = theContext()->getNumNodes();
    return block % num_nodes;
  };

  static RDMA_BlockElmRangeType defaultElmMap(
    RDMA_ElmType elm, RDMA_ElmType num_elms, RDMA_BlockType num_blocks
  ) {
    auto const& block_size = num_elms / num_blocks;
    auto const& block = elm / block_size;
    return RDMA_BlockElmRangeType{
      block, block * block_size, (block + 1) * block_size
    };
  }
};

static Map default_map = Map(Map::defaultBlockMap, Map::defaultElmMap);

}} //end namespace vt::rdma

#endif /*INCLUDED_RDMA_RDMA_MAP_H*/
