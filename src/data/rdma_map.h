
#if ! defined __RUNTIME_TRANSPORT_RDMA_MAP__
#define __RUNTIME_TRANSPORT_RDMA_MAP__

#include "common.h"
#include "rdma_common.h"

#include <sstream>
#include <iostream>

namespace runtime { namespace rdma {

struct Map {
  RDMA_BlockMapType block_map = nullptr;
  RDMA_ElmMapType elm_map = nullptr;

  Map()  = default;

  Map(RDMA_BlockMapType in_block_map, RDMA_ElmMapType in_elm_map)
    : block_map(in_block_map), elm_map(in_elm_map)
  { }

  static NodeType
  default_block_map(RDMA_BlockType block, RDMA_BlockType num_blocks) {
    auto const& num_nodes = the_context->get_num_nodes();
    return block % num_nodes;
  };

  static RDMA_BlockElmRangeType
  default_elm_map(RDMA_ElmType elm, RDMA_ElmType num_elms, RDMA_BlockType num_blocks) {
    auto const& block_size = num_elms / num_blocks;
    auto const& block = elm / block_size;
    return RDMA_BlockElmRangeType{block, (block)*block_size, (block+1)*block_size};
  }
};

static Map default_map = Map(Map::default_block_map, Map::default_elm_map);

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_MAP__*/
