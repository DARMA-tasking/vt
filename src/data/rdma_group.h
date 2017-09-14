
#if ! defined __RUNTIME_TRANSPORT_RDMA_GROUP__
#define __RUNTIME_TRANSPORT_RDMA_GROUP__

#include "configs/types/types_common.h"
#include "rdma_map.h"
#include "rdma_region.h"

#include <unordered_map>

namespace vt { namespace rdma {

struct Group {
  using RDMA_MapType = Map;
  using RDMA_RegionType = Region;

  Group(
    RDMA_MapType const& in_map, RDMA_ElmType const& in_total_elms,
    RDMA_BlockType const& in_num_blocks, ByteType const& in_elm_size
  ) : map(in_map), elm_size(in_elm_size), num_total_elems(in_total_elms),
      num_blocks(in_num_blocks)
  { }

  void set_map(RDMA_MapType const& map);
  RDMA_MapType get_map() const;

  template <typename Walker>
  void walk_region(RDMA_RegionType const& region, Walker&& walk) {
    RDMA_ElmType lo = region.lo;
    RDMA_ElmType hi = region.hi;
    RDMA_ElmType elm_lo, elm_hi = lo;
    RDMA_BlockType blk_lo;
    NodeType cur_node;

    assert(map.elm_map != nullptr);
    assert(map.block_map != nullptr);

    while (elm_hi < hi) {
      auto const& ret = map.elm_map(lo, num_total_elems, num_blocks);
      blk_lo = std::get<0>(ret);
      elm_lo = std::get<1>(ret);
      elm_hi = std::get<2>(ret);
      cur_node = map.block_map(blk_lo, num_blocks);

      walk(cur_node, ret, lo, std::min(elm_hi,hi));

      lo = elm_hi;
    }
  }

  RDMA_MapType map;

  ByteType elm_size;
  RDMA_ElmType num_total_elems = no_rdma_elm;
  RDMA_BlockType num_blocks = no_rdma_block;
};

}} //end namespace vt::rdma

#endif /*__RUNTIME_TRANSPORT_RDMA_GROUP__*/
