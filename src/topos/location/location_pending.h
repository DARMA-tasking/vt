
#if !defined INCLUDED_TOPOS_LOCATION_PENDING
#define INCLUDED_TOPOS_LOCATION_PENDING

#include "config.h"
#include "location_common.h"

namespace vt { namespace location {

template <typename EntityID>
struct PendingLocationLookup {
  EntityID entity;
  NodeActionType action = nullptr;

  PendingLocationLookup(
      EntityID const& in_entity, NodeActionType const& in_action
  ) : entity(in_entity), action(in_action) {}

  void applyNodeAction(NodeType const& node) {
    action(node);
  }
};

}}  // end namespace vt::location

#endif  /*INCLUDED_TOPOS_LOCATION_PENDING*/
