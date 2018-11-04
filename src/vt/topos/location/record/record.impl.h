
#if !defined INCLUDED_TOPOS_LOCATION_RECORD_RECORD_IMPL_H
#define INCLUDED_TOPOS_LOCATION_RECORD_RECORD_IMPL_H

#include "vt/config.h"
#include "vt/topos/location/record/record.h"
#include "vt/topos/location/record/state_printer.h"

namespace vt { namespace location {

template <typename EntityID>
LocRecord<EntityID>::LocRecord(
  EntityID const& in_id, LocStateType const& in_state,
  NodeType const& in_node
) : id_(in_id), state_(in_state), cur_node_(in_node)
{ }

template <typename EntityID>
void LocRecord<EntityID>::updateNode(NodeType const& new_node) {
  if (new_node == theContext()->getNode()) {
    state_ = eLocState::Local;
  } else {
    state_ = eLocState::Remote;
  }

  cur_node_ = new_node;
}

template <typename EntityID>
bool LocRecord<EntityID>::isLocal() const {
  return state_ == eLocState::Local;
}

template <typename EntityID>
bool LocRecord<EntityID>::isRemote() const {
  return state_ == eLocState::Remote;
}

template <typename EntityID>
NodeType LocRecord<EntityID>::getRemoteNode() const {
  return cur_node_;
}

template <typename EntityID>
EntityID LocRecord<EntityID>::getEntityID() const {
  return id_;
}

}}  // end namespace vt::location

#endif /*INCLUDED_TOPOS_LOCATION_RECORD_RECORD_IMPL_H*/
