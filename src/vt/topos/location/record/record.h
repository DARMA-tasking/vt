
#if !defined INCLUDED_TOPOS_LOCATION_RECORD_RECORD_H
#define INCLUDED_TOPOS_LOCATION_RECORD_RECORD_H

#include "config.h"
#include "topos/location/record/state.h"

#include <iostream>

namespace vt { namespace location {

template <typename EntityID>
struct LocRecord {
  using LocStateType = eLocState;

  LocRecord(
    EntityID const& in_id, LocStateType const& in_state,
    NodeType const& in_node
  );
  void updateNode(NodeType const& new_node);

  bool isLocal() const;
  bool isRemote() const;
  NodeType getRemoteNode() const;
  EntityID getEntityID() const;

  template <typename U>
  friend std::ostream& operator<<(std::ostream& os, LocRecord<U> const& rec);

private:
  EntityID id_;
  LocStateType state_ = eLocState::Invalid;
  NodeType cur_node_ = uninitialized_destination;
};

}}  // end namespace vt::location

#include "topos/location/record/record.impl.h"

#endif /*INCLUDED_TOPOS_LOCATION_RECORD_RECORD_H*/
