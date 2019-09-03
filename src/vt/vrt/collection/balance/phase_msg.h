/*
//@HEADER
// ************************************************************************
//
//                          phase_msg.h
//                     vt (Virtual Transport)
//                  Copyright (C) 2018 NTESS, LLC
//
// Under the terms of Contract DE-NA-0003525 with NTESS, LLC,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact darma@sandia.gov
//
// ************************************************************************
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

}}}} /* end namespace vt::vrt::collection::balance */

#endif /*INCLUDED_VRT_COLLECTION_BALANCE_PHASE_MSG_H*/
