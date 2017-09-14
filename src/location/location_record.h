
#if ! defined __RUNTIME_TRANSPORT_LOCATION_RECORD__
#define __RUNTIME_TRANSPORT_LOCATION_RECORD__

#include "common.h"

namespace vt { namespace location {

enum class eLocState : int8_t {
  Local = 1,
  Remote = 2,
  Invalid = -1
};

struct LocRecord {
  using LocStateType = eLocState;

  LocRecord(LocStateType const& in_state, NodeType const& in_node)
    : state_(in_state), cur_node_(in_node)
  { }

  void updateNode(NodeType const& new_node) {
    if (new_node == theContext->getNode()) {
      state_ = eLocState::Local;
    } else {
      state_ = eLocState::Remote;
    }

    cur_node_ = new_node;
  }

  bool isLocal() const {
    return state_ == eLocState::Local;
  }

  bool isRemote() const {
    return state_ == eLocState::Remote;
  }

  NodeType getRemoteNode() const {
    return cur_node_;
  }

private:
  LocStateType state_ = eLocState::Invalid;
  NodeType cur_node_ = uninitialized_destination;
};

}} // end namespace vt::location

#endif /*__RUNTIME_TRANSPORT_LOCATION_RECORD__*/
