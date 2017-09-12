
#if ! defined __RUNTIME_TRANSPORT_RDMA_CHANNEL_LOOKUP__
#define __RUNTIME_TRANSPORT_RDMA_CHANNEL_LOOKUP__

#include "common.h"
#include "rdma_common.h"
#include "rdma_handle.h"

#include <functional>

namespace runtime { namespace rdma {

struct ChannelLookup {
  RDMA_HandleType handle = no_rdma_handle;
  NodeType target = uninitialized_destination;
  NodeType non_target = uninitialized_destination;

  ChannelLookup(
    RDMA_HandleType const& han, NodeType const& in_target,
    NodeType const& in_non_target
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
    size_t operator()(rdma_channel_lookup_t const& in) const {
      auto const& combined =
        std::hash<runtime::RDMA_HandleType>()(in.handle) ^
        std::hash<runtime::NodeType>()(in.target) ^
        std::hash<runtime::NodeType>()(in.non_target);
      return combined;
    }
  };
}

#endif /*__RUNTIME_TRANSPORT_RDMA_CHANNEL_LOOKUP__*/
