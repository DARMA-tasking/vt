
#if !defined INCLUDED_RDMA_RDMA_MAP_H
#define INCLUDED_RDMA_RDMA_MAP_H

#include "config.h"
#include "rdma_common.h"

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
