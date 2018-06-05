
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_PHASE_MSG_H
#define INCLUDED_VRT_COLLECTION_BALANCE_PHASE_MSG_H

#include "config.h"
#include "vrt/collection/messages/user.h"
#include "messaging/message.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename ColT>
struct PhaseMsg : CollectionMessage<ColT> {
  PhaseMsg() = default;
  explicit PhaseMsg(PhaseType const& in_cur_phase)
    : cur_phase_(in_cur_phase)
  { }

  PhaseType getPhase() const { return cur_phase_; }
private:
  PhaseType cur_phase_ = fst_lb_phase;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_PHASE_MSG_H*/
