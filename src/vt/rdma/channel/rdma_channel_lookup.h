
#if !defined INCLUDED_RDMA_RDMA_CHANNEL_LOOKUP_H
#define INCLUDED_RDMA_RDMA_CHANNEL_LOOKUP_H

#include "vt/config.h"
#include "vt/rdma/rdma_common.h"
#include "vt/rdma/rdma_handle.h"

#include <functional>

namespace vt { namespace rdma {

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

}} //end namespace vt::rdma

namespace std {
  using RDMA_ChannelLookupType = vt::rdma::ChannelLookup;

  template <>
  struct hash<RDMA_ChannelLookupType> {
    size_t operator()(RDMA_ChannelLookupType const& in) const {
      auto const& combined =
        std::hash<vt::RDMA_HandleType>()(in.handle) ^
        std::hash<vt::NodeType>()(in.target) ^
        std::hash<vt::NodeType>()(in.non_target);
      return combined;
    }
  };
}

#endif /*INCLUDED_RDMA_RDMA_CHANNEL_LOOKUP_H*/
