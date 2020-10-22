/*
//@HEADER
// *****************************************************************************
//
//                                 phase_msg.h
//                           DARMA Toolkit v. 1.0.0
//                       DARMA/vt => Virtual Transport
//
// Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC
// (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
//
// * Neither the name of the copyright holder nor the names of its
//   contributors may be used to endorse or promote products derived from this
//   software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// *****************************************************************************
//@HEADER
*/

#if !defined INCLUDED_VRT_COLLECTION_BALANCE_PHASE_MSG_H
#define INCLUDED_VRT_COLLECTION_BALANCE_PHASE_MSG_H

#include "vt/config.h"
#include "vt/collective/reduce/reduce.h"
#include "vt/vrt/collection/messages/user.h"
#include "vt/messaging/message.h"

namespace vt { namespace vrt { namespace collection { namespace balance {

template <typename ColT, typename BaseMsgT>
struct PhaseMsgBase : BaseMsgT {
  using ProxyType = typename ColT::CollectionProxyType;
  PhaseMsgBase() = default;

  PhaseMsgBase(
    PhaseType const in_cur_phase, ProxyType const in_proxy,
    bool in_do_sync, bool in_manual
  ) : proxy_(in_proxy), cur_phase_(in_cur_phase), do_sync_(in_do_sync),
      manual_(in_manual)
  { }

  ProxyType getProxy() const { return proxy_; }
  PhaseType getPhase() const { return cur_phase_; }
  bool doSync() const { return do_sync_; }
  bool manual() const { return manual_; }

private:
  ProxyType proxy_ = {};
  PhaseType cur_phase_ = fst_lb_phase;
  bool do_sync_ = true;
  bool manual_ = false;
};

template <typename ColT>
using PhaseMsg = PhaseMsgBase<ColT,CollectionMessage<ColT>>;

template <typename ColT>
using PhaseReduceMsg = PhaseMsgBase<
  ColT,collective::ReduceTMsg<collective::NoneType>
>;

template <typename ColT>
struct CollectStatsMsg : CollectionMessage<ColT> {
  CollectStatsMsg(PhaseType in_phase)
    : phase_(in_phase)
  { }

  PhaseType getPhase() const { return phase_; }

private:
  PhaseType phase_ = fst_lb_phase;
};

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_PHASE_MSG_H*/
