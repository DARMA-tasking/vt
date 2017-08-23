
#if ! defined __RUNTIME_TRANSPORT_RDMA_CHANNEL_LOOKUP__
#define __RUNTIME_TRANSPORT_RDMA_CHANNEL_LOOKUP__

#include "common.h"
#include "rdma_common.h"
#include "rdma_handle.h"

#include <functional>

namespace runtime { namespace rdma {

struct ChannelLookup {
  rdma_handle_t handle = no_rdma_handle;
  node_t target = uninitialized_destination;
  node_t non_target = uninitialized_destination;

  ChannelLookup(
    rdma_handle_t const& han, node_t const& in_target,
    node_t const& in_non_target
  ) : handle(han), target(in_target), non_target(in_non_target)
  {
    assert(target != uninitialized_destination);
    assert(non_target != uninitialized_destination);
  }

  ChannelLookup(ChannelLookup const&) = default;

};

inline bool
operator==(ChannelLookup const& c1, ChannelLookup const& c2) {
  return c1.handle == c2.handle and c1.target == c2.target and
    c1.non_target == c2.non_target;
}

}} //end namespace runtime::rdma

namespace std {
  using rdma_channel_lookup_t = runtime::rdma::ChannelLookup;

  template <>
  struct hash<rdma_channel_lookup_t> {
    size_t
    operator()(rdma_channel_lookup_t const& in) const {
      auto const& combined =
        std::hash<runtime::rdma_handle_t>()(in.handle) ^
        std::hash<runtime::node_t>()(in.target) ^
        std::hash<runtime::node_t>()(in.non_target);
      return combined;
    }
  };
}

#endif /*__RUNTIME_TRANSPORT_RDMA_CHANNEL_LOOKUP__*/
