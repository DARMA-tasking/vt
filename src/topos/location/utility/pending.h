
#if !defined INCLUDED_TOPOS_LOCATION_UTILITY_PENDING_H
#define INCLUDED_TOPOS_LOCATION_UTILITY_PENDING_H

#include "config.h"
#include "topos/location/location_common.h"

namespace vt { namespace location {

template <typename EntityID>
struct PendingLocationLookup {
  PendingLocationLookup(EntityID const& in_entity, NodeActionType const& in_act)
    : entity_(in_entity), action_(in_act)
  { }

  void applyNodeAction(NodeType const& node) {
    action_(node);
  }

private:
  EntityID entity_;
  NodeActionType action_ = nullptr;
};

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_UTILITY_PENDING_H*/
