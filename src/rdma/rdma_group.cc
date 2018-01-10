
#include "config.h"
#include "rdma_common.h"
#include "rdma_group.h"

#include <unordered_map>

namespace vt { namespace rdma {

void
Group::set_map(RDMA_MapType const& in_map) {
  map = in_map;
}

Group::RDMA_MapType
Group::get_map() const {
  return map;
}

NodeType
Group::findDefaultNode(RDMA_ElmType const& elm) {
  auto const& elms = num_total_elems;
  auto const& default_node = map.block_map(elm, elms);
  return default_node;
}

}} //end namespace vt::rdma
