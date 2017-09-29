
#if ! defined __RUNTIME_TRANSPORT_SEQ_ACTION_VIRTUAL__
#define __RUNTIME_TRANSPORT_SEQ_ACTION_VIRTUAL__

#include "config.h"
#include "seq_common.h"
#include "termination.h"

namespace vt { namespace seq {

template <typename MessageT, typename VcT>
struct ActionVirtual {
  using ActionType = std::function<void(MessageT*, VcT*)>;
  using CallableType = std::function<bool()>;

  SeqType const seq_id;
  ActionType const action;

  ActionVirtual(SeqType const& in_seq_id, ActionType const& in_action);

  void runAction(VcT* vc, MessageT* msg, bool const consume = true) const;
  CallableType generateCallable(MessageT* msg, VcT* vc) const;
};

}} //end namespace vt::seq

#include "seq_action_virtual.impl.h"

#endif /* __RUNTIME_TRANSPORT_SEQ_ACTION_VIRTUAL__*/
