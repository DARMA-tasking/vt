
#if !defined INCLUDED_TOPOS_LOCATION_RECORD
#define INCLUDED_TOPOS_LOCATION_RECORD

#include <iostream>

#include "config.h"
#include "context/context.h"

namespace vt { namespace location {

enum class eLocState : int32_t {
  Local = 1,
  Remote = 2,
  Invalid = -1
};

#define PRINT_LOC_STATE(state)                          \
  ((state) == eLocState::Local ? "eLocState::Local" :   \
   ((state) == eLocState::Remote ? "eLocState::Remote" : \
    "eLocState::Invalid"))

template <typename EntityID>
struct LocRecord {
  using LocStateType = eLocState;

  LocRecord(
      EntityID const& in_id,
      LocStateType const& in_state,
      NodeType const& in_node
  ) : id_(in_id), state_(in_state), cur_node_(in_node) {}

  void updateNode(NodeType const& new_node) {
    if (new_node == theContext()->getNode()) {
      state_ = eLocState::Local;
    } else {
      state_ = eLocState::Remote;
    }

    cur_node_ = new_node;
  }

  bool isLocal() const { return state_ == eLocState::Local; }
  bool isRemote() const { return state_ == eLocState::Remote; }
  NodeType getRemoteNode() const { return cur_node_; }
  EntityID getEntityID() const { return id_; }

  template <typename U>
  friend std::ostream& operator<<(std::ostream& os, LocRecord<U> const& rec);

 private:
  EntityID id_;
  LocStateType state_ = eLocState::Invalid;
  NodeType cur_node_ = uninitialized_destination;
};

template <typename EntityID>
std::ostream& operator<<(std::ostream& os, LocRecord<EntityID> const& rec) {
  auto state_val = (int32_t) rec.state_;
  os << "id=" << rec.id_ << ", "
     << "state=" << rec.state_ << ", "
     << "state=" << state_val << ", "
     << "cur_node_=" << rec.cur_node_;
  return os;
}

inline std::ostream& operator<<(std::ostream& os, eLocState const& state) {
  os << PRINT_LOC_STATE(state);
  return os;
}

}}  // end namespace vt::location

#endif  /*INCLUDED_TOPOS_LOCATION_RECORD*/
