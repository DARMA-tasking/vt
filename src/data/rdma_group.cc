
#include "common.h"
#include "rdma_common.h"
#include "rdma_group.h"
#include "transport.h"

#include <unordered_map>

namespace runtime { namespace rdma {

void
Group::set_map(rdma_map_t const& in_map) {
  map = in_map;
}

Group::rdma_map_t
Group::get_map() const {
  return map;
}

}} //end namespace runtime::rdma
