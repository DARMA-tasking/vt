
#if ! defined __RUNTIME_TRANSPORT_RDMA_MAP__
#define __RUNTIME_TRANSPORT_RDMA_MAP__

#include "common.h"
#include "rdma_common.h"

#include <sstream>
#include <iostream>

namespace runtime { namespace rdma {

struct Map {
  rdma_block_map_t block_map = nullptr;
  rdma_elm_map_t elm_map = nullptr;

  Map()  = default;

  Map(rdma_block_map_t in_block_map, rdma_elm_map_t in_elm_map)
    : block_map(in_block_map), elm_map(in_elm_map)
  { }

  static NodeType
  default_block_map(RDMA_BlockType block, RDMA_BlockType num_blocks) {
    auto const& num_nodes = the_context->get_num_nodes();
    return block % num_nodes;
  };

  static rdma_block_elm_range_t
  default_elm_map(RDMA_ElmType elm, RDMA_ElmType num_elms, RDMA_BlockType num_blocks) {
    auto const& block_size = num_elms / num_blocks;
    auto const& block = elm / block_size;
    return rdma_block_elm_range_t{block, (block)*block_size, (block+1)*block_size};
  }
};

static Map default_map = Map(Map::default_block_map, Map::default_elm_map);

}} //end namespace runtime::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_MAP__*/
