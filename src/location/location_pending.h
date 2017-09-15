
#if ! defined __RUNTIME_TRANSPORT_LOCATION_PENDING__
#define __RUNTIME_TRANSPORT_LOCATION_PENDING__

#include "config.h"
#include "location_common.h"

namespace vt { namespace location {

template <typename EntityID>
struct PendingLocationLookup {
  EntityID entity;
  NodeActionType action = nullptr;

  PendingLocationLookup(
    EntityID const& in_entity, NodeActionType const& in_action
  ) : entity(in_entity), action(in_action)
  { }

  void applyNodeAction(NodeType const& node) {
    action(node);
  }
};

}} // end namespace vt::location

#endif /*__RUNTIME_TRANSPORT_LOCATION_PENDING__*/
