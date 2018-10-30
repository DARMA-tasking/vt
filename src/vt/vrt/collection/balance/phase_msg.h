
#if !defined INCLUDED_VRT_COLLECTION_BALANCE_PHASE_MSG_H
#define INCLUDED_VRT_COLLECTION_BALANCE_PHASE_MSG_H

#include "config.h"
#include "collective/reduce/reduce.h"
#include "vrt/collection/messages/user.h"
#include "messaging/message.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename ColT, typename BaseMsgT>
struct PhaseMsgBase : BaseMsgT {
  using ProxyType = typename ColT::CollectionProxyType;
  PhaseMsgBase() = default;
  PhaseMsgBase(PhaseType const& in_cur_phase, ProxyType const& in_proxy)
    : cur_phase_(in_cur_phase), proxy_(in_proxy), BaseMsgT()
  { }

  ProxyType getProxy() const { return proxy_; }
  PhaseType getPhase() const { return cur_phase_; }
private:
  ProxyType proxy_ = {};
  PhaseType cur_phase_ = fst_lb_phase;
};

template <typename ColT>
using PhaseMsg = PhaseMsgBase<ColT,CollectionMessage<ColT>>;

template <typename ColT>
using PhaseReduceMsg = PhaseMsgBase<
  ColT,collective::ReduceTMsg<collective::NoneType>
>;

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_PHASE_MSG_H*/
