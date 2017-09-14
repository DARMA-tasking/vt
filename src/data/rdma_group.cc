
#include "configs/types/types_common.h"
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

}} //end namespace vt::rdma
